# snake_daslang 脚本包化与脚本包加载中间层设计

> 状态：已评审通过
> 日期：2026-08-25
> 范围：`pulse_package_loader` 增加脚本包加载能力（中间层）；`examples/snake_daslang` 改造为纯内容包，由通用 launcher 加载。

---

## 1. 背景与目标

现状：

- `pulse_package_loader` 只能加载 C/C++ DLL 包：搜索 `package.json` → 按 `dependencies` 拓扑排序 → `LoadLibrary` → 调用固定入口 `pulse_package_register(app, config)`。
- `examples/snake`（C++ 版）已经是标准包：`package.json` + 导出 `pulse_package_register` 的 DLL。
- `examples/snake_daslang` 是"纯宿主 exe"：先用 loader 加载 8 个引擎包，再在 `main.cpp` 中手工调用 `pulse_load_module(app, ".../snake_module.das")`。游戏逻辑全部在 `.das` 脚本里，由 `pulse_daslang` 插件编译并调用脚本导出的 `importModule`。

目标：

1. `snake_daslang` 成为一个包：launcher 的 manifest 里列名字即可加载，不再需要专用宿主 exe、不再手工调 `pulse_load_module`。
2. 为此在 `pulse_package_loader` 中引入**脚本包加载中间层**。未来会有不定数量的 daslang 包，甚至其他类型的脚本引擎，机制必须可扩展；第一版只实现 daslang。

## 2. 关键决策（已与用户确认）

| # | 决策 |
|---|---|
| 1 | 验收宿主为**通用 launcher**：`launcher.manifest.json` 中 `snake` 替换为 `snake_daslang` |
| 2 | 第一版只做**扩展点 + daslang 实现**，不引入第二个脚本引擎 |
| 3 | das 标准库（`pulse/*.das`、`daslib/*`）**随 pulse_daslang 包发布**：源码迁入 `src/pulse_daslang/das/`，构建时复制到 DLL 所在目录 |
| 4 | 游戏资源**复制到 snake_daslang 包内**（`assets/` 子目录），包自包含 |
| 5 | **删除** `example-snake-daslang` exe target 与 `main.cpp`，`examples/snake_daslang/` 变成纯包目录 |
| 6 | `PulseDaslangPluginDesc` **删除 `root_path` 字段**：插件自己知道标准库在哪（本 DLL 目录） |
| 7 | 运行时注册**不对外暴露 API**：引擎包 DLL 导出固定符号，loader 加载时自动探测登记 |
| 8 | 新增 **header-only target `pulse_script_register`**（单一头文件），承载协议类型，`pulse_package_loader` 与 `pulse_daslang` 共同依赖 |

## 3. 总体方案

**方案 A（选定）：loader 内部"脚本运行时表" + 引擎包导出符号声明。**

- `package.json` 增加可选字段 `"type"`（缺省 `"native"` = 现有 DLL 流程）与 `"script_file"`（入口脚本相对包目录路径）。
- 引擎包 DLL（如 `pulse_daslang.dll`）导出固定符号 `pulse_package_get_runtimes`，返回它提供的运行时表 `{type, handler}`。
- loader 加载每个 native 包、`pulse_package_register` 成功后，`GetProcAddress` 探测该符号并登记运行时；遇到非 native 包时按 `type` 查表，把 `(app, 包名, 包目录, script_file, config)` 交给 handler。
- 加载顺序由现有 `dependencies` 拓扑排序保证：脚本包声明依赖引擎包，引擎包必然先被完整处理。

否决的备选：

- **方案 B：共享宿主 DLL**（一个通用 `daslang_host.dll` 供所有脚本包当 `library`）。入口签名 `(app, config)` 拿不到包目录与 manifest，仍需改 loader 传上下文；多包共享一个 DLL 语义别扭。省不下改动，不干净。
- **方案 C：loader 内置 daslang 支持**。loader 直接调 `pulse_load_module` 会导致 package_loader 反向依赖 pulse_daslang（分层倒挂），且无法扩展到其他脚本引擎。

方案 A 的优点：loader 保持引擎无关；未来 lua/wasm 引擎只需让自己的包导出同一符号；脚本包是纯内容目录（无 DLL），在 iOS/wasm 等不支持动态库的平台反而天然可用。

### 3.1 注册机制细节

1. **探测时机**：native 包的 `pulse_package_register` 调用成功之后。注册失败的包不贡献运行时。
2. **方向**：探测是 loader → DLL 的单向 `GetProcAddress`。引擎包只需 include 协议头文件拿类型定义，**不链接** `pulse_package_loader`，无分层倒挂。
3. **运行时表**：进程级（`type → handler`），因为 handler 是 DLL 中的代码能力而非 per-app 状态；handler 签名自带 `app` 参数。
4. **冲突**：同一 type 重复注册，先到先得并打印警告。
5. **生命周期**：`pulse_package_loader_cleanup` 卸载 DLL 的同时清空运行时表，避免悬垂函数指针。
6. **缺失**：加载某 `type` 的脚本包但运行时表中没有该 type → 返回新错误码 `PULSE_PACKAGE_LOAD_RESULT_ERROR_UNKNOWN_RUNTIME`，错误信息带包名与 type 名。

## 4. 协议头：`pulse_script_register`

新增 header-only target，仿照 `pulse_math` / `pulse_cpp_gameplay`：

- 目录：`src/pulse_script_register/include/pulse_script_register.h`（唯一文件）
- xmake：`set_kind("headeronly")`，`add_deps("pulse_app", "pulse_config")`（传递 include），`add_includedirs("src/pulse_script_register/include", {public = true})`
- `pulse_package_loader`、`pulse_daslang` 均 `add_deps("pulse_script_register")`

头文件内容：

```c
// pulse_script_register.h —— 脚本包运行时协议（loader 与引擎包共享）
#include "pulse_app.h"      // PulseAppId, EPulseResult
#include "pulse_config.h"   // PulseConfig

// loader 传给脚本运行时 handler 的包信息
typedef struct PulsePackageScriptInfo {
    const char*  name;         // 包名
    const char*  package_dir;  // 包目录（manifest 所在目录）
    const char*  script_file;  // manifest 中 script_file 原值（相对包目录）
    PulseConfig* config;       // 宿主传入的配置，可为空
} PulsePackageScriptInfo;

typedef EPulseResult (*PulseProcScriptPackageLoadFn)(
    PulseAppId app, const PulsePackageScriptInfo* info);

typedef struct PulseScriptRuntimeDesc {
    const char* type;                    // 运行时类型名，如 "daslang"
    PulseProcScriptPackageLoadFn load;   // handler
} PulseScriptRuntimeDesc;

// 引擎包导出符号名与签名
#define PULSE_PACKAGE_GET_RUNTIMES_SYMBOL "pulse_package_get_runtimes"

// 返回运行时数量；*out_runtimes 指向静态数组，生命周期与 DLL 相同
typedef uint32_t (*PulseProcPackageGetRuntimesFn)(
    const PulseScriptRuntimeDesc** out_runtimes);
```

## 5. manifest 格式

脚本包 `package.json` 示例（`examples/snake_daslang/package.json`）：

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
    "pulse_daslang"
  ]
}
```

字段语义：

| 字段 | 说明 |
|---|---|
| `type` | 可选。缺省或 `"native"` = 现有 DLL 流程；其他值 = 按脚本运行时分发 |
| `script_file` | `type` 非 native 时必填：入口脚本相对包目录的路径；缺失 → `ErrorInvalidArgument` |
| `library` / `entry` | 仅 native 有效，脚本包忽略 |
| `assets` | 语义不变：把包目录挂进 VFS content root；两种 type 通用 |
| `dependencies` | 语义不变；脚本包必须把引擎包（`pulse_daslang`）列入，才能保证拓扑序 |

## 6. package_loader 内部流程改动

仅改 `src/pulse_package_loader/src/package_loader.cpp` 与 idl，不改 `PulsePackageListEntry`，launcher 代码零改动。

1. **resolve_package**：多读 `type`、`script_file` 字段存入 `ResolvedPackage`；`type` 非 native 且 `script_file` 为空 → 加载失败（`ErrorInvalidArgument`）。
2. **拓扑排序 / 查重**：不变（按包名）。
3. **加载循环**，每个包：
   - `assets` 挂载 VFS 不变，先于注册发生；
   - **native**：现有流程（LoadLibrary → 找 entry → 调 register）；register 成功后探测 `PULSE_PACKAGE_GET_RUNTIMES_SYMBOL`，有则登记返回的每个 `{type, fn}`；
   - **脚本包**：按 `type` 查运行时表。查不到 → `ErrorUnknownRuntime`；查到 → 组装 `PulsePackageScriptInfo{name, package_dir, script_file, config}` 调 handler；handler 返回非 `PULSE_RESULT_OK` → `ErrorRegisterFailed`。
4. **cleanup**：`pulse_package_loader_cleanup` 同时清空运行时表。
5. **公共面变化**：仅 `EPulsePackageLoadResult` 枚举新增 `ErrorUnknownRuntime`（idl 中 `enum.PackageLoadResult` 追加 `.ErrorUnknownRuntime`，重新生成头文件）。

## 7. pulse_daslang 侧改动

### 7.1 导出运行时表（手写，不走 idl）

`daslang.cpp` 新增：

```c
extern "C" PULSE_EXPORT uint32_t pulse_package_get_runtimes(
    const PulseScriptRuntimeDesc** out_runtimes)
{
    static const PulseScriptRuntimeDesc runtimes[] = {
        { "daslang", daslang_script_package_load },
    };
    *out_runtimes = runtimes;
    return 1;
}
```

该符号由 loader 通过 `GetProcAddress` 调用，不需要进入公共头，也不需要 idl 声明（本节的导出与 7.3 的 idl 变更互相独立）。

### 7.2 handler 实现

`daslang_script_package_load(app, info)`：

- 校验 `app` / `info` / `script_file` 非空；
- 拼绝对路径 `info->package_dir + "/" + info->script_file`；
- 调用现有 `pulse_load_module(app, path)`（编译失败时它已打印详细错误）；
- 成功返回 `PULSE_RESULT_OK`，失败返回 `PULSE_RESULT_ERROR_INTERNAL`；
- `info->config` 本期忽略，预留给未来（如脚本启动参数）。

时序安全性：插件 build 为"即时排水"（`pulse_app_add_plugin` 时依赖满足即 build），loader 按拓扑序处理，脚本包 handler 被调用时 `pulse_daslang` 插件必然已 build，`state_from_app` 可用。

### 7.3 删除 root_path

- idl：`struct.DaslangPluginDesc` 删除 `.rootPath`；`const_value.DaslangPluginDescVersion` 升为 `2u`。重新生成 `pulse_daslang.h`（tools/idl）与 `package_register.cpp`（tools/gen_package_register），生成代码中读取 `"root_path"` 配置的行随之消失。
- `pulse_add_daslang_plugin` 不再要求 `root_path`；`state->root_path` 保留为内部字段，在插件 build 时填充为**本 DLL 所在目录**（`GetModuleHandleEx` / `dladdr` 取自身符号所在模块，`package_loader.cpp` 中已有同样做法 `module_directory()`）；获取失败则 build 报错。
- `das::setDasRoot(state->root_path)` 与 `require pulse/...` 的 fs root 映射逻辑不变（`root_path` 须为包含 `pulse/` 与 `daslib/` 的目录）。
- `tests/daslang/*` 三处 `root_path = "examples/asset"` 删除。

**代价（记录在案）**：失去把 das 库指向别处的覆盖能力；未来需要时通过 desc 版本升级加回。

### 7.4 das 标准库随包发布

- 源码搬家：`examples/asset/{pulse,daslib}` → `src/pulse_daslang/das/{pulse,daslib}`（git mv，~130 个文件）；`examples/asset` 目录删除。
- xmake：`pulse_daslang` target 增加 after_build，把 `das/pulse`、`das/daslib` 复制到 `target:targetdir()`（DLL 平铺输出目录），与默认 root_path 对齐。
- 已知小坑：单独编辑 `.das` 文件不会触发 pulse_daslang 重建，复制会陈旧，需 `xmake build -r`；后续可做成跟踪 `.das` 的自定义 rule，本期不做。

## 8. snake_daslang 包布局

```
examples/snake_daslang/
  package.json          # 见第 5 节
  snake_module.das      # 现有脚本，仅改两处资源路径
  assets/
    color.vert.spv      # 从 examples/snake/assets 复制
    color.frag.spv
    Quad.obj
    color.slang         # shader 源码一并带上
    compile_shader.bat  # 路径相对 assets/，可直接复用
```

- `main.cpp` 删除；xmake 删除 `target("example-snake-daslang")`。
- `snake_module.das` 仅两处资源路径改动（VFS content root 不递归查找，包目录挂载后必须带前缀，与 C++ snake 的 `"assets/..."` 惯例一致）：
  - `"color.vert.spv"` → `"assets/color.vert.spv"`、`"color.frag.spv"` → `"assets/color.frag.spv"`
  - `"Quad.obj"` → `"assets/Quad.obj"`

## 9. launcher 与构建改动

- launcher 代码零改动。
- `src/launcher/launcher.manifest.json`：
  - `"snake"` 条目替换为 `"snake_daslang"`；
  - 新增 `"pulse_daslang"` 条目（无 config），置于 `pulse_imgui` 之后（复现现有示例的加载顺序，`daslang_plugin_build` 依赖 `pulse_imgui_get_phase`）；
  - 窗口标题改为 `"PulseEngine Snake Daslang"`。
- xmake target 变化汇总：
  - 新增 `pulse_script_register`（header-only）；
  - `pulse_package_loader`、`pulse_daslang` 增加对它的依赖；
  - `pulse_daslang` 增加 das 库复制的 after_build；
  - 删除 `example-snake-daslang`。

## 10. 测试与验收

1. **新增无头单测** `tests/package_loader/test_loader_script_runtime.cpp`：
   - 新增测试 DLL `tests/package_loader/pkg_script_runtime/`（仿 `pkg_custom_entry`），导出 `pulse_package_register` 与 `pulse_package_get_runtimes`，handler 类型为 `"mockscript"`；
   - handler 在 DLL 内校验收到的 `name` / `package_dir` / `script_file` / `config` 与预期一致后，注册一个标记 plugin（如 `"mock_script_pkg"`）；
   - 测试断言：标记 plugin 存在（分发与参数传递正确）、`"assets": true` 的包目录已挂载（`pulse_vfs_file_exists`）、加载未注册 type 的脚本包返回 `ErrorUnknownRuntime`。
2. **现有测试**：`xmake test` 全绿（package_loader 既有用例不受影响；daslang 用例适配 root_path 删除）。
3. **端到端（带窗口）**：build 后按 skill `test-tool-for-program-with-window` 运行 launcher，确认窗口标题、截图非黑屏，贪吃蛇可玩（资源加载、输入、ImGui 分数、GameOver 重启）。

## 11. 已知取舍（记录在案，本期不解决）

- 脚本包不会注册同名 plugin，跨多次 `pulse_package_loader_load_packages` 调用的重复加载检测不到（单次调用内查重不受影响）。
- 脚本包的 `config` 透传到 handler 后被 daslang 实现忽略。
- `.das` 标准库编辑后的陈旧复制问题（见 7.4）。
- 静态平台（iOS/wasm）的运行时注册回退：届时引擎静态编入宿主，无 DLL 可探测，需要补充显式注册手段；本期不做。
- `root_path` 覆盖能力被移除（见 7.3）。
