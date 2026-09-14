#pragma once

#include "pulse_datatable.h"
#include "pulse_datatable_schema.h"
#include "pulse_asset.h"
#include "pulse_datalist.h"

#include <cstdint>
#include <cstring>
#include <deque>
#include <memory_resource>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace pulse::datatable {

constexpr const char* kPluginName = "pulse_datatable";

struct TableSlot;
class Registry;

struct Table {
    const PulseDataTableSchemaDesc* schema = nullptr;
    Registry* registry = nullptr;
    TableSlot* slot = nullptr;
    std::string name{};
    std::vector<uint8_t> rows{};
    uint32_t row_count = 0;
    std::unordered_map<std::string, uint32_t> string_index{};
    std::unordered_map<int64_t, uint32_t> int_index{};
    StringVault vault{};
    std::string source_text{};

    void reset(const PulseDataTableSchemaDesc* schema_desc, Registry* owner, TableSlot* owner_slot);
};

struct RowKey {
    bool is_string = false;
    std::string_view text{};
    int64_t number = 0;
};

struct DependencyRef {
    std::string schema{};
    PulseAssetHandle handle{};
};

struct LoadRecord {
    Table* table = nullptr;
    std::vector<DependencyRef> dependencies{};
};

struct FillContext {
    void* owner = nullptr;
    uint32_t row = 0;
    const std::vector<DependencyRef>* dependencies = nullptr;
};

class Registry {
public:
    explicit Registry(std::pmr::memory_resource* resource);
    ~Registry();

    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;

    PulseAppId app() const;
    PulseAssetSystemId asset_system() const;
    std::pmr::memory_resource* resource() const;
    void bind(PulseAppId app_id, PulseAssetSystemId asset_system_id);

    EPulseResult register_schema(const PulseDataTableSchemaDesc* desc);
    const PulseDataTableSchemaDesc* find_schema(std::string_view name) const;

    Table* create_table(PulseAssetHandle handle, const PulseDataTableSchemaDesc* schema);
    void destroy_table(Table* table);
    void bind_slot(PulseAssetHandle handle, TableSlot* slot);
    void release_slot(TableSlot* slot);
    Table* find_table(std::string_view name) const;
    TableSlot* table_slot(std::string_view name) const;

    void add_load_record(PulseAssetHandle handle, LoadRecord&& record);
    LoadRecord* find_load_record(PulseAssetHandle handle);
    void remove_load_record(PulseAssetHandle handle);

    const char* intern_error(std::string_view message);

private:
    std::pmr::memory_resource* resource_ = nullptr;
    PulseAppId app_ = nullptr;
    PulseAssetSystemId asset_system_ = nullptr;
    std::pmr::unordered_map<std::pmr::string, const PulseDataTableSchemaDesc*> schemas_;
    std::unordered_map<uint32_t, Table*> slots_{};
    std::unordered_map<TableSlot*, Table*> live_{};
    std::vector<Table*> table_order_{};
    std::unordered_map<uint32_t, LoadRecord> load_records_{};
    std::deque<std::string> error_text_{};
};

const PulseDataTableColumnDesc* find_column(const PulseDataTableSchemaDesc* schema, std::string_view name);
const PulseDataTableStructDesc* find_struct(const PulseDataTableSchemaDesc* schema, std::string_view name);
const PulseDataTableEnumDesc* find_enum(const PulseDataTableSchemaDesc* schema, std::string_view name);

RowKey read_key(const Table& table, const void* row);

bool decode_int(const PulseDatalist* node, int64_t default_value, int64_t& out);
bool decode_float(const PulseDatalist* node, double default_value, double& out);
bool decode_bool(const PulseDatalist* node, bool default_value, bool& out);
bool decode_string(const PulseDatalist* node, const char* default_value, std::string_view& out);
bool decode_enum(const PulseDataTableEnumDesc* enum_desc, const PulseDatalist* node, std::string_view& out, const char*& message);

uint32_t table_row_size(const PulseDataTableSchemaDesc* schema);
const char* data_table_error_text(EPulseDataTableError error);

bool check_range(const PulseDataTableColumnDesc& column, double value, EPulseDataTableError& error, const char*& message);
bool check_column(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc& column, const PulseDatalist* node, EPulseDataTableError& error, const char*& message);

EPulseResult register_data_table_type(PulseAssetSystemId asset_system, Registry* registry);
EPulseResult register_data_table_loader(PulseAssetSystemId asset_system, Registry* registry);

EPulsePluginBuildResult data_table_plugin_build_callback(PulseAppId app, void* ctx);
EPulsePluginBuildResult data_table_plugin_post_build_callback(PulseAppId app, void* ctx);
void data_table_plugin_shutdown_callback(PulseAppId app, void* ctx);

struct TableSlot {
    const PulseDataTableSchemaDesc* schema = nullptr;
    const void* rows = nullptr;
    uint32_t row_count = 0;
    Table* table = nullptr;
    Registry* registry = nullptr;
};

} // namespace pulse::datatable

struct PulseDataTable;

class PulseDataTableSystem final {
public:
    PulseDataTableSystem();
    ~PulseDataTableSystem();

    PulseDataTableSystem(const PulseDataTableSystem&) = delete;
    PulseDataTableSystem& operator=(const PulseDataTableSystem&) = delete;

    pulse::datatable::Registry& registry();
    pulse::datatable::Registry* registry_ptr();

private:
    std::pmr::monotonic_buffer_resource memory_pool;
    pulse::datatable::Registry registry_;
};

