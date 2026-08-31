#include "pulse_config.h"

#include "json.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

using Json = nlohmann::ordered_json;
namespace fs = std::filesystem;

constexpr int32_t kMaxDepth = 64;
constexpr size_t kMaxJsonInputBytes = 16 * 1024 * 1024;

thread_local std::string g_last_error;

struct Node {
    EPulseConfigType type = PULSE_CONFIG_TYPE_NONE;
    int32_t ref_count = 1;

    bool b = false;
    int64_t i = 0;
    double d = 0.0;
    std::string s;
    std::vector<std::pair<std::string, Node*>> object;
    std::vector<Node*> array;

    explicit Node(EPulseConfigType t) : type(t) {}
    ~Node();
};

void release_node(Node* node);

Node::~Node() {
    for (auto& kv : object) {
        release_node(kv.second);
    }
    for (Node* child : array) {
        release_node(child);
    }
}

Node* new_node(EPulseConfigType type) {
    void* mem = ::operator new(sizeof(Node));
    return new (mem) Node(type);
}

void retain_node(Node* node) {
    if (node) {
        ++node->ref_count;
    }
}

void release_node(Node* node) {
    if (node && --node->ref_count == 0) {
        node->~Node();
        ::operator delete(node);
    }
}

Node* to_node(PulseConfig* cfg) {
    return reinterpret_cast<Node*>(cfg);
}

const Node* to_node(const PulseConfig* cfg) {
    return reinterpret_cast<const Node*>(cfg);
}

Node* to_node(PulseConfigArray* arr) {
    return reinterpret_cast<Node*>(arr);
}

const Node* to_node(const PulseConfigArray* arr) {
    return reinterpret_cast<const Node*>(arr);
}

PulseConfig* to_config(Node* node) {
    return reinterpret_cast<PulseConfig*>(node);
}

PulseConfigArray* to_array(Node* node) {
    return reinterpret_cast<PulseConfigArray*>(node);
}

void set_error(const std::string& message) {
    g_last_error = message;
}

void clear_error() {
    g_last_error.clear();
}

bool reaches_node(const Node* start, const Node* target) {
    if (!start || !target) {
        return false;
    }

    std::vector<const Node*> stack;
    std::unordered_set<const Node*> visited;
    stack.push_back(start);

    while (!stack.empty()) {
        const Node* n = stack.back();
        stack.pop_back();

        if (n == target) {
            return true;
        }

        if (!visited.insert(n).second) {
            continue;
        }

        for (const auto& kv : n->object) {
            if (kv.second) {
                stack.push_back(kv.second);
            }
        }
        for (const Node* child : n->array) {
            if (child) {
                stack.push_back(child);
            }
        }
    }

    return false;
}

Node* find_child(const Node* obj, const char* key) {
    if (!obj || obj->type != PULSE_CONFIG_TYPE_OBJECT || !key) {
        return nullptr;
    }
    std::string k(key);
    for (const auto& kv : obj->object) {
        if (kv.first == k) {
            return kv.second;
        }
    }
    return nullptr;
}

Node* node_for_key(Node* cfg, const char* key) {
    if (key) {
        return find_child(cfg, key);
    }
    return cfg;
}

const Node* node_for_key(const Node* cfg, const char* key) {
    if (key) {
        return find_child(cfg, key);
    }
    return cfg;
}

bool json_to_node(const Json& json, Node** out, int depth) {
    if (depth > kMaxDepth) {
        set_error("json: maximum nesting depth exceeded (64)");
        return false;
    }

    if (json.is_boolean()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_BOOL);
        n->b = json.get<bool>();
        *out = n;
        return true;
    }

    if (json.is_number_unsigned()) {
        uint64_t u = json.get<uint64_t>();
        if (u <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
            Node* n = new_node(PULSE_CONFIG_TYPE_INT);
            n->i = static_cast<int64_t>(u);
            *out = n;
        } else {
            Node* n = new_node(PULSE_CONFIG_TYPE_DOUBLE);
            n->d = static_cast<double>(u);
            *out = n;
        }
        return true;
    }

    if (json.is_number_integer()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_INT);
        n->i = json.get<int64_t>();
        *out = n;
        return true;
    }

    if (json.is_number_float()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_DOUBLE);
        n->d = json.get<double>();
        *out = n;
        return true;
    }

    if (json.is_string()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_STRING);
        n->s = json.get<std::string>();
        *out = n;
        return true;
    }

    if (json.is_array()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_ARRAY);
        for (const auto& item : json) {
            Node* child = nullptr;
            if (!json_to_node(item, &child, depth + 1)) {
                release_node(n);
                return false;
            }
            n->array.push_back(child);
        }
        *out = n;
        return true;
    }

    if (json.is_object()) {
        Node* n = new_node(PULSE_CONFIG_TYPE_OBJECT);
        for (auto it = json.begin(); it != json.end(); ++it) {
            Node* child = nullptr;
            if (!json_to_node(it.value(), &child, depth + 1)) {
                release_node(n);
                return false;
            }
            n->object.emplace_back(it.key(), child);
        }
        *out = n;
        return true;
    }

    // null or unknown -> None
    Node* n = new_node(PULSE_CONFIG_TYPE_NONE);
    *out = n;
    return true;
}

bool node_to_json(const Node* node, Json& out, int depth) {
    if (!node || depth > kMaxDepth) {
        set_error("json: maximum nesting depth exceeded (64)");
        return false;
    }

    switch (node->type) {
        case PULSE_CONFIG_TYPE_NONE:
            out = nullptr;
            return true;
        case PULSE_CONFIG_TYPE_BOOL:
            out = node->b;
            return true;
        case PULSE_CONFIG_TYPE_INT:
            out = node->i;
            return true;
        case PULSE_CONFIG_TYPE_DOUBLE:
            out = node->d;
            return true;
        case PULSE_CONFIG_TYPE_STRING:
            out = node->s;
            return true;
        case PULSE_CONFIG_TYPE_ARRAY: {
            Json arr = Json::array();
            for (const Node* child : node->array) {
                Json item;
                if (!node_to_json(child, item, depth + 1)) {
                    return false;
                }
                arr.push_back(std::move(item));
            }
            out = std::move(arr);
            return true;
        }
        case PULSE_CONFIG_TYPE_OBJECT: {
            Json obj = Json::object();
            for (const auto& kv : node->object) {
                Json value;
                if (!node_to_json(kv.second, value, depth + 1)) {
                    return false;
                }
                obj.emplace(kv.first, std::move(value));
            }
            out = std::move(obj);
            return true;
        }
        case PULSE_CONFIG_TYPE_COUNT:
            break;
    }

    set_error("json: unsupported config type");
    return false;
}

Node* copy_node(const Node* node, int depth) {
    if (!node || depth > kMaxDepth) {
        set_error("config: copy depth exceeded");
        return nullptr;
    }

    Node* copy = new_node(node->type);
    copy->b = node->b;
    copy->i = node->i;
    copy->d = node->d;
    copy->s = node->s;

    if (node->type == PULSE_CONFIG_TYPE_OBJECT) {
        for (const auto& kv : node->object) {
            Node* child = copy_node(kv.second, depth + 1);
            if (!child) {
                release_node(copy);
                return nullptr;
            }
            copy->object.emplace_back(kv.first, child);
        }
    } else if (node->type == PULSE_CONFIG_TYPE_ARRAY) {
        for (const Node* child : node->array) {
            Node* child_copy = copy_node(child, depth + 1);
            if (!child_copy) {
                release_node(copy);
                return nullptr;
            }
            copy->array.push_back(child_copy);
        }
    }

    return copy;
}

char* alloc_json_string(const std::string& text) {
    char* mem = static_cast<char*>(std::malloc(text.size() + 1));
    if (!mem) {
        set_error("config: out of memory");
        return nullptr;
    }
    std::memcpy(mem, text.c_str(), text.size() + 1);
    return mem;
}

bool ensure_object(PulseConfig* cfg) {
    Node* node = to_node(cfg);
    if (!node) {
        set_error("config: null config");
        return false;
    }
    if (node->type != PULSE_CONFIG_TYPE_OBJECT) {
        set_error("config: not an object");
        return false;
    }
    return true;
}

void replace_child(Node* obj, const char* key, Node* child) {
    std::string k(key ? key : "");
    for (auto it = obj->object.begin(); it != obj->object.end(); ++it) {
        if (it->first == k) {
            release_node(it->second);
            it->second = child;
            return;
        }
    }
    obj->object.emplace_back(std::move(k), child);
}

bool validate_set_key_object(PulseConfig* cfg, const char* key) {
    if (!key) {
        set_error("config: null key");
        return false;
    }
    return ensure_object(cfg);
}

Node* merge_nodes(const Node* d, const Node* o, int depth) {
    if (depth > kMaxDepth) {
        set_error("config: merge depth exceeded (64)");
        return nullptr;
    }

    if (!d && !o) {
        return new_node(PULSE_CONFIG_TYPE_OBJECT);
    }
    if (!d) {
        return copy_node(o, depth);
    }
    if (!o) {
        return copy_node(d, depth);
    }

    if (d->type != PULSE_CONFIG_TYPE_OBJECT || o->type != PULSE_CONFIG_TYPE_OBJECT) {
        // Non-object overrides replace the whole default value.
        return copy_node(o, depth);
    }

    Node* merged = copy_node(d, depth);
    if (!merged) {
        return nullptr;
    }

    for (const auto& okv : o->object) {
        Node* existing = find_child(merged, okv.first.c_str());
        if (existing && existing->type == PULSE_CONFIG_TYPE_OBJECT &&
            okv.second->type == PULSE_CONFIG_TYPE_OBJECT) {
            Node* child_merged = merge_nodes(existing, okv.second, depth + 1);
            if (!child_merged) {
                release_node(merged);
                return nullptr;
            }
            replace_child(merged, okv.first.c_str(), child_merged);
        } else {
            Node* copied = copy_node(okv.second, depth + 1);
            if (!copied) {
                release_node(merged);
                return nullptr;
            }
            replace_child(merged, okv.first.c_str(), copied);
        }
    }

    return merged;
}

} // namespace

extern "C" {

PULSE_CONFIG_API PulseConfig* pulse_config_create(void) {
    clear_error();
    return to_config(new_node(PULSE_CONFIG_TYPE_OBJECT));
}

PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json(const char* json, size_t len) {
    clear_error();

    if (!json) {
        set_error("json: null input");
        return nullptr;
    }

    if (len == 0) {
        return pulse_config_create();
    }

    if (len > kMaxJsonInputBytes) {
        set_error("json: input exceeds 16 MiB limit");
        return nullptr;
    }

    std::string text(json, len);
    Json parsed;
    try {
        parsed = Json::parse(text);
    } catch (const std::exception& e) {
        set_error(std::string("json: parse error: ") + e.what());
        return nullptr;
    } catch (...) {
        set_error("json: unknown parse error");
        return nullptr;
    }

    Node* root = nullptr;
    if (!json_to_node(parsed, &root, 0)) {
        return nullptr;
    }
    return to_config(root);
}

PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json_file(const char* path) {
    clear_error();
    if (!path || !path[0]) {
        set_error("json: invalid file path");
        return nullptr;
    }

    const fs::path fspath = fs::u8path(path);

    std::error_code ec;
    const std::uintmax_t file_size = fs::file_size(fspath, ec);
    if (!ec && file_size > static_cast<std::uintmax_t>(kMaxJsonInputBytes)) {
        set_error("json: file exceeds 16 MiB limit");
        return nullptr;
    }

    std::ifstream file(fspath, std::ios::binary);
    if (!file) {
        set_error(std::string("json: cannot open file: ") + path);
        return nullptr;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    if (file.bad()) {
        set_error(std::string("json: failed to read file: ") + path);
        return nullptr;
    }

    std::string text = ss.str();
    return pulse_config_create_from_json(text.c_str(), text.size());
}

PULSE_CONFIG_API void pulse_config_addref(PulseConfig* cfg) {
    retain_node(to_node(cfg));
}

PULSE_CONFIG_API void pulse_config_release(PulseConfig* cfg) {
    release_node(to_node(cfg));
}

PULSE_CONFIG_API void pulse_config_free_string(char* str) {
    std::free(str);
}

PULSE_CONFIG_API bool pulse_config_has(const PulseConfig* cfg, const char* key) {
    const Node* node = to_node(cfg);
    if (!node || node->type != PULSE_CONFIG_TYPE_OBJECT || !key) {
        return false;
    }
    return find_child(node, key) != nullptr;
}

PULSE_CONFIG_API EPulseConfigType pulse_config_get_type(const PulseConfig* cfg, const char* key) {
    const Node* node = to_node(cfg);
    const Node* child = node_for_key(node, key);
    return child ? child->type : PULSE_CONFIG_TYPE_NONE;
}

PULSE_CONFIG_API bool pulse_config_get_bool(const PulseConfig* cfg, const char* key, bool default_value) {
    const Node* child = node_for_key(to_node(cfg), key);
    if (child && child->type == PULSE_CONFIG_TYPE_BOOL) {
        return child->b;
    }
    return default_value;
}

PULSE_CONFIG_API int64_t pulse_config_get_int(const PulseConfig* cfg, const char* key, int64_t default_value) {
    const Node* child = node_for_key(to_node(cfg), key);
    if (!child) {
        return default_value;
    }
    if (child->type == PULSE_CONFIG_TYPE_INT) {
        return child->i;
    }
    if (child->type == PULSE_CONFIG_TYPE_DOUBLE) {
        double d = child->d;
        if (d >= static_cast<double>(std::numeric_limits<int64_t>::min()) &&
            d < static_cast<double>(std::numeric_limits<int64_t>::max())) {
            return static_cast<int64_t>(d);
        }
    }
    return default_value;
}

PULSE_CONFIG_API double pulse_config_get_double(const PulseConfig* cfg, const char* key, double default_value) {
    const Node* child = node_for_key(to_node(cfg), key);
    if (!child) {
        return default_value;
    }
    if (child->type == PULSE_CONFIG_TYPE_DOUBLE) {
        return child->d;
    }
    if (child->type == PULSE_CONFIG_TYPE_INT) {
        return static_cast<double>(child->i);
    }
    return default_value;
}

PULSE_CONFIG_API const char* pulse_config_get_string(const PulseConfig* cfg, const char* key, const char* default_value) {
    const Node* child = node_for_key(to_node(cfg), key);
    if (child && child->type == PULSE_CONFIG_TYPE_STRING) {
        return child->s.c_str();
    }
    return default_value;
}

PULSE_CONFIG_API PulseConfig* pulse_config_get_obj(const PulseConfig* cfg, const char* key) {
    Node* child = const_cast<Node*>(node_for_key(to_node(cfg), key));
    if (child && child->type == PULSE_CONFIG_TYPE_OBJECT) {
        return to_config(child);
    }
    return nullptr;
}

PULSE_CONFIG_API PulseConfigArray* pulse_config_get_array(const PulseConfig* cfg, const char* key) {
    Node* child = const_cast<Node*>(node_for_key(to_node(cfg), key));
    if (child && child->type == PULSE_CONFIG_TYPE_ARRAY) {
        return to_array(child);
    }
    return nullptr;
}

PULSE_CONFIG_API size_t pulse_config_array_count(const PulseConfigArray* arr) {
    const Node* node = to_node(arr);
    if (!node || node->type != PULSE_CONFIG_TYPE_ARRAY) {
        return 0;
    }
    return node->array.size();
}

PULSE_CONFIG_API PulseConfig* pulse_config_array_get(const PulseConfigArray* arr, size_t index) {
    const Node* node = to_node(arr);
    if (!node || node->type != PULSE_CONFIG_TYPE_ARRAY || index >= node->array.size()) {
        return nullptr;
    }
    return to_config(node->array[index]);
}

PULSE_CONFIG_API void pulse_config_set_bool(PulseConfig* cfg, const char* key, bool value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = new_node(PULSE_CONFIG_TYPE_BOOL);
    child->b = value;
    replace_child(to_node(cfg), key, child);
}

PULSE_CONFIG_API void pulse_config_set_int(PulseConfig* cfg, const char* key, int64_t value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = new_node(PULSE_CONFIG_TYPE_INT);
    child->i = value;
    replace_child(to_node(cfg), key, child);
}

PULSE_CONFIG_API void pulse_config_set_double(PulseConfig* cfg, const char* key, double value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = new_node(PULSE_CONFIG_TYPE_DOUBLE);
    child->d = value;
    replace_child(to_node(cfg), key, child);
}

PULSE_CONFIG_API void pulse_config_set_string(PulseConfig* cfg, const char* key, const char* value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = new_node(PULSE_CONFIG_TYPE_STRING);
    child->s = value ? value : "";
    replace_child(to_node(cfg), key, child);
}

PULSE_CONFIG_API void pulse_config_set_obj(PulseConfig* cfg, const char* key, PulseConfig* value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = to_node(value);
    if (!child) {
        set_error("config: null object value");
        return;
    }
    if (child->type != PULSE_CONFIG_TYPE_OBJECT) {
        set_error("config: value is not an object");
        return;
    }
    Node* obj = to_node(cfg);
    if (reaches_node(child, obj)) {
        set_error("config: cannot set object: cycle detected");
        return;
    }
    retain_node(child);
    replace_child(obj, key, child);
}

PULSE_CONFIG_API void pulse_config_set_array(PulseConfig* cfg, const char* key, PulseConfigArray* value) {
    if (!validate_set_key_object(cfg, key)) return;
    Node* child = to_node(value);
    if (!child) {
        set_error("config: null array value");
        return;
    }
    if (child->type != PULSE_CONFIG_TYPE_ARRAY) {
        set_error("config: value is not an array");
        return;
    }
    Node* obj = to_node(cfg);
    if (reaches_node(child, obj)) {
        set_error("config: cannot set array: cycle detected");
        return;
    }
    retain_node(child);
    replace_child(obj, key, child);
}

PULSE_CONFIG_API bool pulse_config_remove(PulseConfig* cfg, const char* key) {
    Node* node = to_node(cfg);
    if (!node || node->type != PULSE_CONFIG_TYPE_OBJECT || !key) {
        return false;
    }
    std::string k(key);
    for (auto it = node->object.begin(); it != node->object.end(); ++it) {
        if (it->first == k) {
            release_node(it->second);
            node->object.erase(it);
            return true;
        }
    }
    return false;
}

PULSE_CONFIG_API char* pulse_config_to_json(const PulseConfig* cfg, size_t* out_len) {
    clear_error();
    const Node* node = to_node(cfg);
    if (!node) {
        set_error("config: null config");
        return nullptr;
    }
    Json j;
    if (!node_to_json(node, j, 0)) {
        return nullptr;
    }
    std::string text = j.dump();
    char* result = alloc_json_string(text);
    if (result && out_len) {
        *out_len = text.size();
    }
    return result;
}

PULSE_CONFIG_API char* pulse_config_to_json_pretty(const PulseConfig* cfg, size_t* out_len) {
    clear_error();
    const Node* node = to_node(cfg);
    if (!node) {
        set_error("config: null config");
        return nullptr;
    }
    Json j;
    if (!node_to_json(node, j, 0)) {
        return nullptr;
    }
    std::string text = j.dump(2);
    char* result = alloc_json_string(text);
    if (result && out_len) {
        *out_len = text.size();
    }
    return result;
}

PULSE_CONFIG_API PulseConfig* pulse_config_merge(const PulseConfig* defaults, const PulseConfig* overrides) {
    clear_error();
    return to_config(merge_nodes(to_node(defaults), to_node(overrides), 0));
}

PULSE_CONFIG_API const char* pulse_config_last_error(void) {
    return g_last_error.c_str();
}

} // extern "C"
