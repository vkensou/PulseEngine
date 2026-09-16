#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_datalist.h"
#include "pulse_datatable.h"

#include "tables_generated.h"

static void update_once(PulseAppId app) {
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
}

static bool wait_ready(PulseAppId app, PulseAssetRequest request) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    PulseAssetSystemId assets = pulse_get_asset_system(app);
    for (int frame = 0; frame < 600; ++frame) {
        if (pulse_data_table_system_is_ready(system, request)) {
            return true;
        }
        if (pulse_asset_system_get_state(assets, request) == PULSE_ASSET_STATE_FAILED) {
            return false;
        }
        if (!pulse_data_table_system_is_alive(system, request)) {
            return false;
        }
        update_once(app);
    }
    return false;
}

static void assert_load_failed(PulseAppId app, PulseAssetRequest request, const char* expected) {
    if (wait_ready(app, request)) {
        printf("FAIL: '%s' loaded but should have failed\n", expected);
        assert(false);
    }
    const char* error = pulse_asset_system_get_error(pulse_get_asset_system(app), request);
    if (!error || !strstr(error, expected)) {
        printf("FAIL: error was '%s', expected substring '%s'\n", error ? error : "(null)", expected);
        assert(false);
    }
}

static void expect_failure(PulseAppId app, const char* path, const char* expected) {
    PulseAssetRequest request = pulse_data_table_system_load(pulse_get_data_table_system(app), "snake", path);
    assert(pulse_asset_request_is_valid(request));
    assert_load_failed(app, request, expected);
}

static void test_parse_errors(void) {
    static const char duplicate[] = "schema: \"dup\"\nrows: [\n    { id: a }\n    { id: a }\n]\n";
    PulseDatalist* node = pulse_datalist_create_from_text(duplicate, sizeof(duplicate) - 1);
    assert(node != nullptr);
    PulseDatalist* rows = pulse_datalist_get_obj(node, "rows");
    assert(pulse_datalist_line(pulse_datalist_get(rows, 0)) == 3);
    assert(pulse_datalist_line(pulse_datalist_get(rows, 1)) == 4);
    pulse_datalist_release(node);
}

int main(void) {
    test_parse_errors();

    PulseAppDesc app_desc = { .name = "test-datatable-basic" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("tests/datatable/schema", "/", true));

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseDataTablePluginDesc table_desc = pulse_data_table_plugin_desc_default();
    assert(table_desc.struct_size == sizeof(PulseDataTablePluginDesc));
    assert(table_desc.version == PULSE_DATA_TABLE_PLUGIN_DESC_VERSION);
    assert(pulse_add_data_table_plugin(app, &table_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_data_table_plugin(app, &table_desc) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    assert(system != nullptr);

    assert(pulse_data_table_system_get_schema(system, "missing") == nullptr);
    assert(pulse_data_table_system_get_schema(system, "snake") == nullptr);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    assert(pulse_tables::RegisterSchemas(system) == PULSE_RESULT_OK);
    assert(pulse_tables::RegisterSchemas(system) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);

    const PulseDataTableSchemaDesc* schema = pulse_data_table_system_get_schema(system, "snake");
    assert(schema != nullptr);
    assert(schema->columns_count == 7);
    assert(schema->key_column == 0);
    assert(!schema->key_is_int);
    assert(schema->p_columns[6].offset == 64);

    PulseAssetRequest snake_request = pulse_tables::PulseSnakeRowTable::Load(app, "snake.datatable");
    assert(pulse_asset_request_is_valid(snake_request));
    assert(wait_ready(app, snake_request));
    assert(pulse_tables::PulseSnakeRowTable::IsReady(app));

    uint32_t count = 0;
    const pulse_tables::PulseSnakeRow* rows = pulse_tables::PulseSnakeRowTable::Rows(app, count);
    assert(rows != nullptr && count == 2);

    const pulse_tables::PulseSnakeRow* green = pulse_tables::PulseSnakeRowTable::GetRow(app, "green");
    assert(green != nullptr);
    assert(green->id == "green");
    assert(green->interval > 2.49 && green->interval < 2.51);
    assert(green->hp == 10);
    assert(green->big);
    assert(green->skill.power > 3.49 && green->skill.power < 3.51);
    assert(green->skill.radius == 2);
    assert(green->price == "cheap");

    const pulse_tables::PulseSnakeRow* blue = pulse_tables::PulseSnakeRowTable::GetRow(app, "blue");
    assert(blue != nullptr);
    assert(blue->interval > 0.99 && blue->interval < 1.01);
    assert(blue->hp == 100);
    assert(!blue->big);
    assert(blue->skill.power > 0.99 && blue->skill.power < 1.01);
    assert(blue->skill.radius == 1);
    assert(blue->price == "pricey");

    assert(green->drop != nullptr);
    assert(green->drop->id == "apple");
    assert(green->drop->name == "Apple");
    assert(green->drop->weight > 0.29 && green->drop->weight < 0.31);
    assert(blue->drop != nullptr);
    assert(blue->drop->id == "rock");

    const pulse_tables::PulseItemRow* apple = pulse_tables::PulseItemRowTable::GetRow(app, "apple");
    assert(apple != nullptr);
    assert(apple == green->drop);

    assert(pulse_tables::PulseSnakeRowTable::GetRow(app, "missing") == nullptr);
    PulseDataTableId snake_table = pulse_data_table_system_get(system, snake_request);

    PulseAssetRequest num_request = pulse_tables::PulseNumRowTable::Load(app, "num.datatable");
    assert(pulse_asset_request_is_valid(num_request));
    assert(wait_ready(app, num_request));
    uint32_t num_count = 0;
    const pulse_tables::PulseNumRow* nums = pulse_tables::PulseNumRowTable::Rows(app, num_count);
    assert(nums != nullptr && num_count == 2);
    assert(nums[0].id == 7);
    assert(nums[1].id == 42);
    const pulse_tables::PulseNumRow* seven = pulse_tables::PulseNumRowTable::GetRow(app, 7);
    assert(seven != nullptr);
    assert(seven->title == "seven");
    const pulse_tables::PulseNumRow* forty_two = pulse_tables::PulseNumRowTable::GetRow(app, 42);
    assert(forty_two != nullptr);
    assert(forty_two->title == "none");
    assert(pulse_tables::PulseNumRowTable::GetRow(app, 8) == nullptr);
    PulseDataTableId num_table = pulse_data_table_system_get(system, num_request);
    assert(num_table != nullptr);
    assert(pulse_data_table_find_row(num_table, "7") == nullptr);
    assert(pulse_data_table_find_row_int(num_table, 7) == seven);
    assert(pulse_data_table_find_row_int(snake_table, 7) == nullptr);

    PulseAssetRequest deep_request = pulse_tables::PulseDeepRowTable::Load(app, "deep.datatable");
    assert(pulse_asset_request_is_valid(deep_request));
    assert(wait_ready(app, deep_request));
    const pulse_tables::PulseDeepRow* deep_a = pulse_tables::PulseDeepRowTable::GetRow(app, "a");
    assert(deep_a != nullptr);
    assert(deep_a->inner.power > 4.49 && deep_a->inner.power < 4.51);
    assert(deep_a->inner.shell.radius == 9);
    assert(deep_a->inner.shell.tag == "deep");
    const pulse_tables::PulseDeepRow* deep_b = pulse_tables::PulseDeepRowTable::GetRow(app, "b");
    assert(deep_b != nullptr);
    assert(deep_b->inner.power > 0.99 && deep_b->inner.power < 1.01);
    assert(deep_b->inner.shell.radius == 2);
    assert(deep_b->inner.shell.tag == "x");

    PulseAssetRequest hero_request = pulse_tables::PulseHeroRowTable::Load(app, "hero.datatable");
    assert(pulse_asset_request_is_valid(hero_request));
    assert(wait_ready(app, hero_request));
    const pulse_tables::PulseHeroRow* mage = pulse_tables::PulseHeroRowTable::GetRow(app, "mage");
    assert(mage != nullptr);
    assert(mage->element == "fire");
    assert(mage->power > 3.49 && mage->power < 3.51);
    const pulse_tables::PulseHeroRow* knight = pulse_tables::PulseHeroRowTable::GetRow(app, "knight");
    assert(knight != nullptr);
    assert(knight->element == "water");
    assert(knight->power > 0.99 && knight->power < 1.01);
    const PulseDataTableSchemaDesc* hero_schema = pulse_data_table_system_get_schema(system, "hero");
    assert(hero_schema != nullptr);
    assert(hero_schema->enums_count == 1);
    assert(hero_schema->p_enums[0].values_count == 3);
    assert(strcmp(hero_schema->p_enums[0].name, "element") == 0);
    assert(strcmp(hero_schema->p_enums[0].p_values[0], "fire") == 0);

    assert(pulse_data_table_system_get(system, pulse_asset_request_make_invalid()) == nullptr);
    assert(pulse_data_table_get_name(nullptr) == nullptr);
    assert(pulse_data_table_row_count(nullptr) == 0);
    assert(pulse_data_table_rows(nullptr, nullptr) == nullptr);
    assert(snake_table != nullptr);
    assert(strcmp(pulse_data_table_get_name(snake_table), "snake") == 0);
    assert(pulse_data_table_row_count(snake_table) == 2);
    assert(pulse_data_table_find_row_int(snake_table, 1) == nullptr);
    expect_failure(app, "req.datatable", "is missing");
    expect_failure(app, "typ.datatable", "expects an int");
    expect_failure(app, "rng.datatable", "above the schema maximum");
    expect_failure(app, "enm.datatable", "enum whitelist");
    expect_failure(app, "dup.datatable", "duplicate primary key");
    expect_failure(app, "ref.datatable", "referenced row does not exist");
    expect_failure(app, "extra.datatable", "unknown column");
    expect_failure(app, "declares_other.datatable", "is not registered");

    PulseAssetRequest empty_request = pulse_data_table_system_load(system, "snake", "no_such_file.datatable");
    assert(pulse_asset_request_is_valid(empty_request));
    assert(!wait_ready(app, empty_request));

    pulse_app_finish(app);
    pulse_destroy_app(app);
    printf("test-datatable-basic: ok\n");
    return 0;
}
