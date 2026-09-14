#include "datatable_internal.h"

#include <algorithm>
#include <cstdio>

namespace pulse::datatable {

static uint32_t align_up(uint32_t value, uint32_t alignment) {
    if (alignment <= 1) {
        return value;
    }
    return (value + alignment - 1u) / alignment * alignment;
}

static uint32_t column_align(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc& column) {
    switch (column.type) {
    case PULSE_DATA_TABLE_COLUMN_TYPE_INT:
        return alignof(int64_t);
    case PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT:
        return alignof(double);
    case PULSE_DATA_TABLE_COLUMN_TYPE_BOOL:
        return alignof(bool);
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRING:
        return alignof(std::string_view);
    case PULSE_DATA_TABLE_COLUMN_TYPE_ENUM:
        return alignof(std::string_view);
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT: {
        const PulseDataTableStructDesc* desc = find_struct(schema, column.struct_type ? column.struct_type : "");
        return desc ? desc->align : 1u;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_REF:
        return alignof(void*);
    default:
        return 1u;
    }
}

static uint32_t column_size(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc& column) {
    switch (column.type) {
    case PULSE_DATA_TABLE_COLUMN_TYPE_INT:
        return sizeof(int64_t);
    case PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT:
        return sizeof(double);
    case PULSE_DATA_TABLE_COLUMN_TYPE_BOOL:
        return sizeof(bool);
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRING:
        return sizeof(std::string_view);
    case PULSE_DATA_TABLE_COLUMN_TYPE_ENUM:
        return sizeof(std::string_view);
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT: {
        const PulseDataTableStructDesc* desc = find_struct(schema, column.struct_type ? column.struct_type : "");
        return desc ? desc->size : 0u;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_REF:
        return sizeof(void*);
    default:
        return 0u;
    }
}

uint32_t table_row_size(const PulseDataTableSchemaDesc* schema) {
    if (!schema) {
        return 1;
    }
    uint32_t cursor = 0;
    uint32_t max_align = 1;
    for (size_t i = 0; i < schema->columns_count; ++i) {
        uint32_t alignment = column_align(schema, schema->p_columns[i]);
        max_align = std::max(max_align, alignment);
        cursor = align_up(cursor, alignment) + column_size(schema, schema->p_columns[i]);
    }
    return std::max(align_up(cursor, max_align), 1u);
}

const PulseDataTableColumnDesc* find_column(const PulseDataTableSchemaDesc* schema, std::string_view name) {
    if (!schema || name.empty()) {
        return nullptr;
    }
    for (size_t i = 0; i < schema->columns_count; ++i) {
        const PulseDataTableColumnDesc& column = schema->p_columns[i];
        if (column.name && name == column.name) {
            return &column;
        }
    }
    return nullptr;
}

const PulseDataTableStructDesc* find_struct(const PulseDataTableSchemaDesc* schema, std::string_view name) {
    if (!schema || name.empty()) {
        return nullptr;
    }
    for (size_t i = 0; i < schema->structs_count; ++i) {
        const PulseDataTableStructDesc& desc = schema->p_structs[i];
        if (desc.name && name == desc.name) {
            return &desc;
        }
    }
    return nullptr;
}

const PulseDataTableEnumDesc* find_enum(const PulseDataTableSchemaDesc* schema, std::string_view name) {
    if (!schema || name.empty()) {
        return nullptr;
    }
    for (size_t i = 0; i < schema->enums_count; ++i) {
        const PulseDataTableEnumDesc& desc = schema->p_enums[i];
        if (desc.name && name == desc.name) {
            return &desc;
        }
    }
    return nullptr;
}

const PulseDataTableColumnDesc* find_struct_column(const PulseDataTableSchemaDesc* schema, std::string_view struct_name, std::string_view column_name) {
    const PulseDataTableStructDesc* desc = find_struct(schema, struct_name);
    if (!desc) {
        return nullptr;
    }
    for (size_t i = 0; i < desc->columns_count; ++i) {
        const PulseDataTableColumnDesc& column = desc->p_columns[i];
        if (column.name && column_name == column.name) {
            return &column;
        }
    }
    return nullptr;
}

const char* data_table_error_text(EPulseDataTableError error) {
    switch (error) {
    case PULSE_DATA_TABLE_ERROR_SYSTEM_IS_INVALID: return "data table system is invalid";
    case PULSE_DATA_TABLE_ERROR_PATH_IS_EMPTY: return "data table path is empty";
    case PULSE_DATA_TABLE_ERROR_NO_SCHEMA_FOR_TYPE: return "file extension has no data table schema";
    case PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED: return "schema is not registered";
    case PULSE_DATA_TABLE_ERROR_PARSE_FAILED: return "failed to parse data table";
    case PULSE_DATA_TABLE_ERROR_ROOT_IS_NOT_TABLE: return "data table root is not a table";
    case PULSE_DATA_TABLE_ERROR_SCHEMA_FIELD_MISSING: return "missing 'schema' field";
    case PULSE_DATA_TABLE_ERROR_ROWS_FIELD_MISSING: return "missing 'rows' list";
    case PULSE_DATA_TABLE_ERROR_ROW_IS_NOT_TABLE: return "row is not a table";
    case PULSE_DATA_TABLE_ERROR_MISSING_COLUMN: return "row is missing a required column";
    case PULSE_DATA_TABLE_ERROR_UNKNOWN_COLUMN: return "row has a column that the schema does not declare";
    case PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH: return "column value has the wrong type";
    case PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE: return "column value is out of range";
    case PULSE_DATA_TABLE_ERROR_INVALID_ENUM_VALUE: return "column value is not in the enum whitelist";
    case PULSE_DATA_TABLE_ERROR_DUPLICATE_KEY: return "duplicate primary key";
    case PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE: return "referenced row does not exist";
    case PULSE_DATA_TABLE_ERROR_REFERENCE_SCHEMA_MISMATCH: return "referenced table has the wrong schema";
    case PULSE_DATA_TABLE_ERROR_OUT_OF_MEMORY: return "out of memory";
    default: return "unknown data table error";
    }
}

bool check_range(const PulseDataTableColumnDesc& column, double value, EPulseDataTableError& error, const char*& message) {
    if (column.has_min && value < column.min_value) {
        error = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
        message = "value is below the schema minimum";
        return false;
    }
    if (column.has_max && value > column.max_value) {
        error = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
        message = "value is above the schema maximum";
        return false;
    }
    return true;
}

bool decode_int(const PulseDatalist* node, int64_t default_value, int64_t& out) {
    if (!node) {
        out = default_value;
        return true;
    }
    if (pulse_datalist_get_type(node, nullptr) != PULSE_DATALIST_TYPE_INT) {
        return false;
    }
    out = pulse_datalist_get_int(node, nullptr, default_value);
    return true;
}

bool decode_float(const PulseDatalist* node, double default_value, double& out) {
    if (!node) {
        out = default_value;
        return true;
    }
    EPulseDatalistType type = pulse_datalist_get_type(node, nullptr);
    if (type != PULSE_DATALIST_TYPE_DOUBLE && type != PULSE_DATALIST_TYPE_INT) {
        return false;
    }
    out = pulse_datalist_get_double(node, nullptr, default_value);
    return true;
}

bool decode_bool(const PulseDatalist* node, bool default_value, bool& out) {
    if (!node) {
        out = default_value;
        return true;
    }
    if (pulse_datalist_get_type(node, nullptr) != PULSE_DATALIST_TYPE_BOOL) {
        return false;
    }
    out = pulse_datalist_get_bool(node, nullptr, default_value);
    return true;
}

bool decode_string(const PulseDatalist* node, const char* default_value, std::string_view& out) {
    if (!node) {
        if (default_value) {
            out = default_value;
            return true;
        }
        out = std::string_view{};
        return false;
    }
    if (pulse_datalist_get_type(node, nullptr) != PULSE_DATALIST_TYPE_STRING) {
        return false;
    }
    const char* text = pulse_datalist_get_string(node, nullptr, nullptr);
    if (text) {
        size_t length = std::strlen(text);
        out = std::string_view(text, length);
        return std::strchr(text, '\0') == text + length;
    }
    if (default_value) {
        out = default_value;
        return true;
    }
    return false;
}

bool decode_enum(const PulseDataTableEnumDesc* enum_desc, const PulseDatalist* node, std::string_view& out, const char*& message) {
    if (!enum_desc || !node || pulse_datalist_get_type(node, nullptr) != PULSE_DATALIST_TYPE_STRING) {
        message = "column value is not in the enum whitelist";
        return false;
    }
    const char* text = pulse_datalist_get_string(node, nullptr, nullptr);
    if (!text) {
        message = "column value is not in the enum whitelist";
        return false;
    }
    for (size_t i = 0; i < enum_desc->values_count; ++i) {
        const char* candidate = enum_desc->p_values[i];
        if (candidate && std::strcmp(candidate, text) == 0) {
            out = std::string_view(text);
            return true;
        }
    }
    message = "column value is not in the enum whitelist";
    return false;
}

bool check_column(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc& column, const PulseDatalist* node, EPulseDataTableError& error, const char*& message) {
    switch (column.type) {
    case PULSE_DATA_TABLE_COLUMN_TYPE_INT: {
        int64_t value = 0;
        if (!decode_int(node, column.has_default ? column.default_int : 0, value)) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "column expects an int";
            return false;
        }
        return check_range(column, static_cast<double>(value), error, message);
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT: {
        double value = 0.0;
        if (!decode_float(node, column.has_default ? column.default_float : 0.0, value)) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "column expects a float";
            return false;
        }
        return check_range(column, value, error, message);
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_BOOL: {
        bool value = false;
        if (!decode_bool(node, column.has_default ? column.default_bool : false, value)) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "column expects a bool";
            return false;
        }
        return true;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRING: {
        std::string_view value{};
        if (!decode_string(node, column.has_default ? column.default_string : nullptr, value)) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "column expects a string without embedded NUL bytes";
            return false;
        }
        return true;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_ENUM: {
        std::string_view value{};
        if (!decode_enum(find_enum(schema, column.enum_type ? column.enum_type : ""), node, value, message)) {
            error = PULSE_DATA_TABLE_ERROR_INVALID_ENUM_VALUE;
            return false;
        }
        return true;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT: {
        if (!node || pulse_datalist_get_type(node, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "column expects a nested table";
            return false;
        }
        const PulseDataTableStructDesc* desc = find_struct(schema, column.struct_type ? column.struct_type : "");
        if (!desc) {
            error = PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED;
            message = "struct type is not declared in the schema";
            return false;
        }
        for (size_t i = 0; i < desc->columns_count; ++i) {
            const PulseDataTableColumnDesc& nested = desc->p_columns[i];
            const PulseDatalist* value = pulse_datalist_get_obj(node, nested.name);
            if (!value && !nested.has_default) {
                error = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
                message = "nested table is missing a required column";
                return false;
            }
            if (!check_column(schema, nested, value, error, message)) {
                return false;
            }
        }
        return true;
    }
    case PULSE_DATA_TABLE_COLUMN_TYPE_REF: {
        std::string_view value{};
        if (!decode_string(node, nullptr, value)) {
            error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            message = "reference column expects a string key";
            return false;
        }
        return true;
    }
    default:
        error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
        message = "unknown column type";
        return false;
    }
}

} // namespace pulse::datatable
