#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static const PulseDataTableSchemaDesc* cpp_probe_schema() {
    static const PulseDataTableColumnDesc columns[] = {
        {"id", PULSE_DATA_TABLE_COLUMN_TYPE_STRING, 0, 0.0, 0.0, false, false, false, 0, 0.0, false, nullptr, nullptr, nullptr, nullptr},
        {"weight", PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, 0, 0.0, 5.0, false, true, true, 0, 0.5, false, nullptr, nullptr, nullptr, nullptr},
    };
    static const PulseDataTableSchemaDesc desc{
        sizeof(PulseDataTableSchemaDesc),
        PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
        "cpp_probe",
        columns,
        2,
        nullptr,
        0,
        nullptr,
        0,
        0,
        false,
        nullptr,
    };
    return &desc;
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
    assert(system != nullptr);

    {
        const PulseDataTableSchemaDesc* src = cpp_probe_schema();
        const char* error = nullptr;
        assert(pulse_data_table_system_register_schema(system, src, &error) == PULSE_RESULT_OK);
        assert(error == nullptr);
        assert(src->p_columns[0].offset == 0u);
        assert(src->p_columns[1].offset == 0u);
        const PulseDataTableSchemaDesc* owned = pulse_data_table_system_get_schema(system, "cpp_probe");
        assert(owned != nullptr && owned != src);
        assert(owned->fill_row == nullptr);
        assert(owned->key_is_int == false);
        assert(owned->p_columns[0].offset == 0u);
        assert(owned->p_columns[1].offset == 16u);
    }

    {
        const char* error = nullptr;
        assert(pulse_data_table_system_register_schema(system, cpp_probe_schema(), &error) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);
        assert(error != nullptr);
        PulseDataTableSchemaDesc bad = *cpp_probe_schema();
        bad.name = "cpp_probe_key_out_of_range";
        bad.key_column = 9;
        assert(pulse_data_table_system_register_schema(system, &bad, &error) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
        assert(error != nullptr);
        PulseDataTableColumnDesc duplicated[2] = { cpp_probe_schema()->p_columns[0], cpp_probe_schema()->p_columns[0] };
        bad = *cpp_probe_schema();
        bad.name = "cpp_probe_duplicate_column";
        bad.p_columns = duplicated;
        assert(pulse_data_table_system_register_schema(system, &bad, &error) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
        assert(error != nullptr);
        assert(pulse_data_table_system_get_schema(system, "cpp_probe_key_out_of_range") == nullptr);
        assert(pulse_data_table_system_get_schema(system, "cpp_probe_duplicate_column") == nullptr);
    }

    ecs_world_t* world = pulse_app_world(app);
    int probe_count = 0;
    for (int frame = 0; frame < 600 && probe_count == 0; ++frame) {
        ecs_id_t probe_id = ecs_lookup(world, "DasTableProbe");
        if (probe_id != 0) {
            ecs_query_desc_t query_desc = {};
            query_desc.terms[0].id = probe_id;
            ecs_query_t* query = ecs_query_init(world, &query_desc);
            assert(query != nullptr);
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
            ecs_query_fini(query);
        }
        if (probe_count == 0) {
            assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
        }
    }
    assert(probe_count == 1);

    {
        const PulseDataTableSchemaDesc* snake = pulse_data_table_system_get_schema(system, "snake");
        assert(snake != nullptr);
        assert(snake->p_columns[0].offset == offsetof(pulse_tables::PulseSnakeRow, id));
        assert(snake->p_columns[1].offset == offsetof(pulse_tables::PulseSnakeRow, interval));
        assert(snake->p_columns[2].offset == offsetof(pulse_tables::PulseSnakeRow, hp));
        assert(snake->p_columns[3].offset == offsetof(pulse_tables::PulseSnakeRow, big));
        assert(snake->p_columns[4].offset == offsetof(pulse_tables::PulseSnakeRow, drop));
        assert(snake->p_columns[5].offset == offsetof(pulse_tables::PulseSnakeRow, skill));
        assert(snake->p_columns[6].offset == offsetof(pulse_tables::PulseSnakeRow, price));
        assert(snake->p_structs[0].size == sizeof(pulse_tables::PulseSkill));
        assert(snake->p_structs[0].align == alignof(pulse_tables::PulseSkill));
    }

    pulse_app_finish(app);
    pulse_destroy_app(app);
    printf("test-datatable-daslang: ok\n");
    return 0;
}
