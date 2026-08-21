#include <flecs.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "pulse_app.h"
#include "app_internal.h"

import pulse_app;

ECS_COMPONENT_DECLARE(PulseTimer);
ECS_COMPONENT_DECLARE(pulse_app_state_resource);

namespace {

pulse::App* to_app(PulseAppId handle) {
    return reinterpret_cast<pulse::App*>(handle);
}

PulseAppId to_handle(pulse::App* app) {
    return reinterpret_cast<PulseAppId>(app);
}

pulse::Result to_result(EPulseResult result) {
    return static_cast<pulse::Result>(result);
}

EPulseResult to_c(pulse::Result result) {
    return static_cast<EPulseResult>(result);
}

pulse::AppDesc to_desc(const PulseAppDesc* desc) {
    pulse::AppDesc out;
    out.name = desc->name ? desc->name : "";
    out.enable_rest_api = desc->enable_restapi;
    return out;
}

} // namespace

extern "C" {

PulseAppId pulse_create_app(const PulseAppDesc* desc) {
    if (!desc) {
        return nullptr;
    }
    return to_handle(new pulse::App(to_desc(desc)));
}

void pulse_destroy_app(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    delete impl;
}

EPulseResult pulse_app_run(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_c(impl->run());
}

EPulseResult pulse_app_prepare(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_c(impl->prepare());
}

EPulseResult pulse_app_update(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_c(impl->update());
}

void pulse_app_teardown(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    impl->teardown();
}

void pulse_app_finish(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    impl->finish();
}

bool pulse_app_should_quit(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->should_quit() : true;
}

EPulseResult pulse_app_set_runner(PulseAppId app, PulseProcRunnerFn runner, void* ctx) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse::Runner wrapped;
    if (runner) {
        wrapped = [app, runner](pulse::App&, void* c) {
            return to_result(runner(app, c));
        };
    }
    return to_c(impl->set_runner(std::move(wrapped), ctx));
}

EPulseResult pulse_app_add_plugin(PulseAppId app, const PulsePluginDesc* desc) {
    pulse::App* impl = to_app(app);
    if (!impl || !desc) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (desc->struct_size != sizeof(PulsePluginDesc)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc->version != PULSE_PLUGIN_DESC_VERSION) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    PulseProcPluginBuildFn build = desc->build;
    PulseProcPluginBuildFn post_build = desc->post_build;
    PulseProcPluginShutdownFn shutdown = desc->shutdown;

    pulse::Plugin plugin;
    plugin.name = desc->name ? desc->name : "";
    plugin.ctx = desc->ctx;
    if (build) {
        plugin.build = [app, build](pulse::App&, void* c) {
            return to_result(build(app, c));
        };
    }
    if (post_build) {
        plugin.post_build = [app, post_build](pulse::App&, void* c) {
            return to_result(post_build(app, c));
        };
    }
    if (shutdown) {
        plugin.shutdown = [app, shutdown](pulse::App&, void* c) {
            shutdown(app, c);
        };
    }

    return to_c(impl->add_plugin(std::move(plugin)));
}

bool pulse_app_has_plugin(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->has_plugin(name ? name : "") : false;
}

ecs_world_t* pulse_app_world(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->world() : nullptr;
}

const char* pulse_app_last_error(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->last_error().data() : "invalid app";
}

EPulseResult pulse_app_insert_subapp(PulseAppId app, const char* name, PulseAppId subapp) {
    if (app == subapp) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    pulse::App* impl = to_app(app);
    if (!impl || !subapp) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto subapp_cpp = std::unique_ptr<pulse::App>(reinterpret_cast<pulse::App*>(subapp));
    auto result = impl->try_insert_subapp(name ? name : "", subapp_cpp);
    if (result != pulse::Result::Ok) subapp_cpp.release();
    return to_c(result);
}

PulseAppId pulse_app_get_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return nullptr;
    }
    return to_handle(impl->get_subapp(name ? name : ""));
}

PulseAppId pulse_app_remove_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return nullptr;
    }

    std::unique_ptr<pulse::App> removed = impl->remove_subapp(name ? name : "");
    return to_handle(removed.release());
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

    pulse::SubappExtract wrapped;
    if (extract) {
        wrapped = [app, extract](pulse::App&, pulse::App& subapp, void* c) {
            return to_result(extract(app, to_handle(&subapp), c));
        };
    }

    return to_c(impl->set_subapp_extract(name ? name : "", std::move(wrapped), ctx));
}

PulseAppId pulse_get_app_from_world(ecs_world_t* world) {
    if (!world) {
        return nullptr;
    }

    const pulse_app_state_resource* resource = ecs_singleton_get(world, pulse_app_state_resource);
    return resource ? to_handle(reinterpret_cast<pulse::App*>(resource->app)) : nullptr;
}

} // extern "C"
