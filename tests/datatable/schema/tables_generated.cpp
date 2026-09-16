#include "tables_generated.h"

#include "pulse_datatable_schema.h"

#include <array>

namespace
{

constexpr const PulseDataTableSchemaDesc* pulse_table_declares_other_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_deep_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_dup_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_enm_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_extra_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_hero_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_item_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_num_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_ref_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_refdefault_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_req_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_rng_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_snake_schema_ptr();
constexpr const PulseDataTableSchemaDesc* pulse_table_typ_schema_ptr();

const PulseDataTableColumnDesc pulse_table_declares_other_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const std::array<PulseDataTableColumnDesc, 1> pulse_table_declares_other_columns_0 = { pulse_table_declares_other_columns_0_0 };

const PulseDataTableColumnDesc pulse_table_deep_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_deep_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("inner").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<::pulse_tables::PulseInner>>::value).offset(16).struct_name("inner").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_deep_columns_0 = { pulse_table_deep_columns_0_0, pulse_table_deep_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_deep_columns_1_0 = pulse::datatable::ColumnDescBuilder{}.name("power").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(0).default_float(1.0).build();
const PulseDataTableColumnDesc pulse_table_deep_columns_1_1 = pulse::datatable::ColumnDescBuilder{}.name("shell").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<::pulse_tables::PulseShell>>::value).offset(8).struct_name("shell").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_deep_columns_1 = { pulse_table_deep_columns_1_0, pulse_table_deep_columns_1_1 };

const PulseDataTableColumnDesc pulse_table_deep_columns_2_0 = pulse::datatable::ColumnDescBuilder{}.name("radius").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(0).default_int(INT64_C(2)).build();
const PulseDataTableColumnDesc pulse_table_deep_columns_2_1 = pulse::datatable::ColumnDescBuilder{}.name("tag").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(8).default_string("x").build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_deep_columns_2 = { pulse_table_deep_columns_2_0, pulse_table_deep_columns_2_1 };

const PulseDataTableColumnDesc pulse_table_dup_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const std::array<PulseDataTableColumnDesc, 1> pulse_table_dup_columns_0 = { pulse_table_dup_columns_0_0 };

const PulseDataTableColumnDesc pulse_table_enm_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_enm_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("kind").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(16).default_string("alpha").enum_name("kind_values").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_ENUM).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_enm_columns_0 = { pulse_table_enm_columns_0_0, pulse_table_enm_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_extra_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const std::array<PulseDataTableColumnDesc, 1> pulse_table_extra_columns_0 = { pulse_table_extra_columns_0_0 };

const PulseDataTableColumnDesc pulse_table_hero_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_hero_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("element").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(16).enum_name("element").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_ENUM).build();
const PulseDataTableColumnDesc pulse_table_hero_columns_0_2 = pulse::datatable::ColumnDescBuilder{}.name("power").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(32).default_float(1.0).build();
const std::array<PulseDataTableColumnDesc, 3> pulse_table_hero_columns_0 = { pulse_table_hero_columns_0_0, pulse_table_hero_columns_0_1, pulse_table_hero_columns_0_2 };

const PulseDataTableColumnDesc pulse_table_item_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_item_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("name").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(16).default_string("unnamed").build();
const PulseDataTableColumnDesc pulse_table_item_columns_0_2 = pulse::datatable::ColumnDescBuilder{}.name("weight").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(32).min(0.0).max(5.0).default_float(0.5).build();
const std::array<PulseDataTableColumnDesc, 3> pulse_table_item_columns_0 = { pulse_table_item_columns_0_0, pulse_table_item_columns_0_1, pulse_table_item_columns_0_2 };

const PulseDataTableColumnDesc pulse_table_num_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_num_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("title").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(8).default_string("none").build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_num_columns_0 = { pulse_table_num_columns_0_0, pulse_table_num_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_ref_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_ref_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("link").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<const ::pulse_tables::PulseItemRow*>>::value).offset(16).ref_name("item").build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_ref_columns_0 = { pulse_table_ref_columns_0_0, pulse_table_ref_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_refdefault_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_refdefault_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("link").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<const ::pulse_tables::PulseItemRow*>>::value).offset(16).ref_name("item").build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_refdefault_columns_0 = { pulse_table_refdefault_columns_0_0, pulse_table_refdefault_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_req_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_req_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("required").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(16).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_req_columns_0 = { pulse_table_req_columns_0_0, pulse_table_req_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_rng_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_rng_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("value").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(16).min(0.0).max(10.0).default_float(1.0).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_rng_columns_0 = { pulse_table_rng_columns_0_0, pulse_table_rng_columns_0_1 };

const PulseDataTableColumnDesc pulse_table_snake_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("interval").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(16).min(0.10000000000000001).max(10.0).default_float(1.0).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_2 = pulse::datatable::ColumnDescBuilder{}.name("hp").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(24).min(1.0).default_int(INT64_C(100)).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_3 = pulse::datatable::ColumnDescBuilder{}.name("big").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<bool>>::value).offset(32).default_bool(false).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_4 = pulse::datatable::ColumnDescBuilder{}.name("drop").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<const ::pulse_tables::PulseItemRow*>>::value).offset(40).ref_name("item").build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_5 = pulse::datatable::ColumnDescBuilder{}.name("skill").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<::pulse_tables::PulseSkill>>::value).offset(48).struct_name("skill").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_0_6 = pulse::datatable::ColumnDescBuilder{}.name("price").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(64).enum_name("price_values").column_type(PULSE_DATA_TABLE_COLUMN_TYPE_ENUM).build();
const std::array<PulseDataTableColumnDesc, 7> pulse_table_snake_columns_0 = { pulse_table_snake_columns_0_0, pulse_table_snake_columns_0_1, pulse_table_snake_columns_0_2, pulse_table_snake_columns_0_3, pulse_table_snake_columns_0_4, pulse_table_snake_columns_0_5, pulse_table_snake_columns_0_6 };

const PulseDataTableColumnDesc pulse_table_snake_columns_1_0 = pulse::datatable::ColumnDescBuilder{}.name("power").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(0).default_float(1.0).build();
const PulseDataTableColumnDesc pulse_table_snake_columns_1_1 = pulse::datatable::ColumnDescBuilder{}.name("radius").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(8).default_int(INT64_C(1)).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_snake_columns_1 = { pulse_table_snake_columns_1_0, pulse_table_snake_columns_1_1 };

const PulseDataTableColumnDesc pulse_table_typ_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_typ_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("count").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<int64_t>>::value).offset(16).default_int(INT64_C(0)).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_typ_columns_0 = { pulse_table_typ_columns_0_0, pulse_table_typ_columns_0_1 };

bool pulse_table_declares_other_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_deep_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_deep_fill_1(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_deep_fill_2(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_dup_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_enm_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_extra_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_hero_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_item_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_num_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_ref_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_refdefault_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_req_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_rng_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_snake_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_snake_fill_1(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);
bool pulse_table_typ_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);

const PulseDataTableStructDesc pulse_table_deep_struct_0{"inner", 32u, 8u, pulse_table_deep_columns_1.data(), 2u};
const PulseDataTableStructDesc pulse_table_deep_struct_1{"shell", 24u, 8u, pulse_table_deep_columns_2.data(), 2u};

const PulseDataTableStructDesc pulse_table_snake_struct_0{"skill", 16u, 8u, pulse_table_snake_columns_1.data(), 2u};

constexpr const PulseDataTableSchemaDesc* pulse_table_declares_other_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_declares_other_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "declares_other",
    pulse_table_declares_other_columns_0.data(),
    1u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_declares_other_fill_0
};


const PulseDataTableStructDesc pulse_table_deep_structs[] = {
    pulse_table_deep_struct_0,
    pulse_table_deep_struct_1,
};

constexpr const PulseDataTableSchemaDesc* pulse_table_deep_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_deep_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "deep",
    pulse_table_deep_columns_0.data(),
    2u,
    pulse_table_deep_structs,
    2u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_deep_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_dup_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_dup_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "dup",
    pulse_table_dup_columns_0.data(),
    1u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_dup_fill_0
};

const char* pulse_table_enm_values_kind_values[] = {
    "alpha",
    "beta",
};
const PulseDataTableEnumDesc pulse_table_enm_enum_kind_values{"kind_values", pulse_table_enm_values_kind_values, 2u};


const PulseDataTableEnumDesc pulse_table_enm_enums[] = {
    pulse_table_enm_enum_kind_values,
};

constexpr const PulseDataTableSchemaDesc* pulse_table_enm_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_enm_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "enm",
    pulse_table_enm_columns_0.data(),
    2u,
    nullptr,
    0u,
    pulse_table_enm_enums,
    1u,
    0u,
    false,
    pulse_table_enm_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_extra_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_extra_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "extra",
    pulse_table_extra_columns_0.data(),
    1u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_extra_fill_0
};

const char* pulse_table_hero_enum_names_element[] = {
    "fire",
    "water",
    "earth",
};
const PulseDataTableEnumDesc pulse_table_hero_enum_element{"element", pulse_table_hero_enum_names_element, 3u};

const PulseDataTableEnumDesc pulse_table_hero_enums[] = {
    pulse_table_hero_enum_element,
};

constexpr const PulseDataTableSchemaDesc* pulse_table_hero_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_hero_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "hero",
    pulse_table_hero_columns_0.data(),
    3u,
    nullptr,
    0u,
    pulse_table_hero_enums,
    1u,
    0u,
    false,
    pulse_table_hero_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_item_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_item_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "item",
    pulse_table_item_columns_0.data(),
    3u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_item_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_num_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_num_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "num",
    pulse_table_num_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    true,
    pulse_table_num_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_ref_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_ref_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "ref",
    pulse_table_ref_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_ref_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_refdefault_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_refdefault_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "refdefault",
    pulse_table_refdefault_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_refdefault_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_req_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_req_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "req",
    pulse_table_req_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_req_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_rng_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_rng_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "rng",
    pulse_table_rng_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_rng_fill_0
};

const char* pulse_table_snake_values_price_values[] = {
    "cheap",
    "normal",
    "pricey",
};
const PulseDataTableEnumDesc pulse_table_snake_enum_price_values{"price_values", pulse_table_snake_values_price_values, 3u};


const PulseDataTableStructDesc pulse_table_snake_structs[] = {
    pulse_table_snake_struct_0,
};

const PulseDataTableEnumDesc pulse_table_snake_enums[] = {
    pulse_table_snake_enum_price_values,
};

constexpr const PulseDataTableSchemaDesc* pulse_table_snake_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_snake_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "snake",
    pulse_table_snake_columns_0.data(),
    7u,
    pulse_table_snake_structs,
    1u,
    pulse_table_snake_enums,
    1u,
    0u,
    false,
    pulse_table_snake_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_typ_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_typ_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "typ",
    pulse_table_typ_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_typ_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_declares_other_schema_ptr() { return &pulse_table_declares_other_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_deep_schema_ptr() { return &pulse_table_deep_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_dup_schema_ptr() { return &pulse_table_dup_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_enm_schema_ptr() { return &pulse_table_enm_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_extra_schema_ptr() { return &pulse_table_extra_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_hero_schema_ptr() { return &pulse_table_hero_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_item_schema_ptr() { return &pulse_table_item_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_num_schema_ptr() { return &pulse_table_num_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_ref_schema_ptr() { return &pulse_table_ref_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_refdefault_schema_ptr() { return &pulse_table_refdefault_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_req_schema_ptr() { return &pulse_table_req_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_rng_schema_ptr() { return &pulse_table_rng_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_snake_schema_ptr() { return &pulse_table_snake_schema; }
constexpr const PulseDataTableSchemaDesc* pulse_table_typ_schema_ptr() { return &pulse_table_typ_schema; }

bool pulse_table_declares_other_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_declares_other_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseDeclaresOtherRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_declares_other_columns_0[0], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_deep_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_deep_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseDeepRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_deep_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "inner");
        if (!value) {
            row->inner = ::pulse_tables::PulseInner{1.0F, ::pulse_tables::PulseShell{INT64_C(2), std::string_view("x")}};
        } else if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'inner' expects a nested table";
            return false;
        } else {
            if (!pulse_table_deep_fill_1(context, vault_data, value, &row->inner, error_line, error_code, out_error)) {
                return false;
            }
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_deep_fill_1(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_deep_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseInner*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "power");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'power' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_deep_columns_1[0], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "shell");
        if (!value) {
            row->shell = ::pulse_tables::PulseShell{INT64_C(2), std::string_view("x")};
        } else if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'shell' expects a nested table";
            return false;
        } else {
            if (!pulse_table_deep_fill_2(context, vault_data, value, &row->shell, error_line, error_code, out_error)) {
                return false;
            }
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_deep_fill_2(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_deep_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseShell*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "radius");
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 2;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'radius' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_deep_columns_2[0], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "tag");
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'tag' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("x");
        pulse_data_table_field_set_string(row, &pulse_table_deep_columns_2[1], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_dup_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_dup_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseDupRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_dup_columns_0[0], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_enm_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_enm_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseEnmRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_enm_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "kind");
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'kind' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("alpha");
        pulse_data_table_field_set_string(row, &pulse_table_enm_columns_0[1], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_extra_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_extra_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseExtraRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_extra_columns_0[0], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_hero_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_hero_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseHeroRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_hero_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "element");
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'element' expects an enum name";
            return false;
        }
        const char* raw = value ? pulse_datalist_get_string(value, nullptr, "") : "water";
        if (pulse_data_table_enum_lookup(owner, &pulse_table_hero_columns_0[1], raw) < 0) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_INVALID_ENUM_VALUE;
            *out_error = "column 'element' is not in the enum whitelist";
            return false;
        }
        std::string_view decoded = vault->append(std::string_view(raw));
        pulse_data_table_field_set_string(row, &pulse_table_hero_columns_0[1], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "power");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'power' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_hero_columns_0[2], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_item_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_item_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseItemRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_item_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "name");
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'name' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("unnamed");
        pulse_data_table_field_set_string(row, &pulse_table_item_columns_0[1], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "weight");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 0.5;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'weight' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_item_columns_0[2], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_num_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_num_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseNumRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 0;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_num_columns_0[0], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "title");
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'title' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("none");
        pulse_data_table_field_set_string(row, &pulse_table_num_columns_0[1], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_ref_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_ref_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseRefRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_ref_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "link");
        const void* resolved = pulse_data_table_fill_context_resolve_ref(context, &pulse_table_ref_columns_0[1], value, out_error);
        if (value && !resolved) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE;
            return false;
        }
        row->link = resolved ? reinterpret_cast<const ::pulse_tables::PulseItemRow*>(resolved) : nullptr;
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_refdefault_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_refdefault_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseRefdefaultRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_refdefault_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "link");
        const void* resolved = pulse_data_table_fill_context_resolve_ref(context, &pulse_table_refdefault_columns_0[1], value, out_error);
        if (value && !resolved) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE;
            return false;
        }
        row->link = resolved ? reinterpret_cast<const ::pulse_tables::PulseItemRow*>(resolved) : nullptr;
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_req_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_req_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseReqRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_req_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "required");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'required' is missing";
            return false;
        }
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 0;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'required' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_req_columns_0[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_rng_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_rng_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseRngRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_rng_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "value");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'value' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_rng_columns_0[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_snake_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_snake_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseSnakeRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_snake_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "interval");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'interval' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_snake_columns_0[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "hp");
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 100;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'hp' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_snake_columns_0[2], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "big");
        bool decoded = value ? pulse_datalist_get_bool(value, nullptr, false) : false;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_BOOL) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'big' expects a bool";
            return false;
        }
        pulse_data_table_field_set_bool(row, &pulse_table_snake_columns_0[3], decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "drop");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'drop' is missing";
            return false;
        }
        const void* resolved = pulse_data_table_fill_context_resolve_ref(context, &pulse_table_snake_columns_0[4], value, out_error);
        if (value && !resolved) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE;
            return false;
        }
        row->drop = resolved ? reinterpret_cast<const ::pulse_tables::PulseItemRow*>(resolved) : nullptr;
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "skill");
        if (!value) {
            row->skill = ::pulse_tables::PulseSkill{1.0F, INT64_C(1)};
        } else if (pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'skill' expects a nested table";
            return false;
        } else {
            if (!pulse_table_snake_fill_1(context, vault_data, value, &row->skill, error_line, error_code, out_error)) {
                return false;
            }
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "price");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'price' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'price' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_snake_columns_0[6], &decoded, out_error);
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_snake_fill_1(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_snake_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseSkill*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "power");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'power' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_snake_columns_1[0], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "radius");
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 1;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'radius' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_snake_columns_1[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}

bool pulse_table_typ_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_typ_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseTypRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_typ_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "count");
        int64_t decoded = value ? pulse_datalist_get_int(value, nullptr, 0) : 0;
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_INT) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'count' expects an int";
            return false;
        }
        if (!pulse_data_table_field_set_int(row, &pulse_table_typ_columns_0[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}


} // namespace

namespace pulse_tables
{

const char* pulse_table_declares_other_path = nullptr;
const char* pulse_table_deep_path = nullptr;
const char* pulse_table_dup_path = nullptr;
const char* pulse_table_enm_path = nullptr;
const char* pulse_table_extra_path = nullptr;
const char* pulse_table_hero_path = nullptr;
const char* pulse_table_item_path = nullptr;
const char* pulse_table_num_path = nullptr;
const char* pulse_table_ref_path = nullptr;
const char* pulse_table_refdefault_path = nullptr;
const char* pulse_table_req_path = nullptr;
const char* pulse_table_rng_path = nullptr;
const char* pulse_table_snake_path = nullptr;
const char* pulse_table_typ_path = nullptr;

const char* PulseDeclaresOtherRowTable::DefaultPath() {
    return "declares_other.datatable";
}

PulseAssetRequest PulseDeclaresOtherRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_declares_other_path = path;
    }
    const char* resolved = pulse_table_declares_other_path ? pulse_table_declares_other_path : DefaultPath();
    pulse_table_declares_other_path = resolved;
    return pulse_data_table_system_load(system, "declares_other", resolved);
}

bool PulseDeclaresOtherRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseDeclaresOtherRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseDeclaresOtherRow* PulseDeclaresOtherRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseDeclaresOtherRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseDeclaresOtherRow* PulseDeclaresOtherRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseDeclaresOtherRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseDeepRowTable::DefaultPath() {
    return "deep.datatable";
}

PulseAssetRequest PulseDeepRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_deep_path = path;
    }
    const char* resolved = pulse_table_deep_path ? pulse_table_deep_path : DefaultPath();
    pulse_table_deep_path = resolved;
    return pulse_data_table_system_load(system, "deep", resolved);
}

bool PulseDeepRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseDeepRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseDeepRow* PulseDeepRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseDeepRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseDeepRow* PulseDeepRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseDeepRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseDupRowTable::DefaultPath() {
    return "dup.datatable";
}

PulseAssetRequest PulseDupRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_dup_path = path;
    }
    const char* resolved = pulse_table_dup_path ? pulse_table_dup_path : DefaultPath();
    pulse_table_dup_path = resolved;
    return pulse_data_table_system_load(system, "dup", resolved);
}

bool PulseDupRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseDupRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseDupRow* PulseDupRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseDupRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseDupRow* PulseDupRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseDupRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseEnmRowTable::DefaultPath() {
    return "enm.datatable";
}

PulseAssetRequest PulseEnmRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_enm_path = path;
    }
    const char* resolved = pulse_table_enm_path ? pulse_table_enm_path : DefaultPath();
    pulse_table_enm_path = resolved;
    return pulse_data_table_system_load(system, "enm", resolved);
}

bool PulseEnmRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseEnmRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseEnmRow* PulseEnmRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseEnmRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseEnmRow* PulseEnmRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseEnmRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseExtraRowTable::DefaultPath() {
    return "extra.datatable";
}

PulseAssetRequest PulseExtraRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_extra_path = path;
    }
    const char* resolved = pulse_table_extra_path ? pulse_table_extra_path : DefaultPath();
    pulse_table_extra_path = resolved;
    return pulse_data_table_system_load(system, "extra", resolved);
}

bool PulseExtraRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseExtraRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseExtraRow* PulseExtraRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseExtraRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseExtraRow* PulseExtraRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseExtraRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseHeroRowTable::DefaultPath() {
    return "hero.datatable";
}

PulseAssetRequest PulseHeroRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_hero_path = path;
    }
    const char* resolved = pulse_table_hero_path ? pulse_table_hero_path : DefaultPath();
    pulse_table_hero_path = resolved;
    return pulse_data_table_system_load(system, "hero", resolved);
}

bool PulseHeroRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseHeroRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseHeroRow* PulseHeroRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseHeroRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseHeroRow* PulseHeroRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseHeroRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseItemRowTable::DefaultPath() {
    return "item.datatable";
}

PulseAssetRequest PulseItemRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_item_path = path;
    }
    const char* resolved = pulse_table_item_path ? pulse_table_item_path : DefaultPath();
    pulse_table_item_path = resolved;
    return pulse_data_table_system_load(system, "item", resolved);
}

bool PulseItemRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseItemRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseItemRow* PulseItemRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseItemRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseItemRow* PulseItemRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseItemRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseNumRowTable::DefaultPath() {
    return "num.datatable";
}

PulseAssetRequest PulseNumRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_num_path = path;
    }
    const char* resolved = pulse_table_num_path ? pulse_table_num_path : DefaultPath();
    pulse_table_num_path = resolved;
    return pulse_data_table_system_load(system, "num", resolved);
}

bool PulseNumRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseNumRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseNumRow* PulseNumRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseNumRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseNumRow* PulseNumRowTable::GetRow(PulseAppId app, int64_t key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseNumRow*>(pulse_data_table_find_row_int(table, key)) : nullptr;
}

const char* PulseRefRowTable::DefaultPath() {
    return "ref.datatable";
}

PulseAssetRequest PulseRefRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_ref_path = path;
    }
    const char* resolved = pulse_table_ref_path ? pulse_table_ref_path : DefaultPath();
    pulse_table_ref_path = resolved;
    return pulse_data_table_system_load(system, "ref", resolved);
}

bool PulseRefRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseRefRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseRefRow* PulseRefRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseRefRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseRefRow* PulseRefRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseRefRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseRefdefaultRowTable::DefaultPath() {
    return "refdefault.datatable";
}

PulseAssetRequest PulseRefdefaultRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_refdefault_path = path;
    }
    const char* resolved = pulse_table_refdefault_path ? pulse_table_refdefault_path : DefaultPath();
    pulse_table_refdefault_path = resolved;
    return pulse_data_table_system_load(system, "refdefault", resolved);
}

bool PulseRefdefaultRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseRefdefaultRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseRefdefaultRow* PulseRefdefaultRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseRefdefaultRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseRefdefaultRow* PulseRefdefaultRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseRefdefaultRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseReqRowTable::DefaultPath() {
    return "req.datatable";
}

PulseAssetRequest PulseReqRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_req_path = path;
    }
    const char* resolved = pulse_table_req_path ? pulse_table_req_path : DefaultPath();
    pulse_table_req_path = resolved;
    return pulse_data_table_system_load(system, "req", resolved);
}

bool PulseReqRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseReqRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseReqRow* PulseReqRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseReqRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseReqRow* PulseReqRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseReqRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseRngRowTable::DefaultPath() {
    return "rng.datatable";
}

PulseAssetRequest PulseRngRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_rng_path = path;
    }
    const char* resolved = pulse_table_rng_path ? pulse_table_rng_path : DefaultPath();
    pulse_table_rng_path = resolved;
    return pulse_data_table_system_load(system, "rng", resolved);
}

bool PulseRngRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseRngRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseRngRow* PulseRngRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseRngRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseRngRow* PulseRngRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseRngRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseSnakeRowTable::DefaultPath() {
    return "snake.datatable";
}

PulseAssetRequest PulseSnakeRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_snake_path = path;
    }
    const char* resolved = pulse_table_snake_path ? pulse_table_snake_path : DefaultPath();
    pulse_table_snake_path = resolved;
    return pulse_data_table_system_load(system, "snake", resolved);
}

bool PulseSnakeRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseSnakeRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseSnakeRow* PulseSnakeRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseSnakeRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseSnakeRow* PulseSnakeRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseSnakeRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

const char* PulseTypRowTable::DefaultPath() {
    return "typ.datatable";
}

PulseAssetRequest PulseTypRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_typ_path = path;
    }
    const char* resolved = pulse_table_typ_path ? pulse_table_typ_path : DefaultPath();
    pulse_table_typ_path = resolved;
    return pulse_data_table_system_load(system, "typ", resolved);
}

bool PulseTypRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseTypRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseTypRow* PulseTypRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseTypRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseTypRow* PulseTypRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseTypRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

EPulseResult RegisterSchemas(PulseDataTableSystemId system) {
    if (!system) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_declares_other_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_deep_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_dup_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_enm_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_extra_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_hero_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_item_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_num_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_ref_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_refdefault_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_req_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_rng_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_snake_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_typ_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    return PULSE_RESULT_OK;
}
} // namespace pulse_tables
