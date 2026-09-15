#include "prefab_internal.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define PREFAB_TLS __declspec(thread)
#else
#define PREFAB_TLS _Thread_local
#endif

namespace pulse_prefab_internal {

namespace {

PREFAB_TLS char g_last_error[256];

const char* set_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error, sizeof(g_last_error), format, args);
    va_end(args);
    return g_last_error;
}

struct prefab_reference {
    uint64_t type_id = 0;
    std::string path;
};

struct prefab_reference_set {
    std::vector<prefab_reference> references;
};

struct prefab_apply_ctx {
    ecs_world_t* world = nullptr;
    prefab_reference_set* references = nullptr;
};

PulseAssetRequest request_file_load(PulseAssetSystemId asset_system, uint64_t type_id, const char* path) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    return pulse_asset_system_load(asset_system, &desc);
}

bool asset_state_is_pending(EPulseAssetState state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD || state == PULSE_ASSET_STATE_LOADING || state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES || state == PULSE_ASSET_STATE_PROCESSING;
}

bool node_is_container(const PulseDatalist* node) {
    EPulseDatalistType type = pulse_datalist_get_type(node, nullptr);
    return type == PULSE_DATALIST_TYPE_LIST || type == PULSE_DATALIST_TYPE_MAP || type == PULSE_DATALIST_TYPE_MIXED;
}

bool node_is_empty_container(const PulseDatalist* node) {
    return pulse_datalist_object_count(node) == 0 && pulse_datalist_count(node) == 0;
}

bool root_section_is_known(const char* key) {
    return strcmp(key, "name") == 0 || strcmp(key, "extends") == 0 || strcmp(key, "components") == 0 || strcmp(key, "tags") == 0 || strcmp(key, "pairs") == 0 || strcmp(key, "children") == 0;
}

bool root_has_section(const PulseDatalist* root) {
    size_t count = pulse_datalist_object_count(root);
    for (size_t i = 0; i < count; ++i) {
        const char* key = pulse_datalist_object_key(root, i);
        if (key && root_section_is_known(key)) {
            return true;
        }
    }
    return false;
}

bool check_known_sections(const PulseDatalist* node, const char* what, const char** out_error) {
    size_t count = pulse_datalist_object_count(node);
    for (size_t i = 0; i < count; ++i) {
        const char* key = pulse_datalist_object_key(node, i);
        if (key && !root_section_is_known(key)) {
            *out_error = set_error("prefab loader: unknown %s key '%s'", what, key);
            return false;
        }
    }
    return true;
}

ecs_id_t component_id_from_datalist(ecs_world_t* world, const char* name);

bool apply_tags(ecs_world_t* world, ecs_entity_t entity, const PulseDatalist* root, const char** out_error) {
    if (!pulse_datalist_has(root, "tags")) {
        return true;
    }

    PulseDatalist* tags = pulse_datalist_get_obj(root, "tags");
    if (!tags || pulse_datalist_object_count(tags) != 0) {
        *out_error = "prefab loader: 'tags' must be a list of tag names";
        return false;
    }

    size_t count = pulse_datalist_count(tags);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* item = pulse_datalist_get(tags, i);
        const char* name = item ? pulse_datalist_get_string(item, nullptr, nullptr) : nullptr;
        if (!name || !name[0]) {
            *out_error = "prefab loader: 'tags' entries must be tag names";
            return false;
        }
        if (strchr(name, '(')) {
            *out_error = set_error("prefab loader: '%s' is a pair, write it in 'pairs'", name);
            return false;
        }

        ecs_id_t id = ecs_lookup_path_w_sep(world, 0, name, ".", nullptr, false);
        if (!id) {
            *out_error = set_error("prefab loader: unknown tag '%s'", name);
            return false;
        }
        if (!ecs_id_is_tag(world, id)) {
            *out_error = set_error("prefab loader: '%s' is a component, write it in 'components'", name);
            return false;
        }
        ecs_add_id(world, entity, id);
    }
    return true;
}

bool apply_pair_targets(ecs_world_t* world, ecs_entity_t entity, ecs_entity_t rel, const PulseDatalist* node, const char** out_error) {
    if (node_is_container(node) && !node_is_empty_container(node)) {
        size_t count = pulse_datalist_count(node);
        for (size_t i = 0; i < count; ++i) {
            PulseDatalist* item = pulse_datalist_get(node, i);
            const char* target = item ? pulse_datalist_get_string(item, nullptr, nullptr) : nullptr;
            if (!target || !target[0]) {
                *out_error = "prefab loader: pair targets must be names";
                return false;
            }
            ecs_entity_t tgt = ecs_lookup_path_w_sep(world, 0, target, ".", nullptr, false);
            if (!tgt) {
                *out_error = set_error("prefab loader: pair target '%s' not found", target);
                return false;
            }
            ecs_add_id(world, entity, ecs_pair(rel, tgt));
        }
        return true;
    }

    const char* target = pulse_datalist_get_string(node, nullptr, nullptr);
    if (!target || !target[0]) {
        *out_error = "prefab loader: pair targets must be names";
        return false;
    }
    ecs_entity_t tgt = ecs_lookup_path_w_sep(world, 0, target, ".", nullptr, false);
    if (!tgt) {
        *out_error = set_error("prefab loader: pair target '%s' not found", target);
        return false;
    }
    ecs_add_id(world, entity, ecs_pair(rel, tgt));
    return true;
}

bool apply_pairs(ecs_world_t* world, ecs_entity_t entity, const PulseDatalist* root, const char** out_error) {
    if (!pulse_datalist_has(root, "pairs")) {
        return true;
    }

    PulseDatalist* pairs = pulse_datalist_get_obj(root, "pairs");
    if (!pairs || pulse_datalist_count(pairs) != 0) {
        *out_error = "prefab loader: 'pairs' must be a map of relationship to target";
        return false;
    }

    size_t count = pulse_datalist_object_count(pairs);
    for (size_t i = 0; i < count; ++i) {
        const char* rel_name = pulse_datalist_object_key(pairs, i);
        PulseDatalist* node = pulse_datalist_object_value(pairs, i);
        if (!rel_name || !node) {
            continue;
        }

        ecs_entity_t rel = ecs_lookup_path_w_sep(world, 0, rel_name, ".", nullptr, false);
        if (!rel) {
            *out_error = set_error("prefab loader: unknown relationship '%s'", rel_name);
            return false;
        }
        if (!apply_pair_targets(world, entity, rel, node, out_error)) {
            return false;
        }
    }
    return true;
}

void collect_reference(prefab_apply_ctx* ctx, uint64_t type_id, const char* path) {
    if (!ctx->references || !type_id || !path || !path[0]) {
        return;
    }
    for (const prefab_reference& seen : ctx->references->references) {
        if (seen.type_id == type_id && seen.path == path) {
            return;
        }
    }
    ctx->references->references.push_back(prefab_reference{ type_id, path });
}

bool walk_node(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);
bool walk_struct(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);
bool walk_collection(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);

bool apply_scalar(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    EPulseDatalistType type = pulse_datalist_get_type(node, nullptr);
    if (type == PULSE_DATALIST_TYPE_NIL) {
        *out_error = "prefab loader: nil member value";
        return false;
    }
    if (ecs_meta_get_type(cursor) == ecs_id(ecs_entity_t)) {
        const char* text = pulse_datalist_get_string(node, nullptr, nullptr);
        if (!text) {
            *out_error = "prefab loader: entity reference needs a path string";
            return false;
        }
        return ecs_meta_set_entity(cursor, ecs_lookup_path_w_sep(ctx->world, 0, text, ".", nullptr, false)) == 0;
    }

    switch (type) {
    case PULSE_DATALIST_TYPE_BOOL:
        return ecs_meta_set_bool(cursor, pulse_datalist_get_bool(node, nullptr, false)) == 0;
    case PULSE_DATALIST_TYPE_INT:
        return ecs_meta_set_int(cursor, pulse_datalist_get_int(node, nullptr, 0)) == 0;
    case PULSE_DATALIST_TYPE_DOUBLE:
        return ecs_meta_set_float(cursor, pulse_datalist_get_double(node, nullptr, 0.0)) == 0;
    case PULSE_DATALIST_TYPE_STRING:
        return ecs_meta_set_string(cursor, pulse_datalist_get_string(node, nullptr, "")) == 0;
    default:
        *out_error = "prefab loader: member value is not a scalar";
        return false;
    }
}

bool walk_member(ecs_meta_cursor_t* cursor, const char* name, const PulseDatalist* value, prefab_apply_ctx* ctx, const char** out_error) {
    if (strchr(name, '.') != nullptr || ecs_meta_try_dotmember(cursor, name) != 0) {
        return true;
    }

    ecs_entity_t member_type = node_is_container(value) ? 0 : ecs_meta_get_type(cursor);
    const EcsOpaque* opaque = member_type ? ecs_get(ctx->world, member_type, EcsOpaque) : nullptr;
    if (ctx->references && opaque && opaque->as_type == ecs_id(ecs_string_t) && opaque->user_data != 0) {
        collect_reference(ctx, opaque->user_data, pulse_datalist_get_string(value, nullptr, nullptr));
        return true;
    }

    return walk_node(cursor, value, ctx, out_error);
}

bool walk_struct(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    size_t count = pulse_datalist_object_count(node);
    for (size_t i = 0; i < count; ++i) {
        const char* key = pulse_datalist_object_key(node, i);
        PulseDatalist* value = pulse_datalist_object_value(node, i);
        if (!key || !value) {
            continue;
        }
        if (!walk_member(cursor, key, value, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool walk_collection(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    size_t count = pulse_datalist_count(node);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* item = pulse_datalist_get(node, i);
        if (!item) {
            continue;
        }
        if (ecs_meta_elem(cursor, (int32_t)i) != 0) {
            *out_error = "prefab loader: collection has more elements than the reflected member";
            return false;
        }
        if (!walk_node(cursor, item, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool walk_node(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    if (!node_is_container(node)) {
        return apply_scalar(cursor, node, ctx, out_error);
    }
    if (ecs_meta_push(cursor) != 0) {
        *out_error = "prefab loader: member value does not match the reflected type";
        return false;
    }

    bool is_collection = ecs_meta_is_collection(cursor);
    if (!(is_collection ? walk_collection(cursor, node, ctx, out_error) : walk_struct(cursor, node, ctx, out_error))) {
        return false;
    }
    if (ecs_meta_pop(cursor) != 0) {
        *out_error = "prefab loader: failed to close member scope";
        return false;
    }
    return true;
}

ecs_id_t component_id_from_datalist(ecs_world_t* world, const char* name) {
    const char* open = strchr(name, '(');
    if (!open) {
        return ecs_lookup_path_w_sep(world, 0, name, ".", nullptr, false);
    }

    size_t path_len = strlen(name);
    size_t rel_len = static_cast<size_t>(open - name);
    if (rel_len == 0 || path_len < rel_len + 2 || name[path_len - 1] != ')') {
        return 0;
    }

    std::string rel_name(name, rel_len);
    std::string tgt_name(name + rel_len + 1, path_len - rel_len - 2);
    ecs_entity_t rel = ecs_lookup_path_w_sep(world, 0, rel_name.c_str(), ".", nullptr, false);
    ecs_entity_t tgt = ecs_lookup_path_w_sep(world, 0, tgt_name.c_str(), ".", nullptr, false);
    if (!rel || !tgt) {
        return 0;
    }
    return ecs_pair(rel, tgt);
}

const ecs_type_info_t* reflected_type_info(ecs_world_t* world, ecs_id_t id) {
    const ecs_type_info_t* type_info = id ? ecs_get_type_info(world, id) : nullptr;
    return type_info && type_info->component ? type_info : nullptr;
}

bool apply_component(ecs_world_t* world, ecs_entity_t entity, const char* name, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    ecs_id_t id = component_id_from_datalist(world, name);
    if (!id) {
        return true;
    }

    const ecs_type_info_t* type_info = reflected_type_info(world, id);
    if (pulse_datalist_get_type(node, nullptr) == PULSE_DATALIST_TYPE_NIL || node_is_empty_container(node)) {
        if (type_info) {
            ecs_ensure_id(world, entity, id, static_cast<size_t>(type_info->size));
            ecs_modified_id(world, entity, id);
        } else {
            ecs_add_id(world, entity, id);
        }
        return true;
    }
    if (!type_info) {
        return true;
    }

    void* ptr = ecs_ensure_id(world, entity, id, static_cast<size_t>(type_info->size));
    if (!ptr) {
        *out_error = "prefab loader: component has no reflection data";
        return false;
    }

    ecs_meta_cursor_t cursor = ecs_meta_cursor(world, type_info->component, ptr);
    if (!cursor.valid) {
        *out_error = "prefab loader: component has no reflection data";
        return false;
    }
    if (!walk_node(&cursor, node, ctx, out_error)) {
        return false;
    }

    ecs_modified_id(world, entity, id);
    return true;
}

bool scan_node_references(ecs_world_t* world, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    PulseDatalist* components = pulse_datalist_get_obj(node, "components");
    if (!components) {
        return true;
    }

    size_t count = pulse_datalist_object_count(components);
    for (size_t i = 0; i < count; ++i) {
        const char* name = pulse_datalist_object_key(components, i);
        PulseDatalist* member = pulse_datalist_object_value(components, i);
        if (!name || !member) {
            continue;
        }

        const ecs_type_info_t* type_info = reflected_type_info(world, component_id_from_datalist(world, name));
        if (!type_info || type_info->size == 0) {
            continue;
        }

        std::vector<unsigned char> scratch(static_cast<size_t>(type_info->size));
        ecs_meta_cursor_t cursor = ecs_meta_cursor(world, type_info->component, scratch.data());
        if (!cursor.valid) {
            continue;
        }
        if (!walk_node(&cursor, member, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool scan_children_references(ecs_world_t* world, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    PulseDatalist* children = pulse_datalist_get_obj(node, "children");
    if (!children) {
        return true;
    }

    size_t count = pulse_datalist_count(children);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* child = pulse_datalist_get(children, i);
        if (!child) {
            continue;
        }
        if (!scan_node_references(world, child, ctx, out_error) || !scan_children_references(world, child, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool scan_asset_references(ecs_world_t* world, const PulseDatalist* root, prefab_reference_set* references, const char** out_error) {
    if (!node_is_container(root)) {
        *out_error = "prefab loader: prefab root is not an object";
        return false;
    }

    prefab_apply_ctx ctx{ world, references };
    return scan_node_references(world, root, &ctx, out_error) && scan_children_references(world, root, &ctx, out_error);
}

bool prefab_name_is_unique(const prefab_library& library, const char* name) {
    for (const prefab_definition& definition : library.definitions) {
        if (definition.name == name) {
            return false;
        }
    }
    return true;
}

bool collect_single_prefab(const PulseDatalist* root, prefab_library* library, const char** out_error) {
    if (!check_known_sections(root, "section", out_error)) {
        return false;
    }

    prefab_definition definition;
    const char* name = pulse_datalist_get_string(root, "name", nullptr);
    if (name && name[0] && name[0] != '#') {
        definition.name = name;
    }
    definition.node = root;
    library->definitions.push_back(definition);
    library->root = 0;
    return true;
}

size_t find_definition_by_node(const prefab_library& library, const PulseDatalist* node) {
    for (size_t i = 0; i < library.definitions.size(); ++i) {
        if (library.definitions[i].node == node) {
            return i;
        }
    }
    return SIZE_MAX;
}

std::string definition_label(const prefab_library& library, size_t index) {
    if (!library.definitions[index].name.empty()) {
        return library.definitions[index].name;
    }
    return std::string("#") + std::to_string(index);
}

bool collect_prefab_library(const PulseDatalist* root, prefab_library* library, const char** out_error) {
    size_t count = pulse_datalist_object_count(root);
    for (size_t i = 0; i < count; ++i) {
        const char* key = pulse_datalist_object_key(root, i);
        PulseDatalist* node = pulse_datalist_object_value(root, i);
        if (!key || !key[0]) {
            *out_error = "prefab loader: prefab entry key must be a name";
            return false;
        }
        if (!node_is_container(node) || pulse_datalist_object_count(node) == 0) {
            *out_error = set_error("prefab loader: prefab entry '%s' must be an object with at least one section", key);
            return false;
        }
        if (!check_known_sections(node, "prefab", out_error)) {
            return false;
        }
        if (find_definition_by_node(*library, node) != SIZE_MAX) {
            *out_error = set_error("prefab loader: prefab entry '%s' is the same node as an earlier entry", key);
            return false;
        }

        prefab_definition definition;
        const char* name = pulse_datalist_get_string(node, "name", nullptr);
        definition.name = name && name[0] && name[0] != '#' ? name : key;
        definition.node = node;
        if (!prefab_name_is_unique(*library, definition.name.c_str())) {
            *out_error = set_error("prefab loader: duplicate prefab name '%s'", definition.name.c_str());
            return false;
        }
        library->definitions.push_back(definition);
    }
    library->root = 0;
    return true;
}

bool mark_child_references(const PulseDatalist* node, const prefab_library& library, std::vector<const PulseDatalist*>* visited, std::vector<bool>* referenced, const char** out_error) {
    for (const PulseDatalist* seen : *visited) {
        if (seen == node) {
            return true;
        }
    }
    visited->push_back(node);

    PulseDatalist* children = pulse_datalist_get_obj(node, "children");
    if (!children) {
        return true;
    }
    if (pulse_datalist_object_count(children) != 0) {
        *out_error = "prefab loader: 'children' must be a list of child entities";
        return false;
    }

    size_t count = pulse_datalist_count(children);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* child = pulse_datalist_get(children, i);
        if (!child || !node_is_container(child)) {
            continue;
        }
        size_t index = find_definition_by_node(library, child);
        if (index != SIZE_MAX) {
            (*referenced)[index] = true;
        }
        if (!mark_child_references(child, library, visited, referenced, out_error)) {
            return false;
        }
    }
    return true;
}

bool collect_section_list(const PulseDatalist* root, prefab_library* library, const char** out_error) {
    size_t count = pulse_datalist_count(root);
    if (count == 0) {
        *out_error = "prefab loader: file declares no prefab";
        return false;
    }

    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* node = pulse_datalist_get(root, i);
        if (!node_is_container(node) || pulse_datalist_object_count(node) == 0) {
            *out_error = set_error("prefab loader: list entry #%zu must be an object with at least one section", i);
            return false;
        }
        if (!check_known_sections(node, "entry", out_error)) {
            return false;
        }
        if (find_definition_by_node(*library, node) != SIZE_MAX) {
            *out_error = set_error("prefab loader: list entry #%zu is the same node as an earlier entry", i);
            return false;
        }

        prefab_definition definition;
        const char* name = pulse_datalist_get_string(node, "name", nullptr);
        if (name && name[0] && name[0] != '#') {
            definition.name = name;
        }
        definition.node = node;
        library->definitions.push_back(definition);
    }

    std::vector<const PulseDatalist*> visited;
    std::vector<bool> referenced(library->definitions.size(), false);
    for (const prefab_definition& definition : library->definitions) {
        if (!mark_child_references(definition.node, *library, &visited, &referenced, out_error)) {
            return false;
        }
    }

    size_t root_count = 0;
    size_t root_index = SIZE_MAX;
    for (size_t i = 0; i < referenced.size(); ++i) {
        if (referenced[i]) {
            continue;
        }
        if (root_count == 0) {
            root_index = i;
        } else {
            *out_error = set_error("prefab loader: list entry '%s' is not referenced by any 'children'", definition_label(*library, i).c_str());
            return false;
        }
        ++root_count;
    }
    if (root_count == 0) {
        *out_error = "prefab loader: list form needs one unreferenced entry as root, found none";
        return false;
    }

    library->root = root_index;
    library->single_root = true;
    return true;
}

bool collect_library(const PulseDatalist* root, prefab_library* library, const char** out_error) {
    if (!node_is_container(root)) {
        *out_error = "prefab loader: prefab root is not an object";
        return false;
    }
    if (pulse_datalist_object_count(root) == 0) {
        return collect_section_list(root, library, out_error);
    }
    if (root_has_section(root)) {
        return collect_single_prefab(root, library, out_error);
    }
    if (!collect_prefab_library(root, library, out_error)) {
        return false;
    }
    if (library->definitions.empty()) {
        *out_error = "prefab loader: file declares no prefab";
        return false;
    }
    return true;
}

ecs_entity_t resolve_prefab_target(ecs_world_t* world, const prefab_library& library, const char* name) {
    for (const prefab_definition& definition : library.definitions) {
        if (definition.name == name) {
            return definition.entity;
        }
    }
    return ecs_lookup_path_w_sep(world, 0, name, ".", nullptr, false);
}

bool build_prefab_parent(prefab_library* library, size_t index, const char** out_error) {
    if (library->definitions[index].expanded) {
        return true;
    }
    library->definitions[index].expanded = true;

    PulseDatalist* children = pulse_datalist_get_obj(library->definitions[index].node, "children");
    if (!children) {
        return true;
    }
    if (pulse_datalist_object_count(children) != 0) {
        *out_error = "prefab loader: 'children' must be a list of child entities";
        return false;
    }

    size_t count = pulse_datalist_count(children);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* child = pulse_datalist_get(children, i);
        if (!child || !node_is_container(child)) {
            *out_error = "prefab loader: 'children' entries must be objects";
            return false;
        }
        if (!check_known_sections(child, "child", out_error)) {
            return false;
        }

        size_t existing = find_definition_by_node(*library, child);
        if (existing != SIZE_MAX) {
            if (library->definitions[existing].parent != SIZE_MAX) {
                *out_error = set_error("prefab loader: '%s' is referenced as a child more than once", definition_label(*library, existing).c_str());
                return false;
            }
            library->definitions[existing].parent = index;
            if (!build_prefab_parent(library, existing, out_error)) {
                return false;
            }
            continue;
        }

        prefab_definition nested;
        const char* name = pulse_datalist_get_string(child, "name", nullptr);
        if (name && name[0] && name[0] != '#') {
            nested.name = name;
        }
        nested.node = child;
        nested.parent = index;
        library->definitions.push_back(nested);

        if (!build_prefab_parent(library, library->definitions.size() - 1, out_error)) {
            return false;
        }
    }
    return true;
}

ecs_entity_t build_prefab_entity(ecs_world_t* world, ecs_entity_t entity, const PulseDatalist* node, const char** out_error) {
    prefab_apply_ctx ctx{ world, nullptr };
    PulseDatalist* components = pulse_datalist_get_obj(node, "components");
    if (components) {
        size_t count = pulse_datalist_object_count(components);
        for (size_t i = 0; i < count; ++i) {
            const char* component_name = pulse_datalist_object_key(components, i);
            PulseDatalist* member = pulse_datalist_object_value(components, i);
            if (!component_name || !member) {
                continue;
            }
            if (!apply_component(world, entity, component_name, member, &ctx, out_error)) {
                return 0;
            }
        }
    }

    if (!apply_tags(world, entity, node, out_error)) {
        return 0;
    }
    if (!apply_pairs(world, entity, node, out_error)) {
        return 0;
    }
    return entity;
}

void delete_prefab_entities(ecs_world_t* world, const prefab_library& library) {
    for (size_t i = library.definitions.size(); i > 0; --i) {
        ecs_entity_t entity = library.definitions[i - 1].entity;
        if (entity && ecs_is_alive(world, entity)) {
            ecs_delete(world, entity);
        }
    }
}

bool resolve_prefab_bases(prefab_library* library, const char** out_error) {
    for (size_t i = 0; i < library->definitions.size(); ++i) {
        prefab_definition& definition = library->definitions[i];
        if (!pulse_datalist_has(definition.node, "extends")) {
            continue;
        }
        const char* base_name = pulse_datalist_get_string(definition.node, "extends", nullptr);
        if (!base_name || !base_name[0]) {
            *out_error = "prefab loader: 'extends' must be a prefab name";
            return false;
        }
        for (size_t j = 0; j < library->definitions.size(); ++j) {
            if (library->definitions[j].name == base_name) {
                definition.base = j;
                break;
            }
        }
    }

    for (size_t i = 0; i < library->definitions.size(); ++i) {
        size_t steps = 0;
        for (size_t at = i; at != SIZE_MAX; at = library->definitions[at].base) {
            if (++steps > library->definitions.size()) {
                *out_error = set_error("prefab loader: 'extends' cycle at '%s'", library->definitions[i].name.c_str());
                return false;
            }
        }
    }
    return true;
}

bool build_prefab_library(ecs_world_t* world, prefab_library* library, const char** out_error) {
    if (library->single_root) {
        if (!build_prefab_parent(library, library->root, out_error)) {
            return false;
        }
    } else {
        size_t top_count = library->definitions.size();
        for (size_t i = 0; i < top_count; ++i) {
            if (!build_prefab_parent(library, i, out_error)) {
                return false;
            }
        }
    }
    if (!resolve_prefab_bases(library, out_error)) {
        return false;
    }

    for (prefab_definition& definition : library->definitions) {
        definition.entity = ecs_new(world);
        if (!definition.name.empty()) {
            ecs_set_name(world, definition.entity, definition.name.c_str());
        }
    }

    for (const prefab_definition& definition : library->definitions) {
        if (definition.parent != SIZE_MAX) {
            ecs_add_pair(world, definition.entity, EcsChildOf, library->definitions[definition.parent].entity);
        }
    }

    for (const prefab_definition& definition : library->definitions) {
        if (!build_prefab_entity(world, definition.entity, definition.node, out_error)) {
            delete_prefab_entities(world, *library);
            return false;
        }
        ecs_add_id(world, definition.entity, EcsPrefab);
    }

    for (const prefab_definition& definition : library->definitions) {
        if (definition.base != SIZE_MAX) {
            ecs_add_pair(world, definition.entity, EcsIsA, library->definitions[definition.base].entity);
            continue;
        }
        if (!pulse_datalist_has(definition.node, "extends")) {
            continue;
        }
        const char* base_name = pulse_datalist_get_string(definition.node, "extends", nullptr);
        ecs_entity_t base = resolve_prefab_target(world, *library, base_name);
        if (!base) {
            delete_prefab_entities(world, *library);
            *out_error = set_error("prefab loader: unknown prefab '%s' in 'extends'", base_name);
            return false;
        }
        ecs_add_pair(world, definition.entity, EcsIsA, base);
    }
    return true;
}

void destroy_prefab(void* ptr, void* user_data) {
    auto* data = static_cast<PulsePrefabData*>(ptr);
    PulseAppId app = static_cast<PulseAppId>(user_data);
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && data->entities) {
        for (uint32_t i = data->entity_count; i > 0; --i) {
            ecs_entity_t entity = data->entities[i - 1];
            if (entity && ecs_is_alive(world, entity)) {
                ecs_delete(world, entity);
            }
        }
    }
    delete[] data->entities;
    data->entities = nullptr;
    data->entity_count = 0;
    data->root = 0;
}

EPulseAssetLoaderStatus step_prefab_load(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* s = static_cast<prefab_load_state*>(state);

    ecs_world_t* world = pulse_app_world(ctx->app);
    if (!world) {
        *out_error = "prefab loader: app world unavailable";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    if (!s->datalist) {
        s->datalist = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        if (!s->datalist) {
            *out_error = pulse_datalist_last_error();
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        s->library = new prefab_library();
        if (!collect_library(s->datalist, s->library, out_error)) {
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
    }

    if (!s->references_ready) {
        prefab_reference_set references;
        for (const prefab_definition& definition : s->library->definitions) {
            if (!scan_asset_references(world, definition.node, &references, out_error)) {
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            }
        }

        for (const prefab_reference& reference : references.references) {
            PulseAssetRequest request = request_file_load(ctx->asset_system, reference.type_id, reference.path.c_str());
            if (asset_state_is_pending(pulse_asset_system_get_state(ctx->asset_system, request))) {
                return PULSE_ASSET_LOADER_STATUS_PENDING;
            }
        }
        s->references_ready = true;
    }

    bool suspended = ecs_is_deferred(world);
    if (suspended) {
        ecs_defer_suspend(world);
    }

    bool built = build_prefab_library(world, s->library, out_error);

    if (suspended) {
        ecs_defer_resume(world);
    }

    if (!built) {
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    auto* data = static_cast<PulsePrefabData*>(ctx->out_asset);
    size_t count = s->library->definitions.size();
    data->entities = new ecs_entity_t[count];
    data->entity_count = static_cast<uint32_t>(count);
    for (size_t i = 0; i < count; ++i) {
        data->entities[i] = s->library->definitions[i].entity;
    }
    data->root = s->library->definitions[s->library->root].entity;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void dtor_prefab_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    auto* s = static_cast<prefab_load_state*>(state);
    pulse_datalist_release(s->datalist);
    s->datalist = nullptr;
    delete s->library;
    s->library = nullptr;
}

} // namespace

void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_PREFAB;
    type_desc.size = sizeof(PulsePrefabData);
    type_desc.align = alignof(PulsePrefabData);
    type_desc.destroy = destroy_prefab;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void register_prefab_load_loader(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_PREFAB;
    ld.extensions = "prefab";
    ld.ctor = nullptr;
    ld.dtor = dtor_prefab_load;
    ld.step = step_prefab_load;
    ld.loader_size = sizeof(prefab_load_state);
    ld.loader_align = alignof(prefab_load_state);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_prefab_internal

extern "C" {

PulsePrefabRequest pulse_load_prefab(PulseAppId app, const char* filepath) {
    PulsePrefabRequest result{};
    if (!app || !filepath || !filepath[0]) {
        return result;
    }

    PulseAssetRequest request = pulse_prefab_internal::request_file_load(pulse_get_asset_system(app), PULSE_TYPE_PREFAB, filepath);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }

    result.index = request.index;
    result.generation = request.generation;
    return result;
}

PulsePrefabHandle pulse_prefab_get_handle(PulseAppId app, PulsePrefabRequest request) {
    if (!pulse_prefab_is_ready(app, request)) {
        return PulsePrefabHandle{};
    }
    PulseAssetHandle handle = pulse_asset_system_get_handle(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(handle) ? PulsePrefabHandle{} : PulsePrefabHandle{ handle.index, handle.generation };
}

bool pulse_prefab_is_ready(PulseAppId app, PulsePrefabRequest request) {
    return pulse_asset_system_is_ready(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
}

bool pulse_prefab_is_alive(PulseAppId app, PulsePrefabRequest request) {
    return pulse_asset_system_is_alive(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
}

ecs_entity_t pulse_prefab_get_root(PulseAppId app, PulsePrefabHandle prefab) {
    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(pulse_get_asset_system(app), pulse_prefab_to_handle(prefab), &ptr, nullptr)) {
        return 0;
    }
    return static_cast<PulsePrefabData*>(ptr)->root;
}

ecs_entity_t pulse_prefab_instantiate(PulseAppId app, PulsePrefabHandle prefab) {
    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t root = pulse_prefab_get_root(app, prefab);
    if (!world || !root || !ecs_is_alive(world, root)) {
        return 0;
    }

    return ecs_new_w_pair(world, EcsIsA, root);
}

} // extern "C"
