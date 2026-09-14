#pragma once

#ifndef PULSE_DATATABLE_API_HEADER_GUARD
#define PULSE_DATATABLE_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // int32_t, int64_t, uint32_t, uint64_t
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_datalist.h"

#if defined(PULSE_DATATABLE_MODULE_BUILD)
#  define PULSE_DATATABLE_API PULSE_EXPORT
#else
#  define PULSE_DATATABLE_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_DATA_TABLE_PLUGIN_DESC_VERSION 1u

/**
 * Asset type id range 0x3000+ belongs to pulse_datatable (graphics uses 0x1000+, pulse_prefab and pulse_daslang use 0x2000+)
 *
 */
#define PULSE_TYPE_DATA_TABLE UINT64_C(0x3000)


/**
 * Loader step results, reported through the asset system
 *
 */
typedef enum EPulseDataTableLoaderStatus
{
    PULSE_DATA_TABLE_LOADER_STATUS_PENDING,   /** ( 0)                                */
    PULSE_DATA_TABLE_LOADER_STATUS_DONE,      /** ( 1)                                */
    PULSE_DATA_TABLE_LOADER_STATUS_FAILED,    /** ( 2)                                */

    PULSE_DATA_TABLE_LOADER_STATUS_COUNT

} EPulseDataTableLoaderStatus;

/**
 * Column storage type
 *
 */
typedef enum EPulseDataTableColumnType
{
    PULSE_DATA_TABLE_COLUMN_TYPE_INT,         /** ( 0)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT,       /** ( 1)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_BOOL,        /** ( 2)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_STRING,      /** ( 3)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_ENUM,        /** ( 4)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT,      /** ( 5)                                */
    PULSE_DATA_TABLE_COLUMN_TYPE_REF,         /** ( 6)                                */

    PULSE_DATA_TABLE_COLUMN_TYPE_COUNT

} EPulseDataTableColumnType;

/**
 * Failure reasons reported through pulse_data_table_system_get_error
 *
 */
typedef enum EPulseDataTableError
{
    PULSE_DATA_TABLE_ERROR_SYSTEM_IS_INVALID, /** ( 0)                                */
    PULSE_DATA_TABLE_ERROR_PATH_IS_EMPTY,     /** ( 1)                                */
    PULSE_DATA_TABLE_ERROR_NO_SCHEMA_FOR_TYPE, /** ( 2)                                */
    PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED, /** ( 3)                                */
    PULSE_DATA_TABLE_ERROR_PARSE_FAILED,      /** ( 4)                                */
    PULSE_DATA_TABLE_ERROR_ROOT_IS_NOT_TABLE, /** ( 5)                                */
    PULSE_DATA_TABLE_ERROR_SCHEMA_FIELD_MISSING, /** ( 6)                                */
    PULSE_DATA_TABLE_ERROR_ROWS_FIELD_MISSING, /** ( 7)                                */
    PULSE_DATA_TABLE_ERROR_ROW_IS_NOT_TABLE,  /** ( 8)                                */
    PULSE_DATA_TABLE_ERROR_MISSING_COLUMN,    /** ( 9)                                */
    PULSE_DATA_TABLE_ERROR_UNKNOWN_COLUMN,    /** (10)                                */
    PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH,     /** (11)                                */
    PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE,      /** (12)                                */
    PULSE_DATA_TABLE_ERROR_INVALID_ENUM_VALUE, /** (13)                                */
    PULSE_DATA_TABLE_ERROR_DUPLICATE_KEY,     /** (14)                                */
    PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE, /** (15)                                */
    PULSE_DATA_TABLE_ERROR_REFERENCE_SCHEMA_MISMATCH, /** (16)                                */
    PULSE_DATA_TABLE_ERROR_OUT_OF_MEMORY,     /** (17)                                */

    PULSE_DATA_TABLE_ERROR_COUNT

} EPulseDataTableError;


DEFINE_PULSE_OBJECT(PulseDataTableSystem)
DEFINE_PULSE_OBJECT(PulseDataTable)

// Forward declarations for types used by struct fields and function pointers
struct PulseDataTableColumnDesc;
typedef struct PulseDataTableColumnDesc PulseDataTableColumnDesc;

/**
 * Function pointer: maps an enum name to its integer value
 *
 * @param[in] value
 *
 */
typedef int32_t (*PulseProcDataTableEnumDecodeFn)(const char* value);
/**
 * Function pointer: fills a row field by field, including references and nested structs
 *
 * @param[in] context
 * @param[in] vault
 * @param[in] node
 * @param[in] out
 * @param[out] errorLine
 * @param[out] errorCode
 * @param[out] outError
 *
 */
typedef bool (*PulseProcDataTableFillRowFn)(const void* context, void* vault, const PulseDatalist* node, void* out, [[pulse::out]] int32_t* error_line, [[pulse::out]] EPulseDataTableError* error_code, [[pulse::out]] const char** out_error);

/**
 * Plugin descriptor
 *
 */
typedef struct PulseDataTablePluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;

} PulseDataTablePluginDesc;

struct PulseDataTableSystem;
typedef struct PulseDataTableSystem PulseDataTableSystem;

struct PulseDataTable;
typedef struct PulseDataTable PulseDataTable;

/**
 * Schema of a shared struct type, used to fill nested struct columns
 *
 */
typedef struct PulseDataTableStructDesc
{
    const char*          name;
    uint32_t             size;
    uint32_t             align;
    Pulse_Array(const PulseDataTableColumnDesc, columns);

} PulseDataTableStructDesc;

/**
 * Schema of an enum type, used to map strings to integer values
 *
 */
typedef struct PulseDataTableEnumDesc
{
    const char*          name;
    Pulse_Array(const char*, values);
    uint32_t             count;

} PulseDataTableEnumDesc;

/**
 * Overrides for a referenced table: a struct column holds a prefix of the target row
 *
 */
typedef struct PulseDataTableColumnInjection
{
    uint32_t             column;
    uint32_t             field;

} PulseDataTableColumnInjection;

/**
 * Column schema: storage layout plus validation rules
 *
 */
typedef struct PulseDataTableColumnDesc
{
    const char*          name;
    EPulseDataTableColumnType type;
    uint32_t             offset;
    double               min_value;
    double               max_value;
    bool                 has_min;
    bool                 has_max;
    bool                 has_default;
    int64_t              default_int;
    double               default_float;
    bool                 default_bool;
    const char*          default_string;
    [[pulse::optional]]
    const char*          struct_type;
    [[pulse::optional]]
    const char*          ref_type;
    [[pulse::optional]]
    const char*          enum_type;
    Pulse_Array(const PulseDataTableColumnInjection, injections);

} PulseDataTableColumnDesc;

/**
 * Schema of one table type
 *
 */
typedef struct PulseDataTableSchemaDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    const char*          name;
    uint32_t             row_size;
    uint32_t             row_align;
    Pulse_Array(const PulseDataTableColumnDesc, columns);
    Pulse_Array(const PulseDataTableStructDesc, structs);
    Pulse_Array(const PulseDataTableEnumDesc, enums);
    uint32_t             key_column;
    bool                 key_is_int;
    uint32_t             key_field;
    PulseProcDataTableFillRowFn fill_row;

} PulseDataTableSchemaDesc;



/**
 * Functions
 *
 */
PULSE_DATATABLE_API PulseDataTablePluginDesc pulse_data_table_plugin_desc_default(void);
PULSE_DATATABLE_API EPulseAppAddPluginResult pulse_add_data_table_plugin(PulseAppId app, const PulseDataTablePluginDesc* desc);
[[pulse::optional]] PULSE_DATATABLE_API PulseDataTableSystemId pulse_get_data_table_system(PulseAppId app);
PULSE_DATATABLE_API EPulseResult pulse_data_table_system_register_schema(PulseDataTableSystemId _this, const PulseDataTableSchemaDesc* desc);
PULSE_DATATABLE_API PulseAssetRequest pulse_data_table_system_load(PulseDataTableSystemId _this, const char* schema, const char* path);
PULSE_DATATABLE_API bool pulse_data_table_system_is_ready(Const_PulseDataTableSystemId _this, PulseAssetRequest request);
PULSE_DATATABLE_API bool pulse_data_table_system_is_alive(Const_PulseDataTableSystemId _this, PulseAssetRequest request);
[[pulse::optional]] PULSE_DATATABLE_API const char* pulse_data_table_system_get_error(Const_PulseDataTableSystemId _this, PulseAssetRequest request);
[[pulse::optional]] PULSE_DATATABLE_API PulseDataTableId pulse_data_table_system_get(PulseDataTableSystemId _this, PulseAssetRequest request);
[[pulse::optional]] PULSE_DATATABLE_API PulseDataTableId pulse_data_table_system_get_by_name(Const_PulseDataTableSystemId _this, const char* schema);
[[pulse::optional]] PULSE_DATATABLE_API const PulseDataTableSchemaDesc* pulse_data_table_system_get_schema(Const_PulseDataTableSystemId _this, const char* schema);
[[pulse::optional]] PULSE_DATATABLE_API const char* pulse_data_table_get_name(PulseDataTableId table);
PULSE_DATATABLE_API uint32_t pulse_data_table_row_count(PulseDataTableId table);
[[pulse::optional]] PULSE_DATATABLE_API const void* pulse_data_table_find_row(PulseDataTableId table, const char* key);
[[pulse::optional]] PULSE_DATATABLE_API const void* pulse_data_table_find_row_int(PulseDataTableId table, int64_t key);
[[pulse::optional]] PULSE_DATATABLE_API const void* pulse_data_table_rows(PulseDataTableId table, [[pulse::out]] uint32_t* out_count);
PULSE_DATATABLE_API bool pulse_data_table_field_set_int(void* row, const PulseDataTableColumnDesc* column, int64_t value, [[pulse::out]] const char** out_error);
PULSE_DATATABLE_API bool pulse_data_table_field_set_float(void* row, const PulseDataTableColumnDesc* column, double value, [[pulse::out]] const char** out_error);
PULSE_DATATABLE_API bool pulse_data_table_field_set_bool(void* row, const PulseDataTableColumnDesc* column, bool value, [[pulse::out]] const char** out_error);
PULSE_DATATABLE_API bool pulse_data_table_field_set_string(void* row, const PulseDataTableColumnDesc* column, const void* value, [[pulse::out]] const char** out_error);
[[pulse::optional]] PULSE_DATATABLE_API const void* pulse_data_table_fill_context_resolve_ref(const void* context, const PulseDataTableColumnDesc* column, const PulseDatalist* node, [[pulse::out]] const char** out_error);
PULSE_DATATABLE_API int32_t pulse_data_table_enum_lookup(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc* column, const char* value);

#ifdef __cplusplus
}
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_DATATABLE_API_HEADER_GUARD
