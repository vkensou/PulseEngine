#include "codegen.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace tablegen {

namespace {

const char* kHeaderGuard = "PULSE_TABLES_GENERATED_H";

struct Scope {
    const Schema* schema = nullptr;
    uint32_t id = 0;
    std::vector<uint32_t> offsets{};
    uint32_t size = 0;
    uint32_t align = 1;
};

struct TablePlan {
    const Schema* schema = nullptr;
    uint32_t root = 0;
    std::vector<Scope> scopes{};
    std::map<std::string, uint32_t> scope_of{};
    std::vector<const Schema*> struct_order{};
};

std::string pascal_case(std::string_view text) {
    std::string result;
    bool upper = true;
    for (char c : text) {
        if (c == '_') {
            upper = true;
            continue;
        }
        if (upper && c >= 'a' && c <= 'z') {
            result.push_back(static_cast<char>(c - 'a' + 'A'));
        } else {
            result.push_back(c);
        }
        upper = false;
    }
    return result;
}

std::string row_type_name(const Schema& schema) {
    return "Pulse" + pascal_case(schema.name) + "Row";
}

std::string nested_type_name(const Schema& schema) {
    return "Pulse" + pascal_case(schema.name) + (schema.kind == Kind::Table ? "Row" : "");
}

std::string escape(std::string_view text) {
    std::string result;
    for (char c : text) {
        if (c == '"' || c == '\\') {
            result.push_back('\\');
        }
        result.push_back(c);
    }
    return result;
}

std::string ensure_decimal(std::string text) {
    if (text.find('.') == std::string::npos && text.find('e') == std::string::npos && text.find('E') == std::string::npos && text.find("inf") == std::string::npos && text.find("nan") == std::string::npos) {
        text.append(".0");
    }
    return text;
}

std::string format_double(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    return ensure_decimal(buffer);
}

std::string format_double_short(double value) {
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.17g", value);
    for (int digits = 1; digits < 17; ++digits) {
        char candidate[64];
        std::snprintf(candidate, sizeof(candidate), "%.*g", digits, value);
        if (std::strtod(candidate, nullptr) != value || std::strpbrk(candidate, "eE") != nullptr) {
            continue;
        }
        std::snprintf(buffer, sizeof(buffer), "%s", candidate);
        break;
    }
    return ensure_decimal(buffer);
}

std::string to_crlf(std::string text) {
    std::string result;
    result.reserve(text.size() + text.size() / 16);
    for (char c : text) {
        if (c == '\n') {
            result.push_back('\r');
        }
        result.push_back(c);
    }
    return result;
}

std::string pad(int depth) {
    return std::string(static_cast<size_t>(depth) * 4, ' ');
}

uint32_t align_up(uint32_t value, uint32_t alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1u) / alignment * alignment;
}

std::string model_type(const Library& library, const std::string& module, const Field& field);

std::string cpp_type(const Library& library, const Field& field) {
    if (field.type == "int") {
        return "int64_t";
    }
    if (field.type == "float") {
        return "double";
    }
    if (field.type == "bool") {
        return "bool";
    }
    if (field.type == "string") {
        return "std::string_view";
    }
    if (field.type == "ref") {
        const Schema* target = library.find(field.ref);
        return target ? "const " + row_type_name(*target) + "*" : "nullptr_t";
    }
    const Schema* schema = library.find(field.type);
    if (!schema) {
        return "void";
    }
    if (schema->kind == Kind::Enum) {
        return "std::string_view";
    }
    if (schema->kind == Kind::Struct) {
        return "Pulse" + pascal_case(schema->name);
    }
    return "const " + row_type_name(*schema) + "*";
}

std::string model_type(const Library& library, const std::string& module, const Field& field) {
    if (field.type == "ref") {
        const Schema* target = library.find(field.ref);
        return target ? "const ::" + module + "::" + row_type_name(*target) + "*" : "nullptr_t";
    }
    std::string type = cpp_type(library, field);
    if (type.rfind("std::", 0) == 0 || type == "int64_t" || type == "double" || type == "bool" || type == "nullptr_t") {
        return type;
    }
    return "::" + module + "::" + type;
}

bool struct_is_defaultable(const Library& library, const Schema& schema, std::set<std::string>& visiting) {
    if (visiting.find(schema.name) != visiting.end()) {
        return false;
    }
    visiting.insert(schema.name);
    bool result = true;
    for (const Field& field : schema.fields) {
        if (is_scalar_type(field.type)) {
            result = result && field.has_default;
            continue;
        }
        const Schema* nested = library.find(field.type);
        if (!nested) {
            result = false;
            continue;
        }
        if (nested->kind == Kind::Enum) {
            result = result && field.has_default;
            continue;
        }
        result = result && struct_is_defaultable(library, *nested, visiting);
    }
    visiting.erase(schema.name);
    return result;
}

bool column_is_defaultable(const Library& library, const Field& field, std::map<std::string, bool>& cache) {
    if (is_scalar_type(field.type)) {
        return field.has_default;
    }
    const Schema* nested = library.find(field.type);
    if (!nested) {
        return false;
    }
    if (nested->kind == Kind::Enum) {
        return field.has_default;
    }
    auto it = cache.find(nested->name);
    if (it != cache.end()) {
        return it->second;
    }
    std::set<std::string> visiting{};
    bool value = struct_is_defaultable(library, *nested, visiting);
    cache[nested->name] = value;
    return value;
}

bool field_size_align(const Library& library, const LayoutSet& layouts, const Field& field, uint32_t& size, uint32_t& align) {
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
    const Schema* nested = library.find(field.type);
    if (!nested) {
        return false;
    }
    if (nested->kind == Kind::Enum) {
        size = 16;
        align = 8;
        return true;
    }
    auto it = layouts.structs.find(nested->name);
    if (it == layouts.structs.end()) {
        return false;
    }
    size = it->second.size;
    align = it->second.align;
    return true;
}

class Generator {
public:
    Generator(const Library& library, const LayoutSet& layouts, std::string module_name, std::string header_name, bool emit_das)
        : library_(library), layouts_(layouts), module_(std::move(module_name)), header_name_(std::move(header_name)), emit_das_(emit_das) {
    }

    bool build(std::vector<std::string>& errors) {
        for (const Schema& schema : library_.schemas) {
            if (schema.kind != Kind::Table) {
                continue;
            }
            TablePlan plan{};
            if (!build_plan(schema, plan)) {
                errors.push_back(schema.path + ": cannot lay out table '" + schema.name + "'");
                return false;
            }
            if (emit_das_) {
                std::string collision{};
                if (!check_das_names(schema, collision)) {
                    errors.push_back(schema.path + ": field '" + collision + "' collides with a generated table function of table '" + schema.name + "'");
                    return false;
                }
            }
            plans_.push_back(std::move(plan));
        }
        return true;
    }

    std::string header_text() {
        emit_header();
        return header_.str();
    }

    std::string source_text() {
        emit_source();
        return source_.str();
    }

    std::string das_text(std::string module_name) {
        das_module_ = std::move(module_name);
        emit_das();
        std::string text = das_.str();
        if (!text.empty() && text.back() == '\n') {
            text.pop_back();
        }
        return text;
    }

private:
    bool build_plan(const Schema& schema, TablePlan& plan) {
        plan.schema = &schema;
        Scope root{};
        root.schema = &schema;
        root.id = 0;
        root.align = layouts_.table_aligns.at(schema.name);
        root.size = layouts_.table_rows.at(schema.name);
        uint32_t cursor = 0;
        for (const Field& field : schema.fields) {
            uint32_t size = 0;
            uint32_t align = 1;
            if (!field_size_align(library_, layouts_, field, size, align)) {
                return false;
            }
            cursor = align_up(cursor, align);
            root.offsets.push_back(cursor);
            cursor += size;
        }
        plan.scopes.push_back(root);
        plan.scope_of[schema.name] = 0;
        for (const Field& field : schema.fields) {
            if (!collect_scopes(field, plan)) {
                return false;
            }
        }
        plan.root = 0;
        return true;
    }

    bool collect_scopes(const Field& field, TablePlan& plan) {
        if (is_scalar_type(field.type)) {
            return true;
        }
        const Schema* nested = library_.find(field.type);
        if (!nested || nested->kind != Kind::Struct) {
            return true;
        }
        if (plan.scope_of.find(nested->name) != plan.scope_of.end()) {
            return true;
        }
        Scope scope{};
        scope.schema = nested;
        scope.id = static_cast<uint32_t>(plan.scopes.size());
        const StructLayout& layout = layouts_.structs.at(nested->name);
        scope.align = layout.align;
        scope.size = layout.size;
        for (const Field& sub : nested->fields) {
            scope.offsets.push_back(layout.offsets.at(sub.name));
        }
        plan.scope_of[nested->name] = scope.id;
        plan.struct_order.push_back(nested);
        plan.scopes.push_back(scope);
        for (const Field& sub : nested->fields) {
            if (!collect_scopes(sub, plan)) {
                return false;
            }
        }
        return true;
    }

    std::string table_type_name(const Schema& schema) const {
        return nested_type_name(schema) + "Table";
    }

    std::string struct_type_name(const TablePlan& plan) const {
        return "pulse_table_" + plan.schema->name;
    }

    std::string columns_name(const TablePlan& plan, uint32_t scope) const {
        return struct_type_name(plan) + "_columns_" + std::to_string(scope);
    }

    std::string column_ref(const TablePlan& plan, uint32_t scope, size_t index) const {
        return columns_name(plan, scope) + "[" + std::to_string(index) + "]";
    }

    std::string struct_desc_name(const TablePlan& plan, size_t index) const {
        return struct_type_name(plan) + "_struct_" + std::to_string(index);
    }

    std::string schema_name(const TablePlan& plan) const {
        return struct_type_name(plan) + "_schema";
    }

    std::string fill_name(const TablePlan& plan, uint32_t scope) const {
        return struct_type_name(plan) + "_fill_" + std::to_string(scope);
    }

    void collect_order(const Schema& schema, std::vector<const Schema*>& out, std::set<std::string>& done) const {
        if (!done.insert(schema.name).second) {
            return;
        }
        for (const Field& field : schema.fields) {
            const Schema* nested = library_.find(field.type);
            if (nested && nested->kind == Kind::Struct) {
                collect_order(*nested, out, done);
            }
        }
        out.push_back(&schema);
    }

    std::vector<const Schema*> ordered_schemas() const {
        std::vector<const Schema*> ordered{};
        std::set<std::string> done{};
        for (const Schema& schema : library_.schemas) {
            if (schema.kind != Kind::Enum) {
                collect_order(schema, ordered, done);
            }
        }
        return ordered;
    }

    void emit_header() {
        header_ << "#pragma once\n\n";
        header_ << "#ifndef " << kHeaderGuard << "\n#define " << kHeaderGuard << "\n\n";
        header_ << "#include \"pulse_datatable.h\"\n\n";
        header_ << "#include <array>\n#include <cstdint>\n#include <string_view>\n\n";

        const std::vector<const Schema*> ordered = ordered_schemas();

        header_ << "namespace " << module_ << "\n{\n\n";

        for (const Schema* schema : ordered) {
            header_ << "struct " << nested_type_name(*schema) << ";\n";
        }
        header_ << "\n";

        for (const Schema* schema : ordered) {
            const uint32_t align = schema->kind == Kind::Table ? layouts_.table_aligns.at(schema->name) : layouts_.structs.at(schema->name).align;
            const uint32_t size = schema->kind == Kind::Table ? layouts_.table_rows.at(schema->name) : layouts_.structs.at(schema->name).size;
            header_ << "struct alignas(" << align << ") " << nested_type_name(*schema) << "\n{\n";
            for (const Field& field : schema->fields) {
                header_ << "    " << cpp_type(library_, field) << " " << field.name << ";\n";
            }
            header_ << "};\n";
            header_ << "static_assert(sizeof(" << nested_type_name(*schema) << ") == " << size << ", \"" << schema->name << " layout mismatch\");\n\n";
        }

        for (const Schema& schema : library_.schemas) {
            if (schema.kind != Kind::Table) {
                continue;
            }
            const std::string row = row_type_name(schema);
            header_ << "struct " << table_type_name(schema) << "\n{\n";
            header_ << "    static PulseAssetRequest Load(PulseAppId app, const char* path = nullptr);\n";
            header_ << "    static bool IsReady(PulseAppId app);\n";
            header_ << "    static const char* GetError(PulseAppId app);\n";
            header_ << "    static const " << row << "* Rows(PulseAppId app, uint32_t& out_count);\n";
            if (schema.fields[static_cast<size_t>(schema.key_index)].type == "int") {
                header_ << "    static const " << row << "* GetRow(PulseAppId app, int64_t key);\n";
            } else {
                header_ << "    static const " << row << "* GetRow(PulseAppId app, const char* key);\n";
            }
            header_ << "    static const char* DefaultPath();\n";
            header_ << "};\n\n";
        }

        header_ << "EPulseResult RegisterSchemas(PulseDataTableSystemId system);\n\n";
        header_ << "} // namespace " << module_ << "\n\n";
        header_ << "#endif // " << kHeaderGuard << "\n";
    }

    void emit_source() {
        source_ << "#include \"" << header_name_ << "\"\n\n";
        source_ << "#include \"pulse_datatable_schema.h\"\n\n";
        source_ << "#include <array>\n\n";
        source_ << "namespace\n{\n\n";

        for (const TablePlan& plan : plans_) {
            source_ << "constexpr const PulseDataTableSchemaDesc* " << schema_name(plan) << "_ptr();\n";
        }
        source_ << "\n";
        for (const TablePlan& plan : plans_) {
            for (const Scope& scope : plan.scopes) {
                emit_column_descriptors(plan, scope);
            }
        }
        for (const TablePlan& plan : plans_) {
            std::set<uint32_t> declared{};
            emit_declaration(plan, plan.scopes[plan.root], 0, declared);
        }
        source_ << "\n";
        for (const TablePlan& plan : plans_) {
            emit_struct_descriptors(plan);
        }
        for (const TablePlan& plan : plans_) {
            emit_schema_descriptor(plan);
        }
        for (const TablePlan& plan : plans_) {
            source_ << "constexpr const PulseDataTableSchemaDesc* " << schema_name(plan) << "_ptr() { return &" << schema_name(plan) << "; }\n";
        }
        source_ << "\n";
        for (const TablePlan& plan : plans_) {
            for (const Scope& scope : plan.scopes) {
                emit_fill_function(plan, scope);
            }
        }

        source_ << "\n} // namespace\n\n";
        source_ << "namespace " << module_ << "\n{\n\n";
        for (const TablePlan& plan : plans_) {
            source_ << "const char* " << struct_type_name(plan) << "_path = nullptr;\n";
        }
        source_ << "\n";
        for (const TablePlan& plan : plans_) {
            emit_table_helpers(plan);
        }
        emit_register();
        source_ << "} // namespace " << module_ << "\n";
    }

    void emit_column_descriptors(const TablePlan& plan, const Scope& scope) {
        const Schema& schema = *scope.schema;
        for (size_t i = 0; i < schema.fields.size(); ++i) {
            const Field& field = schema.fields[i];
            source_ << "const PulseDataTableColumnDesc " << columns_name(plan, scope.id) << "_" << i << " = pulse::datatable::ColumnDescBuilder{}"
                    << ".name(\"" << escape(field.name) << "\")"
                    << ".column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<" << model_type(library_, module_, field) << ">>::value)"
                    << ".offset(" << scope.offsets[i] << ")";
            if (field.has_min) {
                source_ << ".min(" << format_double(field.min_value) << ")";
            }
            if (field.has_max) {
                source_ << ".max(" << format_double(field.max_value) << ")";
            }
            if (field.has_default && field.type == "int") {
                source_ << ".default_int(INT64_C(" << static_cast<long long>(field.default_int) << "))";
            }
            if (field.has_default && field.type == "float") {
                source_ << ".default_float(" << format_double(field.default_float) << ")";
            }
            if (field.has_default && field.type == "bool") {
                source_ << ".default_bool(" << (field.default_bool ? "true" : "false") << ")";
            }
            if (field.has_default && field.type == "string") {
                source_ << ".default_string(\"" << escape(field.default_string) << "\")";
            }
            if (field.type == "ref") {
                source_ << ".ref_name(\"" << escape(field.ref) << "\")";
            }
            if (!is_scalar_type(field.type)) {
                const Schema* nested = library_.find(field.type);
                if (nested && nested->kind == Kind::Struct) {
                    source_ << ".struct_name(\"" << escape(nested->name) << "\")";
                    source_ << ".column_type(PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT)";
                } else if (nested) {
                    source_ << ".enum_name(\"" << escape(nested->name) << "\")";
                    source_ << ".column_type(PULSE_DATA_TABLE_COLUMN_TYPE_ENUM)";
                }
            } else if (!field.values.empty()) {
                source_ << ".enum_name(\"" << escape(field.name) << "_values\")";
                source_ << ".column_type(PULSE_DATA_TABLE_COLUMN_TYPE_ENUM)";
            }
            source_ << ".build();\n";
        }
        source_ << "const std::array<PulseDataTableColumnDesc, " << schema.fields.size() << "> " << columns_name(plan, scope.id) << " = {";
        for (size_t i = 0; i < schema.fields.size(); ++i) {
            source_ << (i == 0 ? " " : ", ") << columns_name(plan, scope.id) << "_" << i;
        }
        source_ << " };\n\n";
    }

    void emit_struct_descriptors(const TablePlan& plan) {
        for (size_t index = 0; index < plan.struct_order.size(); ++index) {
            const Schema& schema = *plan.struct_order[index];
            const uint32_t scope = plan.scope_of.at(schema.name);
            const StructLayout& layout = layouts_.structs.at(schema.name);
            source_ << "const PulseDataTableStructDesc " << struct_desc_name(plan, index)
                    << "{\"" << escape(schema.name) << "\", " << layout.size << "u, " << layout.align << "u, "
                    << columns_name(plan, scope) << ".data(), " << schema.fields.size() << "u};\n";
        }
        if (!plan.struct_order.empty()) {
            source_ << "\n";
        }
    }

    struct InlineEnum {
        std::string name{};
        const Field* field = nullptr;
    };

    std::vector<InlineEnum> inline_enums(const TablePlan& plan) const {
        std::vector<InlineEnum> result{};
        std::set<std::string> seen{};
        for (const Scope& scope : plan.scopes) {
            for (const Field& field : scope.schema->fields) {
                if (is_scalar_type(field.type) && !field.values.empty()) {
                    std::string name = field.name + "_values";
                    if (seen.insert(name).second) {
                        result.push_back(InlineEnum{name, &field});
                    }
                }
            }
        }
        return result;
    }

    void emit_enum_descriptors(const TablePlan& plan) {
        std::set<std::string> emitted{};
        for (const Scope& scope : plan.scopes) {
            for (const Field& field : scope.schema->fields) {
                if (is_scalar_type(field.type)) {
                    continue;
                }
                const Schema* nested = library_.find(field.type);
                if (!nested || nested->kind != Kind::Enum) {
                    continue;
                }
                if (!emitted.insert(nested->name).second) {
                    continue;
                }
                source_ << "const char* " << struct_type_name(plan) << "_enum_names_" << nested->name << "[] = {\n";
                for (const std::string& value : nested->values) {
                    source_ << "    \"" << escape(value) << "\",\n";
                }
                source_ << "};\n";
                source_ << "const PulseDataTableEnumDesc " << struct_type_name(plan) << "_enum_" << nested->name
                        << "{\"" << escape(nested->name) << "\", " << struct_type_name(plan) << "_enum_names_" << nested->name << ", " << nested->values.size() << "u};\n\n";
            }
        }
        for (const InlineEnum& entry : inline_enums(plan)) {
            source_ << "const char* " << struct_type_name(plan) << "_values_" << entry.name << "[] = {\n";
            for (const std::string& value : entry.field->values) {
                source_ << "    \"" << escape(value) << "\",\n";
            }
            source_ << "};\n";
            source_ << "const PulseDataTableEnumDesc " << struct_type_name(plan) << "_enum_" << entry.name
                    << "{\"" << escape(entry.name) << "\", " << struct_type_name(plan) << "_values_" << entry.name << ", " << static_cast<uint32_t>(entry.field->values.size()) << "u};\n\n";
        }
        if (!plan.struct_order.empty() || !inline_enums(plan).empty()) {
            source_ << "\n";
        }
    }

    void emit_schema_descriptor(const TablePlan& plan) {
        const Schema& schema = *plan.schema;
        emit_enum_descriptors(plan);
        const std::vector<InlineEnum> values = inline_enums(plan);
        std::set<std::string> enum_names{};
        for (const Scope& scope : plan.scopes) {
            for (const Field& field : scope.schema->fields) {
                if (is_scalar_type(field.type)) {
                    continue;
                }
                const Schema* nested = library_.find(field.type);
                if (nested && nested->kind == Kind::Enum) {
                    enum_names.insert(nested->name);
                }
            }
        }
        for (const InlineEnum& entry : values) {
            enum_names.insert(entry.name);
        }
        if (!plan.struct_order.empty()) {
            source_ << "const PulseDataTableStructDesc " << struct_type_name(plan) << "_structs[] = {\n";
            for (size_t index = 0; index < plan.struct_order.size(); ++index) {
                source_ << "    " << struct_desc_name(plan, index) << ",\n";
            }
            source_ << "};\n\n";
        }
        if (!enum_names.empty()) {
            source_ << "const PulseDataTableEnumDesc " << struct_type_name(plan) << "_enums[] = {\n";
            for (const std::string& name : enum_names) {
                source_ << "    " << struct_type_name(plan) << "_enum_" << name << ",\n";
            }
            source_ << "};\n\n";
        }
        source_ << "constexpr const PulseDataTableSchemaDesc* " << schema_name(plan) << "_ptr();\n";
        source_ << "const PulseDataTableSchemaDesc " << schema_name(plan) << "{\n";
        source_ << "    sizeof(PulseDataTableSchemaDesc),\n";
        source_ << "    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,\n";
        source_ << "    \"" << escape(schema.name) << "\",\n";
        source_ << "    " << columns_name(plan, plan.root) << ".data(),\n";
        source_ << "    " << schema.fields.size() << "u,\n";
        source_ << "    " << (plan.struct_order.empty() ? std::string("nullptr") : struct_type_name(plan) + "_structs") << ",\n";
        source_ << "    " << plan.struct_order.size() << "u,\n";
        source_ << "    " << (enum_names.empty() ? std::string("nullptr") : struct_type_name(plan) + "_enums") << ",\n";
        source_ << "    " << enum_names.size() << "u,\n";
        source_ << "    " << schema.key_index << "u,\n";
        source_ << "    " << (schema.fields[static_cast<size_t>(schema.key_index)].type == "int" ? "true" : "false") << ",\n";
        source_ << "    " << fill_name(plan, plan.root) << "\n";
        source_ << "};\n\n";
    }

    std::string nested_default_expression(const Schema& schema) const {
        std::string result = "::" + module_ + "::Pulse" + pascal_case(schema.name) + "{";
        for (size_t i = 0; i < schema.fields.size(); ++i) {
            const Field& field = schema.fields[i];
            if (i > 0) {
                result.append(", ");
            }
            result.append(field_default_expression(schema, field));
        }
        result.append("}");
        return result;
    }

    std::string field_default_expression(const Schema& schema, const Field& field) const {
        (void)schema;
        if (field.type == "bool") {
            return field.default_bool ? "true" : "false";
        }
        if (field.type == "int") {
            return "INT64_C(" + std::to_string(static_cast<long long>(field.default_int)) + ")";
        }
        if (field.type == "float") {
            return format_double(field.default_float) + "F";
        }
        if (field.type == "string") {
            return "std::string_view(\"" + escape(field.default_string) + "\")";
        }
        if (field.type == "ref") {
            return "nullptr";
        }
        const Schema* nested = library_.find(field.type);
        if (!nested) {
            return "{}";
        }
        if (nested->kind == Kind::Enum) {
            return "std::string_view(\"" + escape(field.default_string) + "\")";
        }
        return nested_default_expression(*nested);
    }

    void emit_fill_function(const TablePlan& plan, const Scope& scope) {
        const Schema& schema = *scope.schema;
        const std::string row_owner = "::" + module_ + "::";
        source_ << "bool " << fill_name(plan, scope.id) << "(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {\n";
        source_ << "    const PulseDataTableSchemaDesc* owner = " << schema_name(plan) << "_ptr();\n";
        source_ << "    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);\n";
        source_ << "    (void)vault;\n";
        source_ << "    (void)context;\n";
        source_ << "    auto* row = static_cast<" << row_owner << nested_type_name(schema) << "*>(out);\n";
        for (size_t i = 0; i < schema.fields.size(); ++i) {
            emit_field_fill(plan, scope, i, "row->" + schema.fields[i].name, 1);
        }
        source_ << "    (void)error_line;\n";
        source_ << "    (void)node;\n";
        source_ << "    return true;\n";
        source_ << "}\n\n";
    }

    void emit_declaration(const TablePlan& plan, const Scope& scope, int depth, std::set<uint32_t>& declared) {
        if (declared.insert(scope.id).second) {
            source_ << pad(depth) << "bool " << fill_name(plan, scope.id) << "(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);\n";
        }
        for (const Field& field : scope.schema->fields) {
            if (is_scalar_type(field.type)) {
                continue;
            }
            const Schema* nested = library_.find(field.type);
            if (!nested || nested->kind != Kind::Struct) {
                continue;
            }
            emit_declaration(plan, plan.scopes[plan.scope_of.at(nested->name)], depth, declared);
        }
    }

    void emit_field_fill(const TablePlan& plan, const Scope& scope, size_t index, const std::string& target, int depth) {
        const Field& field = scope.schema->fields[index];
        const std::string ind = pad(depth);
        const std::string ind_inner = pad(depth + 1);
        const std::string ind_leaf = pad(depth + 2);
        const std::string ind_leaf2 = pad(depth + 3);
        const std::string column = column_ref(plan, scope.id, index);
        const bool defaultable = column_is_defaultable(library_, field, defaultable_cache_);

        source_ << ind << "{\n";
        source_ << ind_inner << "const PulseDatalist* value = pulse_datalist_value(node, \"" << escape(field.name) << "\");\n";
        if (!field.has_default && !defaultable) {
            source_ << ind_inner << "if (!value) {\n";
            source_ << ind_leaf << "*error_line = pulse_datalist_line(node);\n";
            source_ << ind_leaf << "*error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;\n";
            source_ << ind_leaf << "*out_error = \"column '" << escape(field.name) << "' is missing\";\n";
            source_ << ind_leaf << "return false;\n";
            source_ << ind_inner << "}\n";
        }

        if (field.type == "int") {
            source_ << ind_inner << "int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : " << static_cast<long long>(field.default_int) << ";\n";
            source_ << ind_inner << "if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {\n";
            emit_fill_error(depth + 2, "column '" + escape(field.name) + "' expects an int");
            source_ << ind_inner << "}\n";
            source_ << ind_inner << "if (!pulse_data_table_field_set_int(row, &" << column << ", decoded, out_error)) {\n";
            source_ << ind_leaf << "*error_line = pulse_datalist_line(value);\n";
            source_ << ind_leaf << "*error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;\n";
            source_ << ind_leaf << "return false;\n";
            source_ << ind_inner << "}\n";
        } else if (field.type == "float") {
            source_ << ind_inner << "double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : " << format_double(field.default_float) << ";\n";
            source_ << ind_inner << "if (value) {\n";
            source_ << ind_leaf << "EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);\n";
            source_ << ind_leaf << "if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {\n";
            emit_fill_error(depth + 3, "column '" + escape(field.name) + "' expects a float");
            source_ << ind_leaf << "}\n";
            source_ << ind_inner << "}\n";
            source_ << ind_inner << "if (!pulse_data_table_field_set_float(row, &" << column << ", decoded, out_error)) {\n";
            source_ << ind_leaf << "*error_line = pulse_datalist_line(value);\n";
            source_ << ind_leaf << "*error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;\n";
            source_ << ind_leaf << "return false;\n";
            source_ << ind_inner << "}\n";
        } else if (field.type == "bool") {
            source_ << ind_inner << "bool decoded = value ? pulse_datalist_get_bool(value, nullptr, false) : " << (field.default_bool ? "true" : "false") << ";\n";
            source_ << ind_inner << "if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_BOOL) {\n";
            emit_fill_error(depth + 2, "column '" + escape(field.name) + "' expects a bool");
            source_ << ind_inner << "}\n";
            source_ << ind_inner << "pulse_data_table_field_set_bool(row, &" << column << ", decoded, out_error);\n";
        } else if (field.type == "string") {
            source_ << ind_inner << "if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {\n";
            emit_fill_error(depth + 2, "column '" + escape(field.name) + "' expects a string");
            source_ << ind_inner << "}\n";
            source_ << ind_inner << "std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, \"\"))) : std::string_view(\"" << escape(field.default_string) << "\");\n";
            source_ << ind_inner << "pulse_data_table_field_set_string(row, &" << column << ", &decoded, out_error);\n";
        } else {
            const Schema* nested = library_.find(field.type);
            if (nested && nested->kind == Kind::Enum) {
                source_ << ind_inner << "if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {\n";
                emit_fill_error(depth + 2, "column '" + escape(field.name) + "' expects an enum name");
                source_ << ind_inner << "}\n";
                source_ << ind_inner << "const char* raw = value ? pulse_datalist_get_string(value, nullptr, \"\") : \"" << escape(field.default_string) << "\";\n";
                source_ << ind_inner << "if (pulse_data_table_enum_lookup(owner, &" << column << ", raw) < 0) {\n";
                source_ << ind_leaf << "*error_line = pulse_datalist_line(value);\n";
                source_ << ind_leaf << "*error_code = PULSE_DATA_TABLE_ERROR_INVALID_ENUM_VALUE;\n";
                source_ << ind_leaf << "*out_error = \"column '" << escape(field.name) << "' is not in the enum whitelist\";\n";
                source_ << ind_leaf << "return false;\n";
                source_ << ind_inner << "}\n";
                source_ << ind_inner << "std::string_view decoded = vault->append(std::string_view(raw));\n";
                source_ << ind_inner << "pulse_data_table_field_set_string(row, &" << column << ", &decoded, out_error);\n";
            } else if (nested && nested->kind == Kind::Struct) {
                source_ << ind_inner << "if (!value) {\n";
                source_ << ind_leaf << "row->" << field.name << " = " << nested_default_expression(*nested) << ";\n";
                source_ << ind_inner << "} else if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_MAP) {\n";
                emit_fill_error(depth + 2, "column '" + escape(field.name) + "' expects a nested table");
                source_ << ind_inner << "} else {\n";
                source_ << ind_leaf << "if (!" << fill_name(plan, plan.scope_of.at(nested->name)) << "(context, vault_data, value, &row->" << field.name << ", error_line, error_code, out_error)) {\n";
                source_ << ind_leaf2 << "return false;\n";
                source_ << ind_leaf << "}\n";
                source_ << ind_inner << "}\n";
            } else {
                source_ << ind_inner << "const void* resolved = pulse_data_table_fill_context_resolve_ref(context, &" << column << ", value, out_error);\n";
                source_ << ind_inner << "if (value && !resolved) {\n";
                source_ << ind_leaf << "*error_line = pulse_datalist_line(value);\n";
                source_ << ind_leaf << "*error_code = PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE;\n";
                source_ << ind_leaf << "return false;\n";
                source_ << ind_inner << "}\n";
                source_ << ind_inner << "row->" << field.name << " = resolved ? reinterpret_cast<const ::" << module_ << "::Pulse" << pascal_case(field.ref) << "Row*>(resolved) : nullptr;\n";
            }
        }
        source_ << ind << "}\n";
    }

    void emit_fill_error(int depth, const std::string& message) {
        source_ << pad(depth) << "*error_line = pulse_datalist_line(value);\n";
        source_ << pad(depth) << "*error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;\n";
        source_ << pad(depth) << "*out_error = \"" << message << "\";\n";
        source_ << pad(depth) << "return false;\n";
    }

    void emit_table_helpers(const TablePlan& plan) {
        const Schema& schema = *plan.schema;
        const std::string table = table_type_name(schema);
        const std::string row = row_type_name(schema);
        const std::string path_slot = struct_type_name(plan) + "_path";
        const bool string_key = schema.fields[static_cast<size_t>(schema.key_index)].type != "int";

        source_ << "const char* " << table << "::DefaultPath() {\n";
        source_ << "    return \"" << escape(schema.name) << ".datatable\";\n";
        source_ << "}\n\n";

        source_ << "PulseAssetRequest " << table << "::Load(PulseAppId app, const char* path) {\n";
        source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
        source_ << "    if (!system) {\n";
        source_ << "        return pulse_asset_request_make_invalid();\n";
        source_ << "    }\n";
        source_ << "    if (path) {\n";
        source_ << "        " << path_slot << " = path;\n";
        source_ << "    }\n";
        source_ << "    const char* resolved = " << path_slot << " ? " << path_slot << " : DefaultPath();\n";
        source_ << "    " << path_slot << " = resolved;\n";
        source_ << "    return pulse_data_table_system_load(system, \"" << escape(schema.name) << "\", resolved);\n";
        source_ << "}\n\n";

        source_ << "bool " << table << "::IsReady(PulseAppId app) {\n";
        source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
        source_ << "    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));\n";
        source_ << "}\n\n";

        source_ << "const char* " << table << "::GetError(PulseAppId app) {\n";
        source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
        source_ << "    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;\n";
        source_ << "}\n\n";

        source_ << "const " << row << "* " << table << "::Rows(PulseAppId app, uint32_t& out_count) {\n";
        source_ << "    out_count = 0;\n";
        source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
        source_ << "    if (!system) {\n";
        source_ << "        return nullptr;\n";
        source_ << "    }\n";
        source_ << "    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));\n";
        source_ << "    if (!table) {\n";
        source_ << "        return nullptr;\n";
        source_ << "    }\n";
        source_ << "    return static_cast<const " << row << "*>(pulse_data_table_rows(table, &out_count));\n";
        source_ << "}\n\n";

        if (string_key) {
            source_ << "const " << row << "* " << table << "::GetRow(PulseAppId app, const char* key) {\n";
            source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
            source_ << "    if (!system) {\n";
            source_ << "        return nullptr;\n";
            source_ << "    }\n";
            source_ << "    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));\n";
            source_ << "    return table ? static_cast<const " << row << "*>(pulse_data_table_find_row(table, key)) : nullptr;\n";
            source_ << "}\n\n";
        } else {
            source_ << "const " << row << "* " << table << "::GetRow(PulseAppId app, int64_t key) {\n";
            source_ << "    PulseDataTableSystemId system = pulse_get_data_table_system(app);\n";
            source_ << "    if (!system) {\n";
            source_ << "        return nullptr;\n";
            source_ << "    }\n";
            source_ << "    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));\n";
            source_ << "    return table ? static_cast<const " << row << "*>(pulse_data_table_find_row_int(table, key)) : nullptr;\n";
            source_ << "}\n\n";
        }
    }

    void emit_register() {
        source_ << "EPulseResult RegisterSchemas(PulseDataTableSystemId system) {\n";
        source_ << "    if (!system) {\n";
        source_ << "        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;\n";
        source_ << "    }\n";
        for (const TablePlan& plan : plans_) {
            source_ << "    {\n";
            source_ << "        EPulseResult result = pulse_data_table_system_register_schema(system, &" << schema_name(plan) << ", nullptr);\n";
            source_ << "        if (result != PULSE_RESULT_OK) {\n";
            source_ << "            return result;\n";
            source_ << "        }\n";
            source_ << "    }\n";
        }
        source_ << "    return PULSE_RESULT_OK;\n";
        source_ << "}\n";
    }

    bool collect_das_names(const Schema& scope, const std::string& prefix, int depth, std::set<std::string>& names, std::string& collision) const {
        for (const Field& field : scope.fields) {
            const std::string name = prefix + pascal_case(field.name);
            const Schema* nested = library_.find(field.type);
            if (nested && nested->kind == Kind::Struct) {
                if (depth < 3 && !collect_das_names(*nested, name, depth + 1, names, collision)) {
                    return false;
                }
                continue;
            }
            if (!names.insert(name).second) {
                collision = field.name;
                return false;
            }
        }
        return true;
    }

    bool check_das_names(const Schema& schema, std::string& collision) const {
        static const char* reserved[] = { "Load", "IsReady", "GetError", "RowCount", "RowAt", "FindRow", "FindRowInt" };
        std::set<std::string> names{};
        const std::string prefix = "Pulse" + pascal_case(schema.name);
        for (const char* suffix : reserved) {
            names.insert(prefix + suffix);
        }
        return collect_das_names(schema, prefix + "Get", 0, names, collision);
    }

    void emit_das() {
        das_ << "options gen2\n";
        das_ << "options indenting = 4\n";
        das_ << "options no_unused_block_arguments = false\n";
        das_ << "options no_unused_function_arguments = false\n\n";
        das_ << "module " << das_module_ << " public\n\n";
        das_ << "require pulse public\n\n";
        das_ << "var pulse_tables_registered : bool = false\n\n";
        for (const TablePlan& plan : plans_) {
            emit_das_register(plan);
        }
        emit_das_register_all();
        for (const TablePlan& plan : plans_) {
            emit_das_table(plan);
        }
    }

    std::vector<const Schema*> das_struct_order(const TablePlan& plan) const {
        std::vector<const Schema*> ordered{};
        std::set<std::string> done{};
        collect_das_struct_order(*plan.schema, ordered, done);
        return ordered;
    }

    void collect_das_struct_order(const Schema& schema, std::vector<const Schema*>& out, std::set<std::string>& done) const {
        for (const Field& field : schema.fields) {
            const Schema* nested = library_.find(field.type);
            if (!nested || nested->kind != Kind::Struct) {
                continue;
            }
            if (!done.insert(nested->name).second) {
                continue;
            }
            collect_das_struct_order(*nested, out, done);
            out.push_back(nested);
        }
    }

    struct DasEnum {
        std::string name{};
        std::string variable{};
        const std::vector<std::string>* values = nullptr;
    };

    std::vector<DasEnum> das_enums(const TablePlan& plan) const {
        std::vector<DasEnum> result{};
        std::set<std::string> seen{};
        for (const Scope& scope : plan.scopes) {
            for (const Field& field : scope.schema->fields) {
                DasEnum entry{};
                if (is_scalar_type(field.type)) {
                    if (field.values.empty()) {
                        continue;
                    }
                    entry.name = field.name + "_values";
                    entry.variable = "enum_" + field.name;
                    entry.values = &field.values;
                } else {
                    const Schema* nested = library_.find(field.type);
                    if (!nested || nested->kind != Kind::Enum) {
                        continue;
                    }
                    entry.name = nested->name;
                    entry.variable = "enum_" + nested->name;
                    entry.values = &nested->values;
                }
                if (seen.insert(entry.name).second) {
                    result.push_back(entry);
                }
            }
        }
        return result;
    }

    std::string das_column_type(const Field& field, const Schema* nested) const {
        if (field.type == "int") {
            return "PULSE_DATA_TABLE_COLUMN_TYPE_INT";
        }
        if (field.type == "float") {
            return "PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT";
        }
        if (field.type == "bool") {
            return "PULSE_DATA_TABLE_COLUMN_TYPE_BOOL";
        }
        if (field.type == "ref") {
            return "PULSE_DATA_TABLE_COLUMN_TYPE_REF";
        }
        if (field.type == "string") {
            return field.values.empty() ? "PULSE_DATA_TABLE_COLUMN_TYPE_STRING" : "PULSE_DATA_TABLE_COLUMN_TYPE_ENUM";
        }
        return nested->kind == Kind::Struct ? "PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT" : "PULSE_DATA_TABLE_COLUMN_TYPE_ENUM";
    }

    std::string das_column(const Field& field) const {
        const Schema* nested = library_.find(field.type);
        std::string text = "PulseDataTableColumnDesc(name = \"" + escape(field.name) + "\", column_type = EPulseDataTableColumnType." + das_column_type(field, nested);
        if (field.has_default) {
            if (field.type == "bool") {
                text += ", has_default = true, default_bool = " + std::string(field.default_bool ? "true" : "false");
            } else if (field.type == "int") {
                text += ", has_default = true, default_int = int64(" + std::to_string(static_cast<long long>(field.default_int)) + ")";
            } else if (field.type == "float") {
                text += ", has_default = true, default_float = double(" + format_double_short(field.default_float) + ")";
            } else {
                text += ", has_default = true, default_string = \"" + escape(field.default_string) + "\"";
            }
        }
        if (field.has_min) {
            text += ", has_min = true, min_value = double(" + format_double_short(field.min_value) + ")";
        }
        if (field.has_max) {
            text += ", has_max = true, max_value = double(" + format_double_short(field.max_value) + ")";
        }
        if (field.type == "ref") {
            text += ", ref_type = \"" + escape(field.ref) + "\"";
        } else if (!is_scalar_type(field.type) && nested) {
            text += nested->kind == Kind::Struct ? ", struct_type = \"" + escape(nested->name) + "\"" : ", enum_type = \"" + escape(nested->name) + "\"";
        } else if (!field.values.empty()) {
            text += ", enum_type = \"" + escape(field.name) + "_values\"";
        }
        text += ")";
        return text;
    }

    void emit_das_register(const TablePlan& plan) {
        const Schema& schema = *plan.schema;
        const std::string prefix = "Pulse" + pascal_case(schema.name);
        const std::string name = escape(schema.name);
        const std::vector<const Schema*> declared = das_struct_order(plan);
        const std::vector<DasEnum> enums = das_enums(plan);

        das_ << "def private " << prefix << "RegisterSchema(app: PulseAppId) : bool {\n";
        for (const Schema* nested : declared) {
            das_ << "    var " << nested->name << "_columns : array<PulseDataTableColumnDesc>\n";
            for (const Field& field : nested->fields) {
                das_ << "    " << nested->name << "_columns |> push <| " << das_column(field) << "\n";
            }
            das_ << "    var struct_" << nested->name << " = PulseDataTableStructDesc(name = \"" << escape(nested->name) << "\", columns_count = uint64(length(" << nested->name << "_columns)))\n";
            das_ << "    unsafe {\n";
            das_ << "        struct_" << nested->name << ".p_columns = reinterpret<void?>(addr(" << nested->name << "_columns[0]))\n";
            das_ << "    }\n";
        }
        if (!plan.struct_order.empty()) {
            das_ << "    var structs : array<PulseDataTableStructDesc>\n";
            for (const Schema* nested : plan.struct_order) {
                das_ << "    structs |> push <| struct_" << nested->name << "\n";
            }
        }
        for (const DasEnum& entry : enums) {
            das_ << "    var " << entry.name << " : array<string>\n";
            for (const std::string& value : *entry.values) {
                das_ << "    " << entry.name << " |> push <| \"" << escape(value) << "\"\n";
            }
            das_ << "    var " << entry.variable << " = PulseDataTableEnumDesc(name = \"" << escape(entry.name) << "\", values_count = uint64(length(" << entry.name << ")))\n";
            das_ << "    unsafe {\n";
            das_ << "        " << entry.variable << ".p_values = reinterpret<void?>(addr(" << entry.name << "[0]))\n";
            das_ << "    }\n";
        }
        if (!enums.empty()) {
            das_ << "    var enums : array<PulseDataTableEnumDesc>\n";
            for (const DasEnum& entry : enums) {
                das_ << "    enums |> push <| " << entry.variable << "\n";
            }
        }
        das_ << "    var columns : array<PulseDataTableColumnDesc>\n";
        for (const Field& field : schema.fields) {
            das_ << "    columns |> push <| " << das_column(field) << "\n";
        }
        das_ << "    var desc = PulseDataTableSchemaDesc(name = \"" << name << "\", columns_count = uint64(length(columns)), ";
        if (!plan.struct_order.empty()) {
            das_ << "structs_count = uint64(length(structs)), ";
        }
        if (!enums.empty()) {
            das_ << "enums_count = uint64(length(enums)), ";
        }
        das_ << "key_column = uint(" << schema.key_index << "))\n";
        das_ << "    unsafe {\n";
        das_ << "        desc.p_columns = reinterpret<void?>(addr(columns[0]))\n";
        if (!plan.struct_order.empty()) {
            das_ << "        desc.p_structs = reinterpret<void?>(addr(structs[0]))\n";
        }
        if (!enums.empty()) {
            das_ << "        desc.p_enums = reinterpret<void?>(addr(enums[0]))\n";
        }
        das_ << "    }\n";
        das_ << "    let error = pulse_data_table_register_schema(app, desc)\n";
        das_ << "    if (error != \"\") {\n";
        das_ << "        print(\"Data table schema '" << name << "' failed to register: {error}\\n\")\n";
        das_ << "        return false\n";
        das_ << "    }\n";
        das_ << "    return true\n";
        das_ << "}\n\n";
    }

    void emit_das_register_all() {
        das_ << "def PulseTablesRegisterSchemas(app: PulseAppId) {\n";
        das_ << "    if (pulse_tables_registered) {\n";
        das_ << "        return\n";
        das_ << "    }\n";
        if (plans_.empty()) {
            das_ << "    pulse_tables_registered = true\n";
        } else {
            for (size_t i = 0; i < plans_.size(); ++i) {
                const std::string call = "Pulse" + pascal_case(plans_[i].schema->name) + "RegisterSchema(app)";
                das_ << "    " << (i == 0 ? "var ok = " : "ok = ok && ") << call << "\n";
            }
            das_ << "    pulse_tables_registered = ok\n";
        }
        das_ << "}\n\n";
    }

    void emit_das_table(const TablePlan& plan) {
        const Schema& schema = *plan.schema;
        const std::string prefix = "Pulse" + pascal_case(schema.name);
        const std::string name = escape(schema.name);
        const bool string_key = schema.fields[static_cast<size_t>(schema.key_index)].type != "int";

        das_ << "def " << prefix << "Load(app: PulseAppId; path: string = \"" << name << ".datatable\") : PulseAssetRequest {\n";
        das_ << "    PulseTablesRegisterSchemas(app)\n";
        das_ << "    return pulse_data_table_load(app, \"" << name << "\", path)\n";
        das_ << "}\n\n";

        das_ << "def " << prefix << "IsReady(app: PulseAppId; request: PulseAssetRequest) : bool {\n";
        das_ << "    return pulse_data_table_is_ready(app, request)\n";
        das_ << "}\n\n";

        das_ << "def " << prefix << "GetError(app: PulseAppId; request: PulseAssetRequest) : string {\n";
        das_ << "    return pulse_data_table_get_error(app, request)\n";
        das_ << "}\n\n";

        das_ << "def " << prefix << "RowCount(app: PulseAppId) : int {\n";
        das_ << "    return int(pulse_data_table_row_count(app, \"" << name << "\"))\n";
        das_ << "}\n\n";

        das_ << "def " << prefix << "RowAt(app: PulseAppId; index: int) : void? {\n";
        das_ << "    return pulse_data_table_row_at(app, \"" << name << "\", int64(index))\n";
        das_ << "}\n\n";

        if (string_key) {
            das_ << "def " << prefix << "FindRow(app: PulseAppId; key: string) : void? {\n";
            das_ << "    return pulse_data_table_find_row(app, \"" << name << "\", key)\n";
            das_ << "}\n\n";
        } else {
            das_ << "def " << prefix << "FindRowInt(app: PulseAppId; key: int) : void? {\n";
            das_ << "    return pulse_data_table_find_row_int(app, \"" << name << "\", int64(key))\n";
            das_ << "}\n\n";
        }

        std::vector<int64_t> path{};
        emit_das_fields(schema, schema, prefix + "Get", path, 0);
    }

    void emit_das_fields(const Schema& owner, const Schema& scope, const std::string& prefix, std::vector<int64_t>& path, int depth) {
        for (size_t i = 0; i < scope.fields.size(); ++i) {
            const Field& field = scope.fields[i];
            const std::string name = prefix + pascal_case(field.name);
            const Schema* nested = library_.find(field.type);
            if (nested && nested->kind == Kind::Struct) {
                if (depth < 3) {
                    path.push_back(static_cast<int64_t>(i));
                    emit_das_fields(owner, *nested, name, path, depth + 1);
                    path.pop_back();
                }
                continue;
            }
            emit_das_reader(owner, field, nested, name, path, i);
        }
    }

    void emit_das_reader(const Schema& owner, const Field& field, const Schema* nested, const std::string& name, const std::vector<int64_t>& path, size_t index) {
        const char* reader = "pulse_data_table_read_int";
        const char* result = "int64";
        if (field.type == "float") {
            reader = "pulse_data_table_read_float";
            result = "double";
        } else if (field.type == "bool") {
            reader = "pulse_data_table_read_bool";
            result = "bool";
        } else if (field.type == "string") {
            reader = "pulse_data_table_read_string";
            result = "string";
        } else if (field.type == "ref") {
            reader = "pulse_data_table_read_ref";
            result = "void?";
        } else if (nested && nested->kind == Kind::Enum) {
            reader = "pulse_data_table_read_string";
            result = "string";
        } else if (nested && nested->kind == Kind::Table) {
            reader = "pulse_data_table_read_ref";
            result = "void?";
        }

        std::string indices{};
        for (int level = 0; level < 4; ++level) {
            indices += ", int64(";
            if (level < static_cast<int>(path.size())) {
                indices += std::to_string(path[static_cast<size_t>(level)]);
            } else if (level == static_cast<int>(path.size())) {
                indices += std::to_string(index);
            } else {
                indices += "-1";
            }
            indices += ")";
        }

        das_ << "def " << name << "(app: PulseAppId; row: void?) : " << result << " {\n";
        das_ << "    return " << reader << "(app, \"" << escape(owner.name) << "\", row" << indices << ")\n";
        das_ << "}\n\n";
    }

    const Library& library_;
    const LayoutSet& layouts_;
    std::string module_{};
    std::string header_name_{};
    std::string das_module_{};
    bool emit_das_ = false;
    std::vector<TablePlan> plans_{};
    std::map<std::string, bool> defaultable_cache_{};
    std::ostringstream header_{};
    std::ostringstream source_{};
    std::ostringstream das_{};
};

} // namespace

bool generate(const Library& library, const LayoutSet& layouts, const std::string& header_path, const std::string& source_path, const std::string& header_name, const std::string& das_path, const std::string& das_module, std::vector<std::string>& errors) {
    Generator generator(library, layouts, "pulse_tables", header_name, !das_path.empty());
    if (!generator.build(errors)) {
        return false;
    }

    if (!header_path.empty() && !source_path.empty()) {
        std::ofstream header(header_path, std::ios::binary);
        if (!header) {
            errors.push_back("cannot write " + header_path);
            return false;
        }
        header << to_crlf(generator.header_text());
        header.close();

        std::ofstream source(source_path, std::ios::binary);
        if (!source) {
            errors.push_back("cannot write " + source_path);
            return false;
        }
        source << to_crlf(generator.source_text());
        source.close();
    }

    if (das_path.empty()) {
        return true;
    }

    std::ofstream das(das_path, std::ios::binary);
    if (!das) {
        errors.push_back("cannot write " + das_path);
        return false;
    }
    das << to_crlf(generator.das_text(das_module));
    das.close();
    return true;
}

} // namespace tablegen
