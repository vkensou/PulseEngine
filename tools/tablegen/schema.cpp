#include "schema.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace tablegen {

namespace {

std::string read_file(const std::string& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {};
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

std::string base_name(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string file = slash == std::string::npos ? path : path.substr(slash + 1);
    size_t dot = file.find_last_of('.');
    return dot == std::string::npos ? file : file.substr(0, dot);
}

std::string scalar_text(const PulseDatalist* node) {
    switch (pulse_datalist_get_type(node, nullptr)) {
    case PULSE_DATALIST_TYPE_BOOL:
        return pulse_datalist_get_bool(node, nullptr, false) ? "true" : "false";
    case PULSE_DATALIST_TYPE_INT: {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "%lld", static_cast<long long>(pulse_datalist_get_int(node, nullptr, 0)));
        return buffer;
    }
    case PULSE_DATALIST_TYPE_DOUBLE: {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.17g", pulse_datalist_get_double(node, nullptr, 0.0));
        return buffer;
    }
    case PULSE_DATALIST_TYPE_STRING: {
        const char* text = pulse_datalist_get_string(node, nullptr, "");
        return text ? text : "";
    }
    default:
        return {};
    }
}

bool parse_values(const PulseDatalist* node, const std::string& what, std::vector<std::string>& out, std::vector<std::string>& errors, const std::string& path) {
    for (size_t i = 0; i < pulse_datalist_count(node); ++i) {
        const PulseDatalist* item = pulse_datalist_get(node, i);
        if (pulse_datalist_get_type(item, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            errors.push_back(path + ":" + std::to_string(pulse_datalist_line(item)) + ": " + what + " needs strings");
            return false;
        }
        out.push_back(pulse_datalist_get_string(item, nullptr, ""));
    }
    return true;
}

bool parse_field(const PulseDatalist* node, const std::string& name, Field& out, std::vector<std::string>& errors, const std::string& path) {
    out.name = name;
    out.line = pulse_datalist_line(node);
    for (size_t i = 0; i < pulse_datalist_object_count(node); ++i) {
        const char* key = pulse_datalist_object_key(node, i);
        const PulseDatalist* value = pulse_datalist_object_value(node, i);
        if (!key) {
            continue;
        }
        std::string attribute(key);
        if (attribute == "type") {
            if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute 'type' needs a string");
                return false;
            }
            out.type = pulse_datalist_get_string(value, nullptr, "");
        } else if (attribute == "key") {
            if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_BOOL) {
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute 'key' needs a bool");
                return false;
            }
            out.is_key = pulse_datalist_get_bool(value, nullptr, false);
        } else if (attribute == "default") {
            out.has_default = true;
            switch (pulse_datalist_get_type(value, nullptr)) {
            case PULSE_DATALIST_TYPE_BOOL:
                out.default_bool = pulse_datalist_get_bool(value, nullptr, false);
                out.default_text = scalar_text(value);
                break;
            case PULSE_DATALIST_TYPE_INT:
                out.default_int = pulse_datalist_get_int(value, nullptr, 0);
                out.default_float = static_cast<double>(out.default_int);
                out.default_text = scalar_text(value);
                break;
            case PULSE_DATALIST_TYPE_DOUBLE:
                out.default_float = pulse_datalist_get_double(value, nullptr, 0.0);
                out.default_int = static_cast<int64_t>(out.default_float);
                out.default_text = scalar_text(value);
                break;
            case PULSE_DATALIST_TYPE_STRING:
                out.default_string = pulse_datalist_get_string(value, nullptr, "");
                out.default_text = out.default_string;
                break;
            default:
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute 'default' needs a scalar");
                return false;
            }
        } else if (attribute == "min" || attribute == "max") {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_INT && value_type != PULSE_DATALIST_TYPE_DOUBLE) {
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute '" + attribute + "' needs a number");
                return false;
            }
            double number = pulse_datalist_get_double(value, nullptr, 0.0);
            if (attribute == "min") {
                out.has_min = true;
                out.min_value = number;
            } else {
                out.has_max = true;
                out.max_value = number;
            }
        } else if (attribute == "values") {
            if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_LIST) {
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute 'values' needs a list");
                return false;
            }
            if (!parse_values(value, "field '" + name + "' attribute 'values'", out.values, errors, path)) {
                return false;
            }
        } else if (attribute == "ref") {
            if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
                errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' attribute 'ref' needs a string");
                return false;
            }
            out.ref = pulse_datalist_get_string(value, nullptr, "");
        } else {
            errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + name + "' has unknown attribute '" + attribute + "'");
            return false;
        }
    }
    if (out.type.empty()) {
        errors.push_back(path + ":" + std::to_string(out.line) + ": field '" + name + "' is missing the required 'type' attribute");
        return false;
    }
    return true;
}

} // namespace

const Schema* Library::find(std::string_view name) const {
    for (const Schema& schema : schemas) {
        if (schema.name == name) {
            return &schema;
        }
    }
    return nullptr;
}

bool parse_schema_file(const std::string& path, Schema& out, std::vector<std::string>& errors) {
    std::string text = read_file(path);
    if (text.empty()) {
        errors.push_back(path + ": cannot read schema file");
        return false;
    }
    PulseDatalist* root = pulse_datalist_create_from_text(text.c_str(), text.size());
    if (!root) {
        errors.push_back(path + ": " + (pulse_datalist_last_error() ? pulse_datalist_last_error() : "parse failed"));
        return false;
    }

    out.name = base_name(path);
    out.path = path;
    bool ok = true;
    bool has_values = false;
    for (size_t i = 0; i < pulse_datalist_object_count(root) && ok; ++i) {
        const char* key = pulse_datalist_object_key(root, i);
        const PulseDatalist* value = pulse_datalist_object_value(root, i);
        if (!key) {
            continue;
        }
        if (std::strcmp(key, "values") == 0 && pulse_datalist_get_type(value, nullptr) == PULSE_DATALIST_TYPE_LIST) {
            has_values = true;
            if (!parse_values(value, "top-level 'values'", out.values, errors, path)) {
                ok = false;
            }
            continue;
        }
        if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            errors.push_back(path + ":" + std::to_string(pulse_datalist_line(value)) + ": field '" + std::string(key) + "' needs a table of attributes");
            ok = false;
            break;
        }
        Field field{};
        if (!parse_field(value, key, field, errors, path)) {
            ok = false;
            break;
        }
        out.fields.push_back(std::move(field));
    }
    pulse_datalist_release(root);

    if (!ok) {
        return false;
    }
    if (has_values && !out.fields.empty()) {
        errors.push_back(path + ": schema declares both a top-level 'values' list and fields");
        return false;
    }
    if (has_values) {
        out.kind = Kind::Enum;
        return true;
    }
    if (out.fields.empty()) {
        errors.push_back(path + ": schema declares no fields");
        return false;
    }

    size_t key_count = 0;
    for (size_t i = 0; i < out.fields.size(); ++i) {
        if (out.fields[i].is_key) {
            out.key_index = static_cast<int>(i);
            key_count += 1;
        }
    }
    if (key_count > 1) {
        errors.push_back(path + ": schema declares " + std::to_string(key_count) + " key fields, at most one is allowed");
        return false;
    }
    out.kind = key_count == 1 ? Kind::Table : Kind::Struct;
    return true;
}

bool load_library(const std::vector<std::string>& paths, Library& out) {
    for (const std::string& path : paths) {
        Schema schema{};
        if (!parse_schema_file(path, schema, out.errors)) {
            continue;
        }
        if (out.find(schema.name)) {
            out.errors.push_back(path + ": duplicate schema name '" + schema.name + "'");
            continue;
        }
        out.schemas.push_back(std::move(schema));
    }
    return out.errors.empty();
}

} // namespace tablegen
