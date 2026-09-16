#include "validate.h"

#include <algorithm>

namespace tablegen {

bool is_scalar_type(std::string_view type) {
    return type == "int" || type == "float" || type == "bool" || type == "string" || type == "ref";
}

namespace {

bool visit_struct(const Library& library, const Schema& schema, std::vector<std::string>& stack, std::vector<std::string>& errors) {
    if (schema.kind == Kind::Enum) {
        return true;
    }
    if (std::find(stack.begin(), stack.end(), schema.name) != stack.end()) {
        errors.push_back(schema.path + ": struct '" + schema.name + "' nests itself");
        return false;
    }
    stack.push_back(schema.name);
    bool ok = true;
    for (const Field& field : schema.fields) {
        if (is_scalar_type(field.type)) {
            continue;
        }
        const Schema* nested = library.find(field.type);
        if (!nested || nested->kind != Kind::Struct) {
            continue;
        }
        if (!visit_struct(library, *nested, stack, errors)) {
            ok = false;
        }
    }
    stack.pop_back();
    return ok;
}

uint32_t align_up(uint32_t value, uint32_t alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1u) / alignment * alignment;
}

bool scalar_layout(const Field& field, uint32_t& size, uint32_t& align) {
    if (field.type == "int" || field.type == "float" || field.type == "ref") {
        size = 8;
        align = 8;
        return true;
    }
    if (field.type == "bool") {
        size = 1;
        align = 1;
        return true;
    }
    if (field.type == "string") {
        size = 16;
        align = 8;
        return true;
    }
    return false;
}

bool compute_struct_layout(const Library& library, const Schema& schema, LayoutSet& out, std::vector<std::string>& errors, std::vector<std::string>& stack);

bool struct_field_layout(const Library& library, const Field& field, LayoutSet& out, std::vector<std::string>& errors, std::vector<std::string>& stack, uint32_t& size, uint32_t& align) {
    if (scalar_layout(field, size, align)) {
        return true;
    }
    const Schema* nested = library.find(field.type);
    if (!nested) {
        return false;
    }
    if (nested->kind == Kind::Enum) {
        size = 16;
        align = 8;
        return true;
    }
    if (!compute_struct_layout(library, *nested, out, errors, stack)) {
        return false;
    }
    const StructLayout& layout = out.structs[nested->name];
    size = layout.size;
    align = layout.align;
    return true;
}

bool compute_struct_layout(const Library& library, const Schema& schema, LayoutSet& out, std::vector<std::string>& errors, std::vector<std::string>& stack) {
    if (out.structs.find(schema.name) != out.structs.end()) {
        return true;
    }
    if (std::find(stack.begin(), stack.end(), schema.name) != stack.end()) {
        errors.push_back(schema.path + ": struct '" + schema.name + "' nests itself");
        return false;
    }
    stack.push_back(schema.name);
    StructLayout layout{};
    layout.name = schema.name;
    uint32_t cursor = 0;
    uint32_t max_align = 1;
    for (const Field& field : schema.fields) {
        uint32_t size = 0;
        uint32_t align = 1;
        if (!struct_field_layout(library, field, out, errors, stack, size, align)) {
            stack.pop_back();
            return false;
        }
        max_align = std::max(max_align, align);
        cursor = align_up(cursor, align);
        layout.offsets[field.name] = cursor;
        cursor += size;
    }
    layout.align = max_align;
    layout.size = std::max(align_up(cursor, max_align), 1u);
    stack.pop_back();
    out.structs[schema.name] = layout;
    return true;
}

} // namespace

bool validate_library(const Library& library, std::vector<std::string>& errors) {
    for (const Schema& schema : library.schemas) {
        if (schema.kind == Kind::Enum) {
            if (schema.values.empty()) {
                errors.push_back(schema.path + ": enum '" + schema.name + "' declares no values");
            }
            for (size_t i = 0; i < schema.values.size(); ++i) {
                if (schema.values[i].empty()) {
                    errors.push_back(schema.path + ": enum '" + schema.name + "' declares an empty value");
                    continue;
                }
                for (size_t j = 0; j < i; ++j) {
                    if (schema.values[j] == schema.values[i]) {
                        errors.push_back(schema.path + ": enum '" + schema.name + "' declares duplicate value '" + schema.values[i] + "'");
                        break;
                    }
                }
            }
            continue;
        }

        if (schema.kind == Kind::Table) {
            const Field& key = schema.fields[static_cast<size_t>(schema.key_index)];
            if (key.type != "string" && key.type != "int") {
                errors.push_back(schema.path + ": key field '" + key.name + "' must be a string or an int");
            }
        }

        for (const Field& field : schema.fields) {
            if (!is_scalar_type(field.type)) {
                const Schema* nested = library.find(field.type);
                if (!nested) {
                    errors.push_back(schema.path + ": field '" + field.name + "' uses unknown type '" + field.type + "'");
                } else if (nested->kind == Kind::Table) {
                    errors.push_back(schema.path + ": field '" + field.name + "' uses table '" + field.type + "'; reference a table with type 'ref'");
                } else if (nested->kind == Kind::Enum) {
                    if (field.has_default && std::find(nested->values.begin(), nested->values.end(), field.default_string) == nested->values.end()) {
                        errors.push_back(schema.path + ": field '" + field.name + "' default '" + field.default_string + "' is not a value of enum '" + nested->name + "'");
                    }
                } else if (field.has_default) {
                    errors.push_back(schema.path + ": field '" + field.name + "' cannot have a default value");
                }
                if (field.has_min || field.has_max) {
                    errors.push_back(schema.path + ": field '" + field.name + "' has 'min' or 'max' but its type is '" + field.type + "'");
                }
                if (!field.values.empty()) {
                    errors.push_back(schema.path + ": field '" + field.name + "' has 'values' but its type is '" + field.type + "'");
                }
                if (!field.ref.empty()) {
                    errors.push_back(schema.path + ": field '" + field.name + "' has a 'ref' attribute but its type is '" + field.type + "'");
                }
                if (field.is_key) {
                    errors.push_back(schema.path + ": key field '" + field.name + "' must be a string or an int");
                }
                continue;
            }

            if (field.type == "ref") {
                if (field.ref.empty()) {
                    errors.push_back(schema.path + ": field '" + field.name + "' has type 'ref' but no 'ref' attribute");
                    continue;
                }
                const Schema* target = library.find(field.ref);
                if (!target) {
                    errors.push_back(schema.path + ": field '" + field.name + "' references unknown schema '" + field.ref + "'");
                } else if (target->kind != Kind::Table) {
                    errors.push_back(schema.path + ": field '" + field.name + "' references '" + field.ref + "', which is not a table");
                }
            } else if (!field.ref.empty()) {
                errors.push_back(schema.path + ": field '" + field.name + "' has a 'ref' attribute but its type is '" + field.type + "'");
            }

            if (!field.values.empty() && field.type != "string") {
                errors.push_back(schema.path + ": field '" + field.name + "' has 'values' but its type is '" + field.type + "'");
            }
            if ((field.has_min || field.has_max) && field.type != "int" && field.type != "float") {
                errors.push_back(schema.path + ": field '" + field.name + "' has 'min' or 'max' but its type is '" + field.type + "'");
            }
            if (field.is_key) {
                if (field.type != "string" && field.type != "int") {
                    errors.push_back(schema.path + ": key field '" + field.name + "' must be a string or an int");
                }
                if (field.has_default) {
                    errors.push_back(schema.path + ": key field '" + field.name + "' cannot have a default");
                }
                if (!field.values.empty()) {
                    errors.push_back(schema.path + ": key field '" + field.name + "' cannot have 'values'");
                }
            }
            if (field.has_default) {
                if (field.type == "int" && field.default_text.find('.') != std::string::npos) {
                    errors.push_back(schema.path + ": field '" + field.name + "' has a non-integer default");
                }
                if (field.type == "bool" && field.default_text != "true" && field.default_text != "false") {
                    errors.push_back(schema.path + ": field '" + field.name + "' has a non-bool default");
                }
            }
        }
    }

    for (const Schema& schema : library.schemas) {
        if (schema.kind != Kind::Enum) {
            std::vector<std::string> stack{};
            visit_struct(library, schema, stack, errors);
        }
    }

    return errors.empty();
}

bool compute_layouts(const Library& library, LayoutSet& out, std::vector<std::string>& errors) {
    for (const Schema& schema : library.schemas) {
        if (schema.kind == Kind::Enum) {
            continue;
        }
        if (schema.kind == Kind::Struct) {
            std::vector<std::string> stack{};
            if (!compute_struct_layout(library, schema, out, errors, stack)) {
                return false;
            }
            continue;
        }

        uint32_t cursor = 0;
        uint32_t max_align = 1;
        uint32_t key_offset = 0;
        for (const Field& field : schema.fields) {
            uint32_t size = 0;
            uint32_t align = 1;
            std::vector<std::string> stack{};
            if (!struct_field_layout(library, field, out, errors, stack, size, align)) {
                errors.push_back(schema.path + ": field '" + field.name + "' cannot be laid out");
                return false;
            }
            max_align = std::max(max_align, align);
            cursor = align_up(cursor, align);
            if (field.is_key) {
                key_offset = cursor;
            }
            cursor += size;
        }
        out.table_rows[schema.name] = std::max(align_up(cursor, max_align), 1u);
        out.table_aligns[schema.name] = max_align;
        out.table_keys[schema.name] = key_offset;
    }
    return true;
}

} // namespace tablegen
