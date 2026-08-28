#include "test_common.h"

#include <cstring>
#include <string>
#include <vector>

static std::vector<std::string> g_build_order;
static std::vector<std::string> g_shutdown_order;

static EPulsePluginBuildResult rec_build(PulseAppId app, void* ctx) {
    (void)app;
    g_build_order.push_back(static_cast<const char*>(ctx));
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

static void rec_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    g_shutdown_order.push_back(static_cast<const char*>(ctx));
}

static PulsePluginDesc make_plugin(
    const char* name,
    const char* build_name,
    uint32_t dependency_count,
    const char** dependencies)
{
    PulsePluginDesc desc = {};
    desc.struct_size = sizeof(PulsePluginDesc);
    desc.version = PULSE_PLUGIN_DESC_VERSION;
    desc.plugin_version = 1;
    desc.name = name;
    desc.ctx = const_cast<char*>(build_name);
    desc.build = rec_build;
    desc.shutdown = rec_shutdown;
    desc.dependency_count = dependency_count;
    desc.dependencies = dependencies;
    return desc;
}

static void test_arbitrary_order() {
    g_build_order.clear();

    static const char* b_deps[] = { "A" };

    PulseAppDesc app_desc = { .name = "dep-order" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePluginDesc b = make_plugin("B", "B", 1, b_deps);
    PulsePluginDesc a = make_plugin("A", "A", 0, nullptr);

    // Add dependent first; it must stay pending without failing.
    assert(pulse_app_add_plugin(app, &b) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_has_plugin(app, "A") == false);
    assert(pulse_app_has_plugin(app, "B") == false);

    assert(pulse_app_add_plugin(app, &a) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(g_build_order.size() == 2);
    assert(g_build_order[0] == "A");
    assert(g_build_order[1] == "B");

    pulse_destroy_app(app);
    printf("  arbitrary order ok\n");
}

static void test_missing_dependency() {
    PulseAppDesc app_desc = { .name = "dep-missing" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePluginDesc a = make_plugin("A", "A", 1, nullptr);
    static const char* missing_deps[] = { "DoesNotExist" };
    a.dependencies = missing_deps;

    assert(pulse_app_add_plugin(app, &a) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY);
    assert(strstr(pulse_app_last_error(app), "DoesNotExist") != nullptr);

    pulse_destroy_app(app);
    printf("  missing dependency ok\n");
}

static void test_circular_dependency() {
    PulseAppDesc app_desc = { .name = "dep-cycle" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    static const char* a_deps[] = { "B" };
    static const char* b_deps[] = { "A" };

    PulsePluginDesc a = make_plugin("A", "A", 1, a_deps);
    PulsePluginDesc b = make_plugin("B", "B", 1, b_deps);

    assert(pulse_app_add_plugin(app, &a) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    // Closing the cycle makes the pending set satisfiable, so the eager drain
    // detects and reports the cycle.
    assert(pulse_app_add_plugin(app, &b) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY);
    assert(strstr(pulse_app_last_error(app), "circular") != nullptr);

    pulse_destroy_app(app);
    printf("  circular dependency ok\n");
}

static void test_shutdown_reverse_order() {
    g_build_order.clear();
    g_shutdown_order.clear();

    static const char* a_deps[] = { "B" };

    PulseAppDesc app_desc = { .name = "dep-shutdown" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePluginDesc a = make_plugin("A", "A", 1, a_deps);
    PulsePluginDesc b = make_plugin("B", "B", 0, nullptr);

    assert(pulse_app_add_plugin(app, &a) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_add_plugin(app, &b) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // Build order is B then A (A depends on B).
    assert(g_build_order.size() == 2);
    assert(g_build_order[0] == "B");
    assert(g_build_order[1] == "A");

    pulse_app_prepare(app);
    pulse_app_teardown(app);

    // Shutdown runs in the reverse of build order: A first, then B.
    assert(g_shutdown_order.size() == 2);
    assert(g_shutdown_order[0] == "A");
    assert(g_shutdown_order[1] == "B");

    pulse_destroy_app(app);
    printf("  shutdown reverse ok\n");
}

int main(void) {
    test_arbitrary_order();
    test_missing_dependency();
    test_circular_dependency();
    test_shutdown_reverse_order();
    printf("Plugin dependency test passed!\n");
    return 0;
}