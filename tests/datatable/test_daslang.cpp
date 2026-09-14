#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_datatable.h"
#include "pulse_daslang.h"

#include "../daslang/daslang_inject_helper.h"
#include "tables_generated.h"

struct DasTableProbe
{
    int32_t value;
    double power;
    bool flag;
    int32_t text_code;
    int32_t error_code;
};

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
    PulseAppDesc app_desc = { .name = "test-datatable-daslang" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("src/pulse_daslang", "/", false));
    assert(pulse_vfs_mount("tests/datatable/das", "/", false));
    assert(pulse_vfs_mount("tests/datatable/schema", "/", true));

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseDataTablePluginDesc table_desc = pulse_data_table_plugin_desc_default();
    assert(pulse_add_data_table_plugin(app, &table_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseDaslangPluginDesc daslang_desc = pulse_daslang_plugin_desc_default();
    if (pulse_add_daslang_plugin(app, &daslang_desc) != PULSE_APP_ADD_PLUGIN_RESULT_OK) {
        pulse_destroy_app(app);
        return -1;
    }

    assert(daslang_inject_helper::inject_all_das(app) > 0);
    assert(pulse_load_module(app, "probe.das"));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    assert(pulse_tables::RegisterSchemas(system) == PULSE_RESULT_OK);

    assert(wait_ready(app, pulse_tables::PulseSnakeRowTable::Load(app, "snake.datatable")));
    assert(wait_ready(app, pulse_tables::PulseNumRowTable::Load(app, "num.datatable")));
    assert(wait_ready(app, pulse_tables::PulseDeepRowTable::Load(app, "deep.datatable")));

    ecs_world_t* world = pulse_app_world(app);
    ecs_id_t probe_id = 0;
    for (int frame = 0; frame < 300 && probe_id == 0; ++frame) {
        probe_id = ecs_lookup(world, "DasTableProbe");
        if (probe_id == 0) {
            assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
        }
    }
    assert(probe_id != 0);

    ecs_query_desc_t query_desc = {};
    query_desc.terms[0].id = probe_id;
    ecs_query_t* query = ecs_query_init(world, &query_desc);
    assert(query != nullptr);

    int probe_count = 0;
    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it))
    {
        const DasTableProbe* probes = static_cast<const DasTableProbe*>(ecs_field_w_size(&it, sizeof(DasTableProbe), 0));
        for (int i = 0; i < it.count; ++i)
        {
            assert(probes[i].value == 10024);
            assert(probes[i].power > 11.49 && probes[i].power < 11.51);
            assert(probes[i].flag);
            assert(probes[i].text_code == 0);
            assert(probes[i].error_code == 0);
            ++probe_count;
        }
    }
    assert(probe_count == 1);
    ecs_query_fini(query);

    pulse_app_finish(app);
    pulse_destroy_app(app);
    printf("test-datatable-daslang: ok\n");
    return 0;
}
