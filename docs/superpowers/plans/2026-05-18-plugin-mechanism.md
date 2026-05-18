# Plugin Mechanism Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a Bevy-like plugin mechanism with C API for the PulseEngine.

**Architecture:** New `src/app/` directory with `pulse_app.h` (C API), `app.h` (C++ App class), `app.cpp` (implementation), and `plugin.h` (C++ PluginBase helper). App internally holds a Flecs world, plugin registry (vector in insertion order), subapp registry, and runner function pointer. C API functions cast `pulse_app_t` (uint64_t) to `pulse::App*`.

**Tech Stack:** C++20, Flecs 4.1.5 (bundled), xmake build system. No external test framework — use a standalone test .cpp file.

---

## File Structure

```
src/app/
  pulse_app.h     // C API header (pure C, can be FFI'd by any language)
  app.h           // C++ App class declaration (internal)
  app.cpp         // C++ App implementation + C API glue
  plugin.h        // C++ PluginBase convenience class

tests/
  test_plugin.cpp // Standalone test for the plugin mechanism
```

**App internal structure:**
```
pulse::App
├── flecs::world world_
├── std::vector<RegisteredPlugin> plugins_
├── std::unordered_map<std::string, pulse::App*> subapps_
├── pulse_runner_fn runner_fn_ = nullptr
├── void* runner_ctx_ = nullptr
├── enum State { Adding, Ready, Finished, Cleaned } state_ = Adding
└── std::string name_ (for subapps)
```

---

### Task 1: Create test file

**Files:**
- Create: `tests/test_plugin.cpp`

- [ ] **Step 1: Write the test**

```c
#include <stdio.h>
#include <assert.h>

#define flecs_STATIC
#include <flecs.h>

#include "pulse_app.h"

static bool build_called = false;
static bool ready_called = false;
static bool finish_called = false;
static bool cleanup_called = false;
static bool system_called = false;
static int resource_value = 0;

// Flecs system callback
static void my_system_callback(ecs_iter_t* it) {
    system_called = true;
    int* val = ecs_field(it, int, 1);
    if (val) *val += 1;
}

static void my_build(pulse_app_t app, void* ctx) {
    build_called = true;
    ecs_world_t* world = pulse_app_world(app);

    // Register component type for int
    ecs_entity_t int_type = ecs_component(world, &(ecs_component_desc_t){
        .entity = ecs_new(world),
        .type = { .size = sizeof(int), .alignment = ECS_ALIGNOF(int) }
    });

    // Insert a resource with initial value 0
    int initial = 0;
    pulse_app_insert_resource(app, int_type, sizeof(int), &initial);

    // Add a system that increments the resource
    ecs_entity_t sys = ecs_system(world, &(ecs_system_desc_t){
        .query = { .terms = {
            { .id = int_type, .inout = EcsInOut },
        }},
        .callback = my_system_callback,
    });
    (void)sys;
}

static bool my_ready(pulse_app_t app, void* ctx) {
    ready_called = true;
    return true;
}

static void my_finish(pulse_app_t app, void* ctx) {
    finish_called = true;
}

static void my_cleanup(pulse_app_t app, void* ctx) {
    cleanup_called = true;
}

// Custom runner that runs update N times
static void test_runner(pulse_app_t app, void* ctx) {
    int* frames = (int*)ctx;
    for (int i = 0; i < *frames; ++i) {
        pulse_app_update(app);
    }
}

int main(void) {
    // 1. Create App
    pulse_app_t app = pulse_app_create();
    assert(app != 0);

    // 2. Add plugin
    pulse_plugin_desc desc = {
        .build = my_build,
        .ready = my_ready,
        .finish = my_finish,
        .cleanup = my_cleanup,
        .name = "TestPlugin",
    };
    pulse_app_add_plugin(app, &desc);

    // build() must be called immediately on add_plugin
    assert(build_called);

    // 3. Set runner
    int frames = 1;
    pulse_app_set_runner(app, test_runner, &frames);

    // 4. Run the app
    pulse_app_run(app);

    // 5. Verify lifecycle
    assert(ready_called);
    assert(finish_called);
    assert(cleanup_called);
    assert(system_called);

    // 6. Query APIs
    assert(pulse_app_is_plugin_added(app, "TestPlugin"));
    assert(pulse_app_plugins_state(app) == 3); // Cleaned

    // 7. Verify update works standalone
    pulse_app_update(app);
    assert(system_called); // still true

    // 8. SubApp
    pulse_app_t sub = pulse_subapp_create("Sub");
    assert(sub != 0);
    pulse_app_insert_subapp(app, "Sub", sub);
    pulse_app_t got = pulse_app_get_subapp(app, "Sub");
    assert(got == sub);
    pulse_app_remove_subapp(app, "Sub");
    assert(pulse_app_get_subapp(app, "Sub") == 0);
    pulse_subapp_destroy(sub);

    // 9. Cleanup
    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: Create empty stub files so test compiles against something (will fail link)**

Create `src/app/pulse_app.h` with minimal stubs:
```c
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t pulse_app_t;

typedef struct pulse_plugin_desc {
    void*        ctx;
    void       (*build)  (pulse_app_t app, void* ctx);
    bool       (*ready)  (pulse_app_t app, void* ctx);
    void       (*finish) (pulse_app_t app, void* ctx);
    void       (*cleanup)(pulse_app_t app, void* ctx);
    const char*  name;
    bool         unique;
    const char** dependencies;
    int          dependency_count;
} pulse_plugin_desc;

typedef void (*pulse_runner_fn)(pulse_app_t app, void* ctx);

pulse_app_t  pulse_app_create(void);
void         pulse_app_destroy(pulse_app_t app);
void         pulse_app_run(pulse_app_t app);
void         pulse_app_update(pulse_app_t app);
void         pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx);

void         pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc);

void         pulse_app_add_system(pulse_app_t app, ecs_entity_t phase, ecs_system_desc_t* desc);
void         pulse_app_insert_resource(pulse_app_t app, ecs_entity_t component, size_t size, const void* data);
void         pulse_app_init_resource(pulse_app_t app, ecs_entity_t component, size_t size, void (*ctor)(void* out));
ecs_world_t* pulse_app_world(pulse_app_t app);

pulse_app_t  pulse_subapp_create(const char* name);
void         pulse_subapp_destroy(pulse_app_t subapp);
void         pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp);
pulse_app_t  pulse_app_get_subapp(pulse_app_t app, const char* name);
void         pulse_app_remove_subapp(pulse_app_t app, const char* name);

bool         pulse_app_is_plugin_added(pulse_app_t app, const char* name);
int          pulse_app_plugins_state(pulse_app_t app);
```

- [ ] **Step 3: Verify the test compiles and links against empty stubs**

This step verifies the header is correct but will result in linker errors for the function implementations (expected — they don't exist yet).

---

### Task 2: Create C++ App class header

**Files:**
- Create: `src/app/app.h`

- [ ] **Step 1: Write the App class declaration**

```cpp
#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

#define flecs_STATIC
#include <flecs.h>

extern "C" {
#include "pulse_app.h"
}

namespace pulse {

enum class AppState {
    Adding = 0,
    Ready  = 1,
    Finished = 2,
    Cleaned = 3,
};

struct RegisteredPlugin {
    pulse_plugin_desc desc;
    bool build_done   = false;
    bool ready_done   = false;
    bool finish_done  = false;
    bool cleanup_done = false;
};

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void add_plugin(const pulse_plugin_desc& desc);
    void add_plugin_to_registry(const pulse_plugin_desc& desc);
    void run();
    void update();

    void set_runner(pulse_runner_fn runner, void* ctx);
    void set_name(const char* name);
    const char* get_name() const;

    flecs::world& world() { return world_; }
    ecs_world_t* world_c() { return world_.c_ptr(); }

    bool is_plugin_added(const char* name) const;
    AppState plugins_state() const { return state_; }

    void insert_subapp(const char* name, App* subapp);
    App* get_subapp(const char* name);
    App* remove_subapp(const char* name);

    // Phase tag components registered automatically
    struct InitPhase {};
    struct UpdatePhase {};
    struct PostUpdatePhase {};
    struct RenderPhase {};
    struct ImguiPhase {};

private:
    flecs::world world_;
    std::vector<RegisteredPlugin> plugins_;
    std::unordered_map<std::string, App*> subapps_;
    pulse_runner_fn runner_fn_ = nullptr;
    void* runner_ctx_ = nullptr;
    AppState state_ = AppState::Adding;
    std::string name_;

    void setup_phases();
    void poll_ready();
    void call_finish();
    void call_cleanup();
    void default_runner();
};

} // namespace pulse
```

---

### Task 3: Create C++ PluginBase helper

**Files:**
- Create: `src/app/plugin.h`

- [ ] **Step 1: Write the PluginBase class**

```cpp
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

    // Convert to C plugin descriptor. Caller keeps PluginBase alive.
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
```

---

### Task 4: Implement App class

**Files:**
- Create: `src/app/app.cpp`

- [ ] **Step 1: Implement App constructor and setup_phases**

```cpp
#include "app.h"

namespace pulse {

App::App() {
    setup_phases();
}

App::~App() {
    for (auto& [name, sub] : subapps_) {
        delete sub;
    }
    subapps_.clear();
}

void App::setup_phases() {
    // Register phase tag components in Flecs so they can be used with ecs_system_desc_t
    world_.component<InitPhase>();
    world_.component<UpdatePhase>();
    world_.component<PostUpdatePhase>();
    world_.component<RenderPhase>();
    world_.component<ImguiPhase>();

    // Set up phase dependencies so Flecs orders them correctly
    // Init -> Update -> PostUpdate -> Render -> Imgui
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
```

- [ ] **Step 2: Implement plugin management**

```cpp
void App::add_plugin(const pulse_plugin_desc& desc) {
    add_plugin_to_registry(desc);

    // Immediately call build()
    RegisteredPlugin& entry = plugins_.back();
    if (desc.build) {
        desc.build(reinterpret_cast<pulse_app_t>(this), desc.ctx);
    }
    entry.build_done = true;
}

void App::add_plugin_to_registry(const pulse_plugin_desc& desc) {
    RegisteredPlugin entry;
    entry.desc = desc;
    // Copy name if provided
    if (desc.name) {
        entry.desc.name = desc.name;
    }
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
```

- [ ] **Step 3: Implement lifecycle methods**

```cpp
void App::poll_ready() {
    bool all_ready = true;
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
```

- [ ] **Step 4: Implement run, update, runner**

```cpp
void App::default_runner() {
    // run_once: just call update() once
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
    // Also update subapps
    for (auto& [name, sub] : subapps_) {
        sub->update();
    }
}

void App::set_runner(pulse_runner_fn runner, void* ctx) {
    runner_fn_ = runner;
    runner_ctx_ = ctx;
}
```

- [ ] **Step 5: Implement SubApp methods**

```cpp
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
```

- [ ] **Step 6: Implement C API glue functions**

```cpp
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
    // Create system in Flecs and add the phase tag
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
        // Create with constructor
        ent = ecs_singleton(world, &(ecs_singleton_desc_t){
            .entity = ecs_new(world),
            .type = component,
            .type_size = size,
            .type_alignment = 0,
        });
        if (ctor) {
            void* buffer = alloca(size);
            ctor(buffer);
            ecs_set_id(world, ent, component, size, buffer);
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
```

---

### Task 5: Integrate into xmake build system

**Files:**
- Modify: `xmake.lua` (add new target)

- [ ] **Step 1: Add `app` library target and test target to xmake.lua**

Add after the `rgframework` target definition (after line 66):

```lua
target("pulse_app")
    set_kind("static")
    add_includedirs("src/rgframework/include", {public = true})
    add_includedirs("src/app", {public = true})
    add_headerfiles("src/app/*.h")
    add_files("src/app/app.cpp")
    add_files("src/rgframework/src/flecs.c")

target("test_plugin")
    set_kind("binary")
    set_group("tests")
    add_deps("pulse_app")
    add_files("tests/test_plugin.cpp")
```

- [ ] **Step 2: Verify build**

Run: `xmake build test_plugin`
Expected: compilation succeeds, link succeeds.

- [ ] **Step 3: Run test**

Run: `xmake run test_plugin`
Expected: `All tests passed!`

- [ ] **Step 4: Commit**

```bash
git add src/app/pulse_app.h src/app/app.h src/app/app.cpp src/app/plugin.h tests/test_plugin.cpp xmake.lua
git commit -m "feat: add plugin mechanism with C API"
```

---

### Task 6: Verify with xmake and commit

- [ ] **Step 1: Full build and run**

```bash
xmake build test_plugin
xmake run test_plugin
```

Expected: `All tests passed!`

- [ ] **Step 2: Commit all changes**

```bash
git add src/app/ tests/ xmake.lua
git commit -m "feat: implement plugin mechanism (App + C API)"
```
