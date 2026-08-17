import pulse_app;

#include "pulse_app.h"
#include "app_internal.h"

ECS_COMPONENT_DECLARE(PulseTimer);
ECS_COMPONENT_DECLARE(pulse_app_state_resource);

namespace {

pulse::App* to_app(PulseAppId handle) {
    return handle ? &handle->impl : nullptr;
}

} // namespace

extern "C" {

PulseAppId pulse_create_app(PulseAppDesc* desc) {
    return new PulseApp(desc);
}

void pulse_destroy_app(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }

    impl->shutdown();
    delete app;
}

EPulseResult pulse_app_run(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->run();
}

EPulseResult pulse_app_update(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->update();
}

EPulseResult pulse_app_set_runner(PulseAppId app, PulseProcRunnerFn runner, void* ctx) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_runner(runner, ctx);
}

EPulseResult pulse_app_add_plugin(PulseAppId app, const PulsePluginDesc* desc) {
    pulse::App* impl = to_app(app);
    if (!impl || !desc) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->add_plugin(*desc);
}

bool pulse_app_has_plugin(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->has_plugin(name) : false;
}

ecs_world_t* pulse_app_world(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->world_c() : nullptr;
}

const char* pulse_app_last_error(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->last_error() : "invalid app";
}

EPulseResult pulse_app_insert_subapp(PulseAppId app, const char* name, PulseAppId subapp) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->insert_subapp(name, subapp);
}

PulseAppId pulse_app_get_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->get_subapp(name) : nullptr;
}

PulseAppId pulse_app_remove_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->remove_subapp(name) : nullptr;
}

EPulseResult pulse_app_set_subapp_extract(
    PulseAppId app,
    const char* name,
    PulseProcSubappExtractFn extract,
    void* ctx
) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_subapp_extract(name, extract, ctx);
}

EPulseResult pulse_app_extract_subapps(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->extract_subapps();
}

PULSE_API PulseAppId pulse_get_app_from_world(ecs_world_t* world) {
    if (world == nullptr) return nullptr;
    const pulse_app_state_resource* resource = ecs_singleton_get(world, pulse_app_state_resource);
    return resource->app;
}

} // extern "C"
