# PulseEngine Plugin 机制设计规格

> 状态: 已确认  
> 日期: 2026-05-18  
> 版本: 0.1

## 1. 目标与动机

在 PulseEngine 中实现类似 Bevy 的插件机制，达成以下目标：

- **统一初始化流程** — 用 `App::new().add_plugins(...).run()` 的 builder 模式替代手动 `oval_device_descriptor` 回调 + `_init_world()` 分配
- **插件可组合性** — 方便地启用/禁用/重排整个子系统
- **第三方插件生态** — 设计稳定的 C API，让外部模块无需了解引擎内部就能注册自己

## 2. 设计原则

- **渐进式**：新代码完全独立于 oval，两套体系并存，不碰 oval 任何代码
- **C API 对外**：所有能力通过 C API 暴露，C++ / C / daScript / 其他语言均通过同一套 C API 接入
- **App 接管一切**：App 是最顶层入口，内部持有 Flecs world，CGPU 渲染、窗口、ImGui 全部是 plugin
- **App 核心极薄**：只有 Plugin 注册表 + Flecs world + SubApp 注册表 + Runner 函数指针

## 3. MVP 范围

### 包含

| 特性 | 说明 |
|------|------|
| App 生命周期 | create / destroy / run / update |
| Plugin 注册 | `pulse_plugin_desc` + build/ready/finish/cleanup 生命周期 |
| SubApp | 创建/挂载/查询/移除子 App |
| Runner 机制 | 可替换的 runner，默认 `run_once`（只跑一帧） |
| System 注册 | 直接暴露 Flecs 的 `ecs_system_desc_t` |
| Resource 注册 | insert / init resource（基于 Flecs component entity） |
| Flecs pipeline | 沿用现有 Init/Update/PostUpdate/Render/Imgui phase |
| daScript 支持 | 通过 C API 的 FFI 绑定接入 |

### 不包含（后续版本）

| 特性 | 说明 |
|------|------|
| oval 代码改动 | 完全不碰 |
| 线程调度 | enkiTS 暂不引入 |
| Event 系统 | 后续按需加入 |
| `res<>` / `singleton_query<>` 等宏 | 由各语言自己封装 |
| daScript hot-reload | 后续 |
| Plugin 依赖自动排序 / 去重 | MVP 阶段显式加载，不做自动处理 |
| PluginGroup | 后续版本 |
| 嵌套 add_plugins | build 内不能调用 add_plugin |
| Render plugin 化 | 渲染模块通过头文件暴露，暂不做 plugin 封装 |

## 4. C API 设计

### 4.1 基础类型

```c
typedef uint64_t pulse_app_t;

typedef struct pulse_plugin_desc {
    void*        ctx;
    void       (*build)  (pulse_app_t app, void* ctx);
    bool       (*ready)  (pulse_app_t app, void* ctx);   // NULL 表示 true
    void       (*finish) (pulse_app_t app, void* ctx);   // NULL 表示空
    void       (*cleanup)(pulse_app_t app, void* ctx);   // NULL 表示空
    const char*  name;                                    // NULL 表示用类型名
    bool         unique;                                  // 默认 true
    const char** dependencies;                            // 依赖的 plugin name 列表
    int          dependency_count;
} pulse_plugin_desc;

typedef void (*pulse_runner_fn)(pulse_app_t app, void* ctx);
```

### 4.2 App 生命周期

```c
pulse_app_t  pulse_app_create(void);
void         pulse_app_destroy(pulse_app_t app);
void         pulse_app_run(pulse_app_t app);
void         pulse_app_update(pulse_app_t app);
```

### 4.3 Runner

```c
void pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx);
```

- 默认 runner = `run_once`：只调一次 `update()` 就返回
- 用户替换：例如 SDL event loop runner

### 4.4 Plugin 注册

```c
void pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc);
```

### 4.5 SubApp

```c
pulse_app_t  pulse_subapp_create(const char* name);
void         pulse_subapp_destroy(pulse_app_t subapp);
void         pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp);
pulse_app_t  pulse_app_get_subapp(pulse_app_t app, const char* name);
void         pulse_app_remove_subapp(pulse_app_t app, const char* name);
```

### 4.6 System & Resource（直通 Flecs）

```c
void  pulse_app_add_system(pulse_app_t app, ecs_entity_t phase, ecs_system_desc_t* desc);
void  pulse_app_insert_resource(pulse_app_t app, ecs_entity_t component, size_t size, const void* data);
void  pulse_app_init_resource(pulse_app_t app, ecs_entity_t component, size_t size, void (*ctor)(void* out));
ecs_world_t* pulse_app_world(pulse_app_t app);
```

### 4.7 查询

```c
bool  pulse_app_is_plugin_added(pulse_app_t app, const char* name);
int   pulse_app_plugins_state(pulse_app_t app);  // 0=Adding, 1=Ready, 2=Finished, 3=Cleaned
```

## 5. C++ 内部架构

### 5.1 App 内部组成

```
pulse::App
├── PluginRegistry  (std::vector<PluginDesc>, 按添加顺序)
├── Flecs World     (ECS)
├── SubAppRegistry  (std::unordered_map<string, App*>)
└── Runner          (函数指针 + ctx)
```

- `App::new()` → 创建 Flecs world + 默认 runner
- `App::empty()` → 仅创建空的 Flecs world
- 不含 CGPU、不含 enkiTS、不含 ImGui —— 这些都是 plugin

### 5.2 Plugin 生命周期流程

```
pulse_app_create()
  │
  ├─► pulse_app_add_plugin(&desc1)  → desc1.build() 立即调用
  ├─► pulse_app_add_plugin(&desc2)  → desc2.build() 立即调用
  │
  ▼
pulse_app_run(app)
  │
  ├─► 1. 遍历 plugin，调 ready() 直到全部返回 true
  ├─► 2. 遍历 plugin，调 finish()
  ├─► 3. 遍历 plugin，调 cleanup()
  ├─► 4. runner(app)  → 内部循环调 app.update()
  │     └─► update() 按 phase 顺序执行 Flecs pipeline
  └─► runner 返回 → run() 结束
```

- `build()` 在 add_plugin 时立即同步调用
- `ready()` / `finish()` / `cleanup()` 在 `run()` 时串行调用
- 不做嵌套 add_plugins（build 内部不能再调 add_plugin）

### 5.3 Plugin 注册表

- 简单 `std::vector` 存储，按添加顺序
- 不做去重检查（`pulse_plugin_desc.unique` 字段保留供后续版本使用，当前忽略）
- 不做依赖分析和排序（`pulse_plugin_desc.dependencies` 字段保留供后续版本使用，当前忽略）
- 不做 Plugin 实例存活性管理（插件自己管理 ctx 生命周期）

## 6. daScript 集成

C API 全部导出后，daScript 通过 FFI 绑定即可使用：

- `pulse_app_create` / `pulse_app_add_plugin` / `pulse_app_run` 等直接 bind 为 das 函数
- das 侧可封装 `PulsePlugin` class，内部构造 `pulse_plugin_desc`，填入 das lambda
- system 注册通过已有的 `dasFlecs` 绑定操作

C++ 侧不感知 daScript，只提供 C API。

## 7. 文件布局

全部新文件，与 oval 代码无交叠：

```
src/app/
  pulse_app.h         // C API 头文件（纯 C，可被任何语言 FFI）
  app.h               // C++ App 内部声明
  app.cpp             // C++ App 实现
  plugin.h            // C++ PluginBase 辅助类（方便 C++ 用户继承使用）
```

## 8. 未决项

以下内容在本 spec 范围内已明确推迟到后续版本：

- 线程调度（enkiTS 集成）
- Event 系统（EventCenter plugin 化）
- `res<>` / `singleton_query<>` 等宏与 plugin 机制的整合
- daScript hot-reload 在 plugin 机制下的工作方式
- 嵌套 add_plugins（build 内部调用 add_plugin）
- Plugin 去重与依赖自动排序
- SubApp 的 extract 机制
