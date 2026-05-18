#include "app.h"

namespace pulse {

App::App() {
    setup_phases();
}

App::~App() {
    for (auto& pair : subapps_) {
        delete pair.second;
    }
    subapps_.clear();
}

void App::setup_phases() {
    world_.component<InitPhase>();
    world_.component<UpdatePhase>();
    world_.component<PostUpdatePhase>();
    world_.component<RenderPhase>();
    world_.component<ImguiPhase>();

    auto init_ent    = world_.entity().add<InitPhase>();
    auto update_ent  = world_.entity().add<UpdatePhase>();
    auto post_ent    = world_.entity().add<PostUpdatePhase>();
    auto render_ent  = world_.entity().add<RenderPhase>();
    auto imgui_ent   = world_.entity().add<ImguiPhase>();

    update_ent.add(flecs::DependsOn, init_ent);
    post_ent.add(flecs::DependsOn, update_ent);
    render_ent.add(flecs::DependsOn, post_ent);
    imgui_ent.add(flecs::DependsOn, render_ent);
}

void App::set_name(const char* name) {
    name_ = name ? name : "";
}

const char* App::get_name() const {
    return name_.empty() ? nullptr : name_.c_str();
}

void App::add_plugin(const pulse_plugin_desc& desc) {
    add_plugin_to_registry(desc);
    RegisteredPlugin& entry = plugins_.back();
    if (desc.build) {
        desc.build(reinterpret_cast<pulse_app_t>(this), desc.ctx);
    }
    entry.build_done = true;
}

void App::add_plugin_to_registry(const pulse_plugin_desc& desc) {
    RegisteredPlugin entry;
    entry.desc = desc;
    plugins_.push_back(entry);
}

bool App::is_plugin_added(const char* name) const {
    if (!name) return false;
    for (const auto& p : plugins_) {
        if (p.desc.name && std::string(p.desc.name) == name) {
            return true;
        }
    }
    return false;
}

void App::poll_ready() {
    bool all_ready;
    do {
        all_ready = true;
        for (auto& p : plugins_) {
            if (!p.ready_done) {
                if (p.desc.ready) {
                    if (p.desc.ready(reinterpret_cast<pulse_app_t>(this), p.desc.ctx)) {
                        p.ready_done = true;
                    } else {
                        all_ready = false;
                    }
                } else {
                    p.ready_done = true;
                }
            }
        }
    } while (!all_ready);
    state_ = AppState::Ready;
}

void App::call_finish() {
    for (auto& p : plugins_) {
        if (!p.finish_done && p.desc.finish) {
            p.desc.finish(reinterpret_cast<pulse_app_t>(this), p.desc.ctx);
        }
        p.finish_done = true;
    }
    state_ = AppState::Finished;
}

void App::call_cleanup() {
    for (auto& p : plugins_) {
        if (!p.cleanup_done && p.desc.cleanup) {
            p.desc.cleanup(reinterpret_cast<pulse_app_t>(this), p.desc.ctx);
        }
        p.cleanup_done = true;
    }
    state_ = AppState::Cleaned;
}

void App::default_runner() {
    update();
}

void App::run() {
    poll_ready();
    call_finish();
    call_cleanup();

    if (runner_fn_) {
        runner_fn_(reinterpret_cast<pulse_app_t>(this), runner_ctx_);
    } else {
        default_runner();
    }
}

void App::update() {
    world_.progress();
    for (auto& pair : subapps_) {
        pair.second->update();
    }
}

void App::set_runner(pulse_runner_fn runner, void* ctx) {
    runner_fn_ = runner;
    runner_ctx_ = ctx;
}

void App::insert_subapp(const char* name, App* subapp) {
    subapps_[name] = subapp;
}

App* App::get_subapp(const char* name) {
    auto it = subapps_.find(name);
    return it != subapps_.end() ? it->second : nullptr;
}

App* App::remove_subapp(const char* name) {
    auto it = subapps_.find(name);
    if (it != subapps_.end()) {
        App* sub = it->second;
        subapps_.erase(it);
        return sub;
    }
    return nullptr;
}

} // namespace pulse

// ============================================================
// C API implementation
// ============================================================

extern "C" {

static inline pulse::App* to_app(pulse_app_t handle) {
    return reinterpret_cast<pulse::App*>(handle);
}

pulse_app_t pulse_app_create(void) {
    return reinterpret_cast<pulse_app_t>(new pulse::App());
}

void pulse_app_destroy(pulse_app_t app) {
    delete to_app(app);
}

void pulse_app_run(pulse_app_t app) {
    to_app(app)->run();
}

void pulse_app_update(pulse_app_t app) {
    to_app(app)->update();
}

void pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx) {
    to_app(app)->set_runner(runner, ctx);
}

void pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc) {
    to_app(app)->add_plugin(*desc);
}

void pulse_app_add_system(pulse_app_t app, ecs_entity_t phase, ecs_system_desc_t* desc) {
    ecs_world_t* world = to_app(app)->world_c();
    ecs_entity_t sys = ecs_system(world, desc);
    ecs_add_id(world, sys, phase);
}

void pulse_app_insert_resource(pulse_app_t app, ecs_entity_t component, size_t size, const void* data) {
    ecs_world_t* world = to_app(app)->world_c();
    ecs_entity_t singleton = ecs_singleton(world, &(ecs_singleton_desc_t){
        .entity = ecs_new(world),
        .type = component,
        .type_size = size,
        .type_alignment = 0,
    });
    ecs_set_id(world, singleton, component, size, data);
}

void pulse_app_init_resource(pulse_app_t app, ecs_entity_t component, size_t size, void (*ctor)(void* out)) {
    ecs_world_t* world = to_app(app)->world_c();
    ecs_entity_t ent = ecs_singleton_get(world, component);
    if (ent == 0 || !ecs_has(world, ent, component)) {
        ent = ecs_singleton(world, &(ecs_singleton_desc_t){
            .entity = ecs_new(world),
            .type = component,
            .type_size = size,
            .type_alignment = 0,
        });
        if (ctor) {
            std::vector<char> buffer(size);
            ctor(buffer.data());
            ecs_set_id(world, ent, component, size, buffer.data());
        }
    }
}

ecs_world_t* pulse_app_world(pulse_app_t app) {
    return to_app(app)->world_c();
}

pulse_app_t pulse_subapp_create(const char* name) {
    auto* sub = new pulse::App();
    if (name) sub->set_name(name);
    return reinterpret_cast<pulse_app_t>(sub);
}

void pulse_subapp_destroy(pulse_app_t subapp) {
    delete to_app(subapp);
}

void pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp) {
    to_app(app)->insert_subapp(name, to_app(subapp));
}

pulse_app_t pulse_app_get_subapp(pulse_app_t app, const char* name) {
    auto* sub = to_app(app)->get_subapp(name);
    return reinterpret_cast<pulse_app_t>(sub);
}

void pulse_app_remove_subapp(pulse_app_t app, const char* name) {
    to_app(app)->remove_subapp(name);
}

bool pulse_app_is_plugin_added(pulse_app_t app, const char* name) {
    return to_app(app)->is_plugin_added(name);
}

int pulse_app_plugins_state(pulse_app_t app) {
    return static_cast<int>(to_app(app)->plugins_state());
}

} // extern "C"
