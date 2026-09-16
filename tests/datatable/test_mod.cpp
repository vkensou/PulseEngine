#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_datatable.h"

#include "tables_generated.h"

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
        if (pulse_app_update(app) != PULSE_APP_UPDATE_RESULT_OK) {
            return false;
        }
    }
    return false;
}

int main(void) {
    PulseAppDesc app_desc = { .name = "test-datatable-mod" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("tests/datatable/schema", "/", true));
    assert(pulse_vfs_mount("tests/datatable/mod", "/", false));

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseDataTablePluginDesc table_desc = pulse_data_table_plugin_desc_default();
    assert(pulse_add_data_table_plugin(app, &table_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    assert(pulse_tables::RegisterSchemas(system) == PULSE_RESULT_OK);

    PulseAssetRequest snake_request = pulse_tables::PulseSnakeRowTable::Load(app, "snake.datatable");
    assert(wait_ready(app, snake_request));

    uint32_t count = 0;
    const pulse_tables::PulseSnakeRow* rows = pulse_tables::PulseSnakeRowTable::Rows(app, count);
    assert(rows != nullptr && count == 1);

    const pulse_tables::PulseSnakeRow* green = pulse_tables::PulseSnakeRowTable::GetRow(app, "green");
    assert(green != nullptr);
    assert(green->interval > 8.99 && green->interval < 9.01);
    assert(green->hp == 3);
    assert(!green->big);
    assert(green->skill.power > 7.99 && green->skill.power < 8.01);
    assert(green->skill.radius == 6);
    assert(green->price == "pricey");

    assert(green->drop != nullptr);
    assert(green->drop->id == "apple");
    assert(green->drop->name == "Apple");

    assert(pulse_tables::PulseSnakeRowTable::GetRow(app, "blue") == nullptr);

    pulse_app_finish(app);
    pulse_destroy_app(app);
    printf("test-datatable-mod: ok\n");
    return 0;
}
