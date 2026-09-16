# 包（Package）开发指南

PulseEngine 的功能单元。本文覆盖包模型、`package.json` 字段、插件接口、运行时加载语义，以及四类包的新建步骤。

本文是自足的：照本文写就能跑通，不需要再翻源码。遇到语义分歧时以源码为准，权威实现位置见 §12。

## 0. 先选路径

| 你要做的事 | 走哪一节 | 生成链 |
|---|---|---|
| 给引擎加一个可插拔的 native 功能（窗口、渲染、字体……） | §5 新建 native 可选包 | IDL → 公共头 + `package_register.cpp` |
| 扩展引擎底座（宿主起不来就缺它） | §6 新建核心包 | IDL → 公共头 |
| 用 C++ 写一个游戏 | §7 新建 native 游戏包 | `generate_module` → 模块 + 插件 |
| 只用 daslang 脚本写一个游戏 | §8 新增脚本内容包 | 无 C++、无 xmake target |

本文的代码片段是完整可用的，照着写就能跑通。更省事的做法是对着现有的同类包改：native 可选包看 `src/pulse_window/`（有 desc）与 `src/pulse_transform/`（无 desc），native 游戏包看 `examples/snake/`，脚本内容包看 `examples/snake_daslang/`。

## 1. 包模型

### 1.1 核心包 vs 可选包

根 `xmake.lua` 用 `set_group` 区分，这是两类包的唯一定义处。

| | 核心包 `set_group("core")` | 可选包 `set_group("pacakges")` |
|---|---|---|
| 成员 | `pulse_platform`、`pulse_app`、`pulse_vfs`、`pulse_config`、`pulse_datalist`（含 `pulse_datalist_static`）、`pulse_script_register`、`pulse_package_loader` | `pulse_math`、`pulse_window`、`pulse_input`、`pulse_asset`、`pulse_prefab`、`pulse_datatable`、`pulse_graphics`、`pulse_transform`、`pulse_renderer`、`pulse_imgui`、`pulse_daslang` |
| 位置 | `src/pulse_*` | `src/pulse_*` 或 `examples/<game>` |
| `package.json` | 无 | 有 |
| `package_register.cpp` | 无 | 有，生成物 |
| 加载方式 | launcher 的 `add_deps` 链接期依赖 | `dlopen` + `pulse_package_register` |
| `pulse.package_install` 规则 | 无 | 有 |

`pacakges` 是 `xmake.lua` 里的原文拼写（`packages` 之误），照抄，不要"修正"。

核心包是引擎最小底座，宿主没有它们就起不来，因此直接链进 launcher，不参与动态加载。

`cgpu`、`src/rendergraph`、`khr`、`dascript`、`tools/*` 不是包，是被包依赖的基础设施代码。`pulse_cpp_gameplay` 挂在 `pacakges` 分组下，但是 header-only 支撑库，没有 `package.json`，不被动态加载。

### 1.2 可选包的三种形态

| 形态 | 位置 | `package.json` type | 公共头文件 | 包注册实现 | 参考 |
|---|---|---|---|---|---|
| native 可选包 | `src/pulse_*` | `native`（默认） | IDL 生成 | `tools/gen_package_register` 生成 | `src/pulse_window` |
| native 游戏包 | `examples/<game>` | `native`（默认） | 模块头（手写） | `tools/generate_module` 生成 | `examples/snake` |
| 脚本内容包 | `examples/<game>` | `daslang` 等 | 无 | 无动态库，交脚本运行时 | `examples/snake_daslang` |

### 1.3 一个可选包由哪些文件组成

| 文件 | 手写/生成 | 作用 |
|---|---|---|
| `package.json` | 手写 | 包清单：名字、库名、依赖、资源 |
| `idl/<module>.idl` | 手写 | 公共接口定义（native 可选包） |
| `idl/temp.<module>.h` | 手写 | 公共头模板，含 `$` 占位符 |
| `include/<module>.h` | 生成 | 公共头，禁止手改 |
| `src/<module>_reflection.h` | 生成（可选） | component/tag 的 flecs 反射，禁止手改 |
| `src/package_register.cpp` | 生成 | 包入口 `pulse_package_register`，禁止手改 |
| `src/<module>_internal.h` | 手写 | 模块内部声明（插件 state、内部函数） |
| `src/<module>.cpp` | 手写 | 插件生命周期 + 公共 API 实现 |
| `src/<module>_components.cpp` | 手写（可选） | 组件注册与系统安装 |
| 根 `xmake.lua` 的 target | 手写 | 构建与安装规则 |
| `src/launcher/launcher.manifest.json` 条目 | 手写 | 让 launcher 加载它 |

## 2. 运行时加载

只适用于可选包；核心包在链接期就位。

### 2.1 流程

```
launcher.exe                      链 pulse_app / pulse_config / pulse_package_loader / pulse_vfs
  └─ 读 launcher.manifest.json → packages 列表（name + 可选 config）
  └─ 在 search roots 中定位 <root>/<name>/（或 <root>/<name>.zip），先命中者胜
  └─ 解析该目录的 package.json
  └─ 按 dependencies 拓扑排序
  └─ dlopen(<library>) → 取 entry 符号 → 调 pulse_package_register(app, config)
       └─ 包内部：pulse_add_<x>_plugin(app[, &desc]) → build 钩子注册组件/系统
```

### 2.2 搜索根与库定位

- **搜索根**：`launcher` 的每个位置参数是一个搜索根，按顺序查找 `<root>/<name>` 目录或 `<root>/<name>.zip`。`xmake run launcher` 通过 `set_runargs` 传的是仓库根的 `src` 与 `examples`（绝对路径）；不传参数时默认 `"packages"`。发布包的目录布局就是 `<root>/<name>/`，所以发布时把 `packages` 传进去或直接用默认值。
- **库定位**：清单有 `library` 时先找 `<package_dir>/<platform_library_name(library)>`；文件不存在则退化为裸文件名，交给系统加载器。
- **开发期走的就是退化路径**：`xmake run launcher` 的工作目录是 target 输出目录 `build/<plat>/<arch>/<mode>/`，所有 dll 都在那里，裸文件名搜索正好命中。搜索根只是用来找 `package.json` 的，不是用来找 dll 的。手工运行 `launcher.exe` 时工作目录若不是该输出目录，dll 就会找不到，此时要么切到该目录，要么把 dll 放到清单同目录。

### 2.3 加载语义要点

- **依赖即加载顺序**：`dependencies` 的名字必须已被加载或已在同一批清单里，否则 `ERROR_MISSING_DEPENDENCY`；成环则 `ERROR_CIRCULAR_DEPENDENCY`。清单顺序不影响正确性，但写清单时保持依赖在前更易读。
- **重名即失败**：清单 `name` 与已注册插件名重复返回 `ERROR_DUPLICATE_PACKAGE`（loader 与 `pulse_app` 各有一道去重）。
- **VFS 挂载**：解析与注册期间，包目录挂在 VFS `/`，所以包注册时就能用 `assets/xxx` 读自己的资源。`"assets": true` 的包注册后保持挂载，否则注册完成后卸载。多个 `assets: true` 的包共同可见。
- **静态兜底**：`pulse_package_loader_register_static_package(loader, name, fn)` 把已静态链接进宿主的包登记进去，找不到清单时使用（`tests/package_loader` 用到）。
- **脚本注入时序**：脚本包不是逐个加载的。loader 先处理所有 native 包，然后把当前 VFS 可见范围内的全部脚本文件喂给对应运行时的 `load`，最后才对每个脚本包调 `load_package` 注册入口。因此脚本之间可以互相 `require`。

### 2.4 返回码与报错文本

`EPulsePackageLoadResult`（`PULSE_PACKAGE_LOAD_RESULT_*`）：`OK` / `ERROR_INVALID_ARGUMENT` / `ERROR_INVALID_STATE`（未先加 `pulse_vfs` 插件）/ `ERROR_LIBRARY_NOT_FOUND` / `ERROR_ENTRY_NOT_FOUND` / `ERROR_REGISTER_FAILED` / `ERROR_DUPLICATE_PACKAGE` / `ERROR_MISSING_DEPENDENCY` / `ERROR_CIRCULAR_DEPENDENCY` / `ERROR_UNKNOWN_RUNTIME` / `ERROR_INTERNAL`。

失败时 stderr 的可读原因：

| 报错文本 | 含义 |
|---|---|
| `package loader: pulse_vfs plugin not found; ...` | 宿主没先加 `pulse_vfs` 插件 |
| `package '%s': no manifest found in any search path ...` | 搜索根里没有该包的 `package.json` |
| `package loader: no package.json found in '%s'` | 目录存在但没有清单 |
| `package loader: failed to parse '%s/package.json': %s` | JSON 语法错误，冒号后是 `pulse_config` 的解析错误 |
| `library '%s' not found beside package manifest, falling back to name search` | 退化为裸文件名加载（开发期正常现象） |
| `package '%s': type '%s' requires 'script_file' in package.json` | 非 native 包缺 `script_file` |
| `package '%s': no script runtime registered for type '%s'` | 运行时包没被加载，或 `type` 与运行时注册的 type 不一致 |
| `package loader: script runtime failed to inject '%s'` | 脚本编译/导入失败 |
| `daslang: entry '%s' not injected before package load` | `script_file` 写的路径不在被注入的脚本集合里（见 §8） |
| `plugin '{}' depends on missing plugin '{}'` | `PulsePluginDesc.dependencies` 里有未加载的插件（`pulse_app_last_error`） |
| `failed to load packages, result=%d` | launcher 侧总报错，`%d` 对上面的返回码 |

## 3. package.json 字段

### 3.1 字段表

```json
{
  "name": "pulse_font",
  "version": "1.0.0",
  "library": "pulse_font",
  "entry": "pulse_package_register",
  "assets": false,
  "dependencies": ["pulse_math", "pulse_asset"],
  "paths": ["assets"]
}
```

| 字段 | 必填 | 默认 | 含义 |
|---|---|---|---|
| `name` | 是 | 条目名 | 包名。必须等于 `PulsePluginDesc.name`、生成器使用的模块名、`launcher.manifest.json` 条目名。 |
| `version` | 否 | — | 仅记录，loader 不解释。 |
| `type` | 否 | `"native"` | 非 `native` 表示脚本包，必须配 `script_file`。取值要与脚本运行时注册的 `type` 一致。 |
| `library` | 否 | 包目录名 | 动态库基名，不含前缀与扩展名：Windows `<library>.dll`、Linux `lib<library>.so`、macOS `lib<library>.dylib`。绝对路径原样使用。 |
| `entry` | 否 | `pulse_package_register` | 动态库导出的注册符号名。 |
| `script_file` | 脚本包必填 | — | 脚本入口文件，相对包根，且必须是被注入的脚本之一。 |
| `assets` | 否 | `false` | `true` = 注册后包目录保持在 VFS `/` 挂载。 |
| `dependencies` | 否 | `[]` | 包名数组，决定加载顺序，必须能被满足。 |
| `paths` | 否 | `[]` | 相对包根的资源文件/目录；`pulse.package_install` 安装时拷进包输出目录，不存在直接报错。 |

`library` 必须等于 xmake target 产出的动态库基名。默认就是 target 名；target 名带 `-`（如 `example-snake`）时会产出 `example-snake.dll`，此时用 `set_basename` 改名更省事——`examples/snake` 就是 target 叫 `example-snake`、`set_basename("example_snake")`，所以 `library` 写 `example_snake`。

### 3.2 三种形态的完整清单

native 可选包（`src/pulse_window/package.json`）：

```json
{
  "name": "pulse_window",
  "version": "1.0.0",
  "library": "pulse_window",
  "entry": "pulse_package_register",
  "dependencies": [
    "pulse_input"
  ]
}
```

native 游戏包（`examples/snake/package.json`）：

```json
{
  "name": "snake",
  "version": "1.0.0",
  "library": "example_snake",
  "entry": "pulse_package_register",
  "assets": true,
  "dependencies": [
    "pulse_window",
    "pulse_input",
    "pulse_asset",
    "pulse_transform",
    "pulse_graphics",
    "pulse_renderer",
    "pulse_imgui",
    "pulse_prefab",
    "pulse_datatable"
  ],
  "paths": [
    "assets"
  ]
}
```

脚本内容包（`examples/snake_daslang/package.json`）：

```json
{
  "name": "snake_daslang",
  "version": "1.0.0",
  "type": "daslang",
  "script_file": "snake_module.das",
  "assets": true,
  "dependencies": [
    "pulse_window",
    "pulse_input",
    "pulse_asset",
    "pulse_transform",
    "pulse_graphics",
    "pulse_renderer",
    "pulse_imgui",
    "pulse_daslang",
    "pulse_prefab",
    "pulse_datatable"
  ],
  "paths": [
    "assets"
  ]
}
```

### 3.3 launcher.manifest.json

条目只有 `name` 和可选 `config`：

```json
{
  "app": { "name": "pulse-launcher", "enable_restapi": true },
  "packages": [
    { "name": "pulse_math" },
    { "name": "pulse_window", "config": { "primary_window": { "title": "Pulse", "width": 800, "height": 600 } } },
    { "name": "snake" }
  ]
}
```

`config` 作为 `PulseConfig*` 传进 `pulse_package_register`，由包映射到插件 desc；没有 desc 的包必须拒绝非空 `config`。

### 3.4 config 的键名与类型映射

`config` 的键名就是 IDL 里 desc 字段名的 snake_case：IDL 写 `.primaryWindow "WindowDesc"`，JSON 里写 `"primary_window"`；`.sdlInitFlags` → `"sdl_init_flags"`。嵌套值结构体对应嵌套 JSON 对象。

`tools/gen_package_register` 按下表生成取值代码，`config` 里缺的键保留 `XxxPluginDescDefault()` 的默认值：

| desc 字段类型 | config 读取方式 | JSON 写法示例 |
|---|---|---|
| `bool` | `pulse_config_get_bool` | `"resizable": false` |
| `float` / `double` | `pulse_config_get_double` | `0.5` |
| 整型（`int8_t`…`uint64_t`、`uintptr_t`、`size_t`） | `pulse_config_get_int` + 强转 | `800` |
| `cstring` | `pulse_config_get_string` | `"Pulse"` |
| `flag` / `enum` 类型 | `(EPulseX)(uint32_t)pulse_config_get_int` | `3` 或 `0x3` |
| 嵌套值结构体 | `pulse_config_get_obj` 后递归 | `{ ... }` |
| `struct_size`、`version` | 跳过，恒由默认函数填 | — |
| 指针、数组、可空字段、funcptr、未知外部类型 | 跳过，config 无法表达 | — |

因此 `pulse_window` 的 `primary_window.title` 在清单里写 `"primary_window": { "title": ... }`，而 `PulseWindowPluginDesc.flags` 直接写整数。

## 4. 插件接口

包在 `pulse_package_register` 里最终做的就是调用 `pulse_add_<x>_plugin`，后者把一个 `PulsePluginDesc` 交给 `pulse_app_add_plugin`。

### 4.1 PulsePluginDesc 全字段

```c
typedef EPulsePluginBuildResult (*PulseProcPluginBuildFn)(PulseAppId app, void* ctx);
typedef void (*PulseProcPluginShutdownFn)(PulseAppId app, void* ctx);

typedef struct PulsePluginDesc
{
    uint32_t                 struct_size;
    uint32_t                 version;
    uint32_t                 plugin_version;
    const char*              name;
    void*                    ctx;
    PulseProcPluginBuildFn   build;
    PulseProcPluginBuildFn   post_build;
    PulseProcPluginShutdownFn shutdown;
    uint32_t                 dependency_count;
    const char**             dependencies;
} PulsePluginDesc;
```

| 字段 | 填什么 |
|---|---|
| `struct_size` | 恒为 `sizeof(PulsePluginDesc)` |
| `version` | 恒为 `PULSE_PLUGIN_DESC_VERSION`（`pulse_app.h` 里的常量，当前 `2u`） |
| `plugin_version` | 自己 IDL 里的 `const_value.XxxPluginDescVersion`，即 `PULSE_<MODULE>_PLUGIN_DESC_VERSION` |
| `name` | 插件名，必须等于包名 |
| `ctx` | 插件 state 指针，随 `build`/`post_build`/`shutdown` 传回 |
| `build` | 注册组件、安装系统；可返回 `PULSE_PLUGIN_BUILD_RESULT_*`，失败即整体加载失败 |
| `post_build` | 全部插件 build 完成后才跑，用在这里访问别的插件的组件/系统 |
| `shutdown` | 释放 state、移除系统；必须容忍 `world == NULL` |
| `dependency_count` / `dependencies` | 依赖的**插件名**数组，决定 build 顺序 |

`build`、`post_build`、`shutdown` 都可以传 `NULL`。

### 4.2 生命周期时序

1. loader 按 `package.json.dependencies` 拓扑排序，逐个 `dlopen` 并调 `pulse_package_register`。
2. `pulse_package_register` 调 `pulse_add_<x>_plugin` → `pulse_app_add_plugin` 只把插件放进 pending 队列，**不立即 build**。
3. `pulse_app_prepare`/`pulse_app_run` 时 drain pending：按 `PulsePluginDesc.dependencies` 拓扑顺序跑 `build`。依赖不满足则报 `MISSING_PLUGIN_DEPENDENCY`，成环报 `CIRCULAR_PLUGIN_DEPENDENCY`。`build` 里可以再 `add_plugin`，新插件的依赖检查能看到当前插件。
4. 全部 `build` 成功后，按插件注册顺序逐个跑 `post_build`。
5. 销毁 app 时按注册顺序**逆序**跑 `shutdown`。

结论：**组件和系统注册放 `build`**；需要读取其它插件已注册的 ECS 资源时放 `post_build`。

### 4.3 同一份依赖要写三处

同一个依赖出现在三个地方，语义不同，但必须一致，否则会出现"包加载成功但 build 期报 `plugin 'x' depends on missing plugin 'y'`"或链接期缺符号：

| 位置 | 消费者 | 作用 |
|---|---|---|
| `package.json` 的 `dependencies` | `pulse_package_loader` | 包加载顺序；缺失即 `ERROR_MISSING_DEPENDENCY` |
| `PulsePluginDesc.dependencies` | `pulse_app` | 插件 build 顺序与先决条件校验 |
| 根 `xmake.lua` 的 `add_deps` | xmake | 链接期符号解析 |

引擎包手写依赖数组（`PulsePluginDesc.dependencies`），游戏包由 `generate_module` 从 `package.json` 生成，不需要手写。

## 5. 新建 native 可选包

以 `pulse_font` 为例，产出一个带 desc 的包。每一步都可以对照 `src/pulse_window` 的对应文件。

### 5.1 目录骨架

```
src/pulse_font/
├── package.json
├── idl/
│   ├── pulse_font.idl
│   └── temp.pulse_font.h
├── include/
│   └── pulse_font.h            # 生成物
└── src/
    ├── font_internal.h
    ├── font.cpp                # 插件生命周期 + 公共 API 实现
    ├── font_components.cpp     # 组件注册（可选）
    ├── pulse_font_reflection.h # 生成物（有 component/tag 时）
    └── package_register.cpp    # 生成物
```

### 5.2 IDL

新建 `idl/pulse_font.idl`（IDL 语法见 [IDL.md](IDL.md)）。

```lua
version(1)

typedef "bool"
typedef "float"
typedef "int32_t"
typedef "uint32_t"
typedef "void"
typedef "size_t"
typedef "cstring"

--- From pulse_app.h（必须写精确 C 类型名，codegen 会覆盖自定义 cname）
typedef "EPulseResult"
typedef "EPulseAppAddPluginResult"
typedef "PulseAppId"

const_value.FontPluginDescVersion { value = "1u" }

struct.FontPluginDesc
    .structSize "uint32_t"
    .version    "uint32_t"
    .cacheSize  "uint32_t"
    ()

func.FontPluginDescDefault
    "FontPluginDesc"
    ()

func.AddFontPlugin
    "EPulseAppAddPluginResult"
    .app  "PulseAppId"
    .desc "*const FontPluginDesc"
    ()

func.LoadFont
    "EPulseResult"
    .app  "PulseAppId"
    .path "cstring"
    .size "float"
    ()
```

命名是硬约定，`pulse_package_register` 生成器按它判断生成形态：

- `struct.XxxPluginDesc` + `func.AddXxxPlugin` + `func.XxxPluginDescDefault` 三件套齐全 → 生成带 desc 形态，从 `config` 填 desc；
- 只有 `func.AddXxxPlugin`（如 `pulse_transform`）→ 生成无 desc 形态，要求 `config == NULL`；
- 连 `AddXxxPlugin` 都没有（如 `pulse_app`、`pulse_config`）→ 生成器跳过，适合核心包。

`const_value.XxxPluginDescVersion` 无论有没有 desc 都要写：它给 `PulsePluginDesc.plugin_version` 用。

### 5.3 模板 temp.pulse_font.h

复制 `src/pulse_transform/idl/temp.pulse_transform.h`（这是最干净的模板），改四处：include guard、`PULSE_<MODULE>_API`、`PULSE_<MODULE>_MODULE_BUILD`、依赖头。

```c
#pragma once

#ifndef PULSE_FONT_API_HEADER_GUARD
#define PULSE_FONT_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
#endif

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;

#if defined(PULSE_FONT_MODULE_BUILD)
#  define PULSE_FONT_API PULSE_EXPORT
#else
#  define PULSE_FONT_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

$ccomponents

$ctags

$c99decl

#ifdef __cplusplus
}
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_FONT_API_HEADER_GUARD
```

模板要点：

- `$` 占位符行必须保留，顺序不要动。占位符含义见 [IDL.md](IDL.md) 的"模板占位符"。IDL 里没写 tag 时 `$ctags` 只是不产出内容，留着无副作用。
- `#include "pulse_platform.h"` 提供 `PULSE_EXPORT`/`PULSE_IMPORT` 与 `Pulse_Array`/`Pulse_Blob` 宏；`#include "pulse_app.h"` 提供 `PulseAppId`、`EPulseResult` 等。IDL 里引用了别的模块的类型，就要补对应的 `#include`（`pulse_transform` 的模板就多了一行 `#include "pulse_math.h"`）。
- `typedef uint32_t EPulseFlags; typedef uint64_t EPulseFlags64;` 本身不在任何公共头里，而是**由模板提供**：IDL 里用 `flag.Xxx { ... }` 时生成的 `typedef EPulseFlags EPulseXxxFlags;` 依赖它。不用 flag 时可以删掉。重复 typedef 同一类型合法，多个模块都写这两行不会冲突。
- IDL 引用了外部 C 类型时，模板里要补它的前置声明或 typedef，例如 `pulse_window` 有 `typedef struct SDL_Window SDL_Window;`。生成的类型如果用到 `bool`/`size_t`，补 `<stdbool.h>`/`<stddef.h>`。
- 头部那三个 pragma 是给生成的 C23 属性用的，不要删。

### 5.4 生成公共头

在 `tools/idl/generate.bat` 里加两条命令，然后在 `tools/idl/` 下运行 `generate.bat`（它按行顺序执行，模块之间无依赖，位置只影响可读性）。

C 生成命令放到文件上半段最后一条 `binding = c` 命令之后（即所有 `... c .../include/*.h ...` 命令连成一段）：

```bat
.\lua54.exe generate-binding.lua ..\..\src\pulse_font\idl\pulse_font.idl ..\..\src\pulse_font\idl\temp.pulse_font.h c ..\..\src\pulse_font\include\pulse_font.h Pulse "    " PULSE_FONT_API
```

flecs 反射命令放到文件末尾那一段，只在 IDL 有 `component`/`tag` 时才需要：

```bat
.\lua54.exe generate-binding.lua ..\..\src\pulse_font\idl\pulse_font.idl temp.flecs_reflection.h flecs ..\..\src\pulse_font\src\pulse_font_reflection.h Pulse "    "
```

产物：`include/pulse_font.h`，可选 `src/pulse_font_reflection.h`。

生成命令第 7 参数 `PULSE_FONT_API` 必须与模板里的 `PULSE_FONT_API` 一致；模板里的 `PULSE_FONT_MODULE_BUILD` 必须与 xmake 的 `add_defines("PULSE_FONT_MODULE_BUILD")` 一致。include guard 名字随意，只要唯一。

### 5.5 写实现

**`src/font_internal.h`**：

```cpp
#pragma once

#include "pulse_font.h"

namespace pulse_font_internal {

struct pulse_font_plugin_state {
    PulseAppId app = nullptr;
    uint32_t cache_size = 0;
};

void register_components(ecs_world_t* world);

}
```

**`src/font.cpp`**，三段结构照抄 `src/pulse_transform/src/transform.cpp`：

```cpp
#include "font_internal.h"

namespace pulse_font_internal {

constexpr const char* kPluginName = "pulse_font";

EPulsePluginBuildResult font_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    auto* state = static_cast<pulse_font_plugin_state*>(ctx);
    if (!state) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    state->app = app;
    register_components(world);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void font_plugin_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    delete static_cast<pulse_font_plugin_state*>(ctx);
}

}

using namespace pulse_font_internal;

extern "C" {

PulseFontPluginDesc pulse_font_plugin_desc_default(void) {
    PulseFontPluginDesc desc{};
    desc.struct_size = sizeof(PulseFontPluginDesc);
    desc.version = PULSE_FONT_PLUGIN_DESC_VERSION;
    desc.cache_size = 64;
    return desc;
}

static bool validate_font_plugin_desc(const PulseFontPluginDesc* desc) {
    return !desc || (desc->struct_size == sizeof(PulseFontPluginDesc) && desc->version == PULSE_FONT_PLUGIN_DESC_VERSION);
}

static PulseFontPluginDesc normalize_font_plugin_desc(const PulseFontPluginDesc* desc) {
    PulseFontPluginDesc normalized = pulse_font_plugin_desc_default();
    if (desc) normalized = *desc;
    normalized.struct_size = sizeof(PulseFontPluginDesc);
    normalized.version = PULSE_FONT_PLUGIN_DESC_VERSION;
    return normalized;
}

EPulseAppAddPluginResult pulse_add_font_plugin(PulseAppId app, const PulseFontPluginDesc* desc) {
    if (!app || !validate_font_plugin_desc(desc)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;

    PulseFontPluginDesc normalized = normalize_font_plugin_desc(desc);

    auto* state = new pulse_font_plugin_state();
    state->cache_size = normalized.cache_size;

    const char* font_dependencies[] = { "pulse_asset" };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_FONT_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = font_plugin_build,
        .post_build = nullptr,
        .shutdown = font_plugin_shutdown,
        .dependency_count = 1,
        .dependencies = font_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) delete state;
    return result;
}

EPulseResult pulse_load_font(PulseAppId app, const char* path, float size) {
    (void)app;
    (void)path;
    (void)size;
    return PULSE_RESULT_OK;
}

}
```

要点：

- **desc 三件套的约定**（`pulse_window` 的 `validate_plugin_desc` / `normalize_plugin_desc` 是同一套）：
  - `desc == NULL` 合法，表示全用默认值；生成的 `package_register.cpp` 总是传 `&desc`，所以这条只对直接调 API 的调用方有意义。
  - `desc->struct_size != sizeof(XxxPluginDesc)` 或 `desc->version != PULSE_<MODULE>_PLUGIN_DESC_VERSION` → `ERROR_INVALID_ARGUMENT`。是**精确相等**，不是 `>=`。
  - 取到 desc 后先 `normalize`：以默认值为基底，用 desc 覆盖，再把 `struct_size`/`version` 复位，这样后面所有代码都只面对一份干净的 desc。
- `pulse_app_has_plugin` 查重是必须的：loader 已有一道去重，但插件自身被重复 `pulse_add_*_plugin` 时仍要靠这里挡住。
- 失败且未注册成功时回收 state，成功或已注册时**不能**回收（`shutdown` 会接管）。
- state 里要持有 `cstring` 时自己深拷贝（`ecs_os_strdup`），因为 config 树会在 `pulse_package_register` 返回后被释放，`shutdown` 里再 `ecs_os_free`。
- `pulse_font_plugin_desc_default` 与 `validate_*` / `normalize_*` 都是内部函数，只有 IDL 里声明的 `func.` 才需要 `extern "C"` 导出并带 `PULSE_FONT_API`（生成的公共头已经带了）。
- 没有组件时删掉 `register_components` 调用与 `font_components.cpp`，否则 `font.cpp` 会链接到未定义符号。

**`src/font_components.cpp`**，有 `component`/`tag` 时：

```cpp
#include "font_internal.h"

#include "pulse_font_reflection.h"

ECS_COMPONENT_DECLARE(PulseFontAtlas);

namespace pulse_font_internal {

void register_components(ecs_world_t* world) {
    pulse_font_register_reflection(world);
}

}
```

`ECS_COMPONENT_DECLARE(PulseFontAtlas);` 是必需的：生成的公共头只给出 `extern ECS_COMPONENT_DECLARE(...)` 声明，模块里必须有一个 `.cpp` 在文件作用域放下非 extern 的定义（flecs 的宏会展开成 `ecs_entity_t ecs_id(PulseFontAtlas);`）。放在任何一个模块 `.cpp` 里且只放一次。

### 5.6 写 package.json

```json
{
  "name": "pulse_font",
  "version": "1.0.0",
  "library": "pulse_font",
  "entry": "pulse_package_register",
  "dependencies": ["pulse_asset"]
}
```

`dependencies` 必须与 `PulsePluginDesc.dependencies` 一致，见 §4.3。资源包另加 `"assets": true` 与 `"paths": ["assets"]`。

### 5.7 生成 package_register.cpp

```bat
cd tools\gen_package_register
..\lua54.exe generate.lua ..\..\src\pulse_font\idl\pulse_font.idl
```

或运行 `tools\gen_package_register\generate.bat` 批量重生成所有 `src/pulse_*`。

生成器行为：

- 模块目录必须叫 `pulse_*`，否则报 `not a pulse module idl`；
- 头文件固定 `include/<modname>.h`，API 宏固定 `PULSE_<MODNAME>_API`；
- 有 desc 时按 §3.4 的规则从 `PulseConfig` 读值，调 `pulse_add_font_plugin(app, &desc)`；
- 无 desc 时要求 `config == NULL`，否则返回 `PULSE_RESULT_ERROR_INVALID_ARGUMENT`；
- 把 `EPulseAppAddPluginResult` 逐项映射到 `EPulseResult`，未覆盖的项落到 `PULSE_RESULT_ERROR_INTERNAL`。

以后改 desc 字段只需改 IDL 再重跑。**手写的 `package_register` 必须删掉**，否则重复符号。

### 5.8 xmake target

追加到根 `xmake.lua`（位置不敏感）：

```lua
target("pulse_font")
    set_group("pacakges")
    set_kind("shared")
    add_rules("pulse.package_install", {manifest = "src/pulse_font/package.json"})
    add_defines("PULSE_FONT_MODULE_BUILD")
    add_deps("pulse_platform")
    add_deps("pulse_app")
    add_deps("pulse_config")
    add_deps("pulse_asset")
    add_includedirs("src/pulse_font/include", {public = true})
    add_headerfiles("src/pulse_font/include/*.h")
    add_headerfiles("src/pulse_font/src/*.h", {install = false})
    add_files("src/pulse_font/src/*.cpp")
```

- `add_deps` 要覆盖 `PulsePluginDesc.dependencies` 加上公共头里 `#include` 的模块（`pulse_platform`、`pulse_app`、`pulse_config` 是每包必备）。
- `add_headerfiles(..., {install = false})` 用于内部头，不对外安装。
- 需要第三方 xmake 包时用 `add_packages("...", {public = true})`；需要一起发布运行时 dll 时加 `add_rules("pulse.copy_package_runtime", {packages = {"imgui"}})`。
- 需要异常（如 `std::format`、`pulse_graphics`、`pulse_imgui`、`pulse_daslang`）时加 `set_exceptions("cxx")`；根 `xmake.lua` 默认是 `set_exceptions("none")`。

### 5.9 接进 launcher

1. 在 `src/launcher/launcher.manifest.json` 的 `packages` 里加 `{ "name": "pulse_font", "config": { "cache_size": 128 } }`，放在其依赖之后、需要它的游戏包之前。
2. 构建：`xmake build pulse_font`，再 `xmake build launcher`（清单是 `pulse.copy_manifest` 在 `after_build` 拷的，改清单必须重编 launcher）。
3. `xmake run launcher`；带窗口的验证按 AGENTS.md 走 skill `test-tool-for-program-with-window`。

### 5.10 加测试

测试放 `tests/<name>/`，在根 `xmake.lua` 里加一段循环（照抄现有 `tests/transform` 那段）：

```lua
for _, test_file in ipairs(os.files("tests/font/test_*.cpp")) do
    local test_name = "test-font-" .. path.basename(test_file):gsub("^test_", "")
    target(test_name)
        set_group("tests")
        set_kind("binary")
        set_default(false)
        set_rundir("$(projectdir)")
        add_deps("pulse_app")
        add_deps("pulse_config")
        add_deps("pulse_font")
        add_files(test_file)
        add_files("tests/helper/msvc_headless_asserts.c")
        add_tests("default", {group = "font", rundir = "$(projectdir)"})
end
```

用 `xmake test` 跑（不要 `xmake build` 测试目标）。测试里直接 `#include "pulse_font.h"` 并链接模块，不需要走动态加载；要测动态加载链路则参考 `tests/package_loader/`。

## 6. 新建核心包

只在扩展引擎底座时新增。相比可选包少三样：`package.json`、`package_register.cpp`、`pulse.package_install`；公共头仍走 IDL。

1. 目录骨架、IDL、模板、生成命令同 §5.1–§5.4。
2. 不写 `package.json`。IDL 里不要写 `AddXxxPlugin`：`gen_package_register` 遍历 `src/pulse_*`，没有 `AddXxxPlugin` 的模块自动跳过，所以核心包跑批量生成时是安全的；`examples/` 下的游戏包根本不在它的扫描范围内。
3. target 用核心分组且不挂安装规则：

```lua
target("pulse_font_cache")
    set_group("core")
    set_kind("shared")
    add_defines("PULSE_FONT_CACHE_MODULE_BUILD")
    add_deps("pulse_platform")
    add_includedirs("src/pulse_font_cache/include", {public = true})
    add_headerfiles("src/pulse_font_cache/include/*.h")
    add_files("src/pulse_font_cache/src/*.cpp")
```

4. 接进宿主：加到 `launcher` 的 `add_deps`；被 `pulse_package_loader` 使用则同时加到它的 `add_deps`。
5. 核心包可以照常暴露插件式入口（如 `pulse_add_vfs_plugin`），由宿主在启动流程里显式调用（见 `src/launcher/main.cpp`），但不进清单、不被动态加载。
6. 测试同 §5.10。

共享同一份 IDL 的变体 target（如 `pulse_datalist_static` 与 `pulse_datalist`）各自 `add_defines` 同一个 `PULSE_<MOD>_MODULE_BUILD`、引用同一份 `include/`，只在 `set_kind` 和 `set_default(false)` 上区分。

## 7. 新建 native 游戏包

游戏包走另一条生成链（`tools/generate_module`），与 §5 无关。ECS 模块头的写法与 system 规则见 `tools/generate_module/ECS Module 代码生成规则.md`；完整模板与踩坑清单见 skill `pulseengine-game-module-cpp`。包层面的最短路径：

1. 生成骨架：

```powershell
pwsh -File .agents/skills/pulseengine-game-module-cpp/scripts/new_game.ps1 -Name bounce -DisplayName Bounce
```

产出 `examples/bounce/` 骨架与根 `xmake.lua` 里的 `example-bounce` target。

2. 改写 `bounce.h`：用 `PULSE_ECS_COMPONENT` / `PULSE_ECS_SINGLETON_COMPONENT` / `PULSE_ECS_TAG` / `PULSE_ECS_EVENT` / `PULSE_ECS_RESOURCE` / `PULSE_ECS_STATE_MACHINE` 标记类型，用 `PULSE_ECS_SYSTEM(PHASE=UPDATE)` 标记 system。
3. 写 `examples/bounce/package.json`，`name` 与目录名一致（生成器优先取它的 `name` 当插件名）：

```json
{
  "name": "bounce",
  "version": "1.0.0",
  "library": "example_bounce",
  "entry": "pulse_package_register",
  "assets": true,
  "dependencies": ["pulse_window", "pulse_input", "pulse_asset", "pulse_transform", "pulse_graphics", "pulse_renderer", "pulse_imgui", "pulse_prefab", "pulse_datatable"],
  "paths": ["assets"]
}
```

4. 有数据表时先 `xmake build tablegen`，再用 `tablegen.exe` 生成绑定：

```bat
build\windows\x64\debug\tablegen.exe --out-h examples\bounce\schema\tables_generated.h --out-cpp examples\bounce\schema\tables_generated.cpp examples\bounce\schema\bounce_config.schema
```

纯脚本包用 `--out-das <out>.das` 生成 daslang 表绑定（参考 `examples/snake_daslang/snake_tables.das`）。

5. 生成模块与插件：

```bat
tools\lua54.exe tools\generate_module\generate_module.lua generate examples\bounce\bounce.h examples\bounce\package.json
```

第二个参数（packageinfo）可省略，省略时若头文件同目录有 `package.json` 会自动读取；都没有则模块名取头文件名、插件依赖为空。产物：

| 产物 | 内容 |
|---|---|
| `bounce_module.h` | 生成物，模块声明 |
| `bounce_module.cpp` | 生成物，`importModule`：反射注册、system 注册、状态机、资源 |
| `bounce_plugin.cpp` | 生成物，含 `pulse_add_bounce_plugin` 与 `pulse_package_register` |

`pulse_add_bounce_plugin` 的 build 固定做 `pulse::init_gameplay_base` → `pulse::make_module_context` → `importModule`；插件依赖只来自 `package.json.dependencies`，所以不用手写 `PulsePluginDesc.dependencies`。

6. 生成骨架时已加好 target，确认三处一致：target 名、`set_basename` 的库基名、`package.json.library`。
7. 构建与验证：`xmake build example-bounce` → `xmake build launcher` → 在 `launcher.manifest.json` 的 `packages` 里加 `{ "name": "bounce" }` → `xmake build launcher` → `xmake run launcher`，带窗口验证走 skill `test-tool-for-program-with-window`。

游戏包**不要**跑 `tools/gen_package_register`，那条链要求模块目录叫 `pulse_*`。

## 8. 新增脚本内容包

脚本包不建 xmake target、不写 C++，只写 `package.json` + 脚本 + 资产，靠运行时包（`pulse_daslang`）执行。

最少需要：

```
examples/bounce_daslang/
├── package.json
├── bounce_module.das      # script_file 指向它
├── bounce_tables.das      # tablegen --out-das 产物（可选）
├── schema/*.schema        # 表定义（可选）
└── assets/**              # 资源
```

`package.json`：

```json
{
  "name": "bounce_daslang",
  "version": "1.0.0",
  "type": "daslang",
  "script_file": "bounce_module.das",
  "assets": true,
  "dependencies": ["pulse_window", "pulse_input", "pulse_asset", "pulse_transform", "pulse_graphics", "pulse_renderer", "pulse_imgui", "pulse_daslang", "pulse_prefab", "pulse_datatable"],
  "paths": ["assets"]
}
```

约束：

- `type` 必须与运行时注册的 `type` 一致。daslang 运行时在 `src/pulse_daslang/src/daslang.cpp` 里注册为 `{ "daslang", "das", ... }`，所以 `type` 写 `daslang`。
- `pulse_daslang` 必须在 `dependencies` 里，且排在脚本包之前——loader 只有在 native 包注册成功后才登记它导出的运行时代（符号 `pulse_package_get_runtimes`）。
- `script_file` 必须是包目录内的脚本**相对路径**，且是 loader 注入集合里的一个。loader 把当前 VFS 可见范围内所有 `.das` 文件喂给运行时，所以 `bounce_module.das` 与它 `require` 的 `bounce_tables.das` 都会被注入；写成包外路径或未注入的名字会在加载时报 `daslang: entry '%s' not injected before package load`。
- `script_file` 对应的模块必须提供运行时的入口约定（daslang 侧即 `importModule`）。参考 `examples/snake_daslang/snake_module.das`。
- 脚本包不需要 xmake target，但要在 `launcher.manifest.json` 里列出 `{ "name": "bounce_daslang" }` 才会被加载。`snake_daslang` 在 `install.bat` 里靠 `xcopy` 整目录复制到 `packages/snake_daslang`。

给脚本运行时加一种新脚本语言（而非新增内容包）才需要写 C++：实现一个导出了 `pulse_package_get_runtimes` 的 native 包，返回 `PulseScriptRuntimeDesc` 数组：

```c
typedef struct PulseScriptRuntimeDesc {
    const char* type;
    const char* extensions;
    PulseProcScriptLoadFn load;
    PulseProcScriptPackageLoadFn load_package;
} PulseScriptRuntimeDesc;
```

`type` 对应 `package.json.type`，`extensions` 是逗号/空格分隔的扩展名列表（不带点），`load` 接收每个脚本文件的路径与全文，`load_package` 在全部脚本注入后为每个包注册入口。参考 `tests/package_loader/pkg_script_runtime/package.cpp`。

## 9. 构建与发布

开发期：`xmake build <target>`，产物落在 `build/<plat>/<arch>/<mode>/`，`xmake run launcher` 从那里加载。

发布期：`xmake install -o <pkg_dir> <target>`，`pulse.package_install` 拷进目标目录的有四样：

1. target 产物（`<library>.dll`）；
2. `package.json`；
3. 清单 `paths` 列出的目录；
4. `orderpkgs()` 里各依赖包提供的动态库。

`install.bat`（仓库根）就是逐包安装：先 `xmake build`，清空并重建 `release/`，`xmake install -o release launcher` 后把 `bin/`、`lib/` 里的文件提到根目录，拷 `launcher.manifest.json`，然后逐包：

```bat
xmake install -o "%dest_file%/packages/pulse_font" pulse_font
```

脚本包没有 target，用目录拷贝：

```bat
xcopy "examples\snake_daslang\*.*" "%dest_file%\packages\snake_daslang" /e /i /y
```

安装后一个包一个目录，与运行时搜索根约定一致：

```
release/
├── launcher.exe
├── launcher.manifest.json
└── packages/
    ├── pulse_math/
    │   ├── package.json
    │   └── pulse_math.dll
    └── snake/
        ├── package.json
        ├── example_snake.dll
        └── assets/
```

新增包时同步改 `install.bat`。发布目录直接运行 `launcher.exe`（不要传搜索根）即可，因为默认搜索 `packages`。

## 10. 命名一致性检查单

- [ ] 目录名 = `package.json.name` = `PulsePluginDesc.name` = `launcher.manifest.json` 条目名
- [ ] `package.json.library` = xmake target 库基名（改过 `set_basename` 要同步）
- [ ] `PULSE_<MODULE>_API` = `temp.<module>.h` 里的宏 = IDL 生成命令第 7 参数
- [ ] `PULSE_<MODULE>_MODULE_BUILD` = 模板里的 `#if defined(...)` = xmake `add_defines`
- [ ] `PULSE_<MODULE>_PLUGIN_DESC_VERSION` = `PulsePluginDesc.plugin_version`
- [ ] 有 desc 的包：`struct.XxxPluginDesc` + `func.AddXxxPlugin` + `func.XxxPluginDescDefault` 三件套齐全
- [ ] `PulsePluginDesc.version` = `PULSE_PLUGIN_DESC_VERSION`，`struct_size` = `sizeof(PulsePluginDesc)`
- [ ] `package.json.dependencies` = `PulsePluginDesc.dependencies` = xmake `add_deps`（§4.3）
- [ ] 有 `component`/`tag` 的模块在某一个 `.cpp` 里放了 `ECS_COMPONENT_DECLARE(...)` 定义
- [ ] 资源类包 `"assets": true`，并在 `paths` 里列出要发布进包的目录
- [ ] `install.bat`、`launcher.manifest.json` 已同步
- [ ] 公共头 `include/<module>.h`、`src/package_register.cpp`、`*_module.h/.cpp`、`*_plugin.cpp`、`tables_generated.*` 都是生成物，未被手改

## 11. 常见坑

1. **忘记加 `pulse.package_install`**：`xmake install` 直接报 `pulse.package_install: manifest not configured`。
2. **`paths` 指向不存在的目录**：安装阶段 `os.raise("pulse.package_install: resource not found: ...")`，构建中断；目录要先建好。
3. **漏了 `"assets": true`**：包目录注册后被卸载，之后 `assets/...` 全部解析失败，而且报错不指出原因。
4. **改了 `package.json` 或新增包后没重跑 `xmake build launcher`**：`launcher.manifest.json` 是 `after_build` 拷贝的，launcher 里还是旧清单。
5. **`gen_package_register` 静默跳过该模块**：模块目录不叫 `pulse_*`，或 IDL 里没有 `AddXxxPlugin`。游戏包请用 `generate_module`，不要套这条链。
6. **手改生成物**：`include/*.h`、`package_register.cpp`、`*_module.h/.cpp`、`*_plugin.cpp`、`tables_generated.*`、`*_reflection.h` 下次重跑即被覆盖。
7. **`library` 写成带扩展名的文件名**：loader 会自己补 `.dll`/`lib*.so`，写 `pulse_font.dll` 会变成 `pulse_font.dll.dll`（绝对路径除外）。
8. **同名包同时列出**：loader 去重与 `pulse_app` 插件去重是两道，都会失败。
9. **`xmake build a b` 不合法**：一次只能一个 target；测试用 `xmake test`，不要 `xmake build` 测试目标。
10. **`PulsePluginDesc` 只填了 `name`/`ctx`/`build`/`shutdown`/`dependencies`**：漏 `struct_size`、`version`、`plugin_version`、`post_build`、`dependency_count` 会导致 `pulse_app_add_plugin` 校验失败或 build 顺序错乱；用指定初始化器逐字段写，见 §4.1。
11. **state 里存了 `config` 树里的 `cstring` 指针**：`pulse_package_register` 返回后 config 被释放，指针悬空。必须 `ecs_os_strdup` 自己持有。
12. **`PULSE_<MODULE>_PLUGIN_DESC_VERSION` 没定义**：IDL 里漏了 `const_value.XxxPluginDescVersion`。
13. **有 component 却没有 `ECS_COMPONENT_DECLARE(...)` 定义**：链接期报 `ecs_id(PulseXxx)` 未定义；生成的公共头只有 `extern` 声明。
14. **发布目录里 `launcher.exe` 找不到 dll**：开发期靠工作目录是 `build/.../<mode>/` 才命中裸文件名，发布期包目录里**没有** dll 就会失败——检查 `install.bat` 是否装了依赖包，或 `paths`/`add_rules` 是否漏配。

## 12. 相关文件

| 文件 | 内容 |
|---|---|
| `xmake.lua` | 各包 target；`set_group("core")` / `set_group("pacakges")` |
| `src/pulse_package_loader/include/pulse_package_loader.h` | 加载器 C API 与返回码（IDL 生成） |
| `src/pulse_package_loader/src/package_loader.cpp` | 搜索、清单解析、拓扑排序、dlopen、VFS 挂载、脚本注入 |
| `src/pulse_app/src/c_api.cpp`、`app_impl.cpp` | `PulsePluginDesc` 校验、插件依赖检查、build/post_build/shutdown 时序 |
| `src/launcher/main.cpp`、`src/launcher/launcher.manifest.json` | 宿主与开发期包清单 |
| `src/pulse_script_register/include/pulse_script_register.h` | `PulseScriptRuntimeDesc` 与 `pulse_package_get_runtimes` |
| `xmake/rules/package_output/xmake.lua` | `pulse.package_install` / `pulse.copy_manifest` / `pulse.copy_package_runtime` / `pulse.copy_libc++shared` |
| `tools/idl/`、`docs/IDL.md` | 公共头与反射代码生成 |
| `tools/gen_package_register/` | `package_register.cpp` 生成 |
| `tools/generate_module/`、`tools/generate_module/ECS Module 代码生成规则.md` | 游戏包模块/插件代码生成 |
| `tools/tablegen/` | 数据表绑定生成 |
| `.agents/skills/pulseengine-game-module-cpp/`、`pulseengine-game-module-daslang/` | 游戏包完整模板与踩坑清单 |
| `install.bat` | 发布目录逐包安装 |
