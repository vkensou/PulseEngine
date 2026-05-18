#pragma once

#include "pulse_app.h"

namespace pulse {

class PluginBase {
public:
    virtual ~PluginBase() = default;

    virtual void build(pulse_app_t app) = 0;
    virtual bool ready(pulse_app_t app) { return true; }
    virtual void finish(pulse_app_t app) {}
    virtual void cleanup(pulse_app_t app) {}
    virtual const char* name() { return nullptr; }

    void fill_desc(pulse_plugin_desc& desc) {
        desc.ctx = this;
        desc.build = &_build_thunk;
        desc.ready = &_ready_thunk;
        desc.finish = &_finish_thunk;
        desc.cleanup = &_cleanup_thunk;
        desc.name = name();
        desc.unique = true;
        desc.dependencies = nullptr;
        desc.dependency_count = 0;
    }

private:
    static void _build_thunk(pulse_app_t app, void* ctx) {
        static_cast<PluginBase*>(ctx)->build(app);
    }
    static bool _ready_thunk(pulse_app_t app, void* ctx) {
        return static_cast<PluginBase*>(ctx)->ready(app);
    }
    static void _finish_thunk(pulse_app_t app, void* ctx) {
        static_cast<PluginBase*>(ctx)->finish(app);
    }
    static void _cleanup_thunk(pulse_app_t app, void* ctx) {
        static_cast<PluginBase*>(ctx)->cleanup(app);
    }
};

} // namespace pulse
