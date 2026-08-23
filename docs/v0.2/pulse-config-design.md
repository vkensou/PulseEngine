# PulseConfig 模块设计方案

> 状态：已实施（v0.2）
> 关联文档：`docs/launcher-json-config-research.md`（附录 A 为该方案的背景与对比）
> 目标：用「通用配置树」替代 `void* config + uint32_t config_size`，让 launcher 与各插件之间安全地传递 JSON 配置。

---

## 1. 目标与范围

1. 新增公共模块 `src/pulse_config`，提供 OBS `obs_data_t` 风格的通用配置树对象 `PulseConfig`。
2. launcher 解析 `packages.json` 后，把每个包的 `config` 子对象以 `PulseConfig*` 形式传给插件；插件按名查询字段。
3. 替代现有的 `PulsePackageListEntry.config`（`void*` + `config_size`）二进制 desc 传递方式。
4. 保持静态链接路径不变：`pulse_add_window_plugin(app, &desc)` 等 API 继续可用，`PulseConfig` 只用于跨 dll 传输。
5. 参考 `others/obs` 里的代码

**非目标（阶段 1）**：

- 不实现自动 UI（properties 元数据）。
- 不实现多线程安全。
- 不暴露 `json.hpp` 或任何 C++ 类型到公共 C API（JSON 能力封装在模块内部）。

---

## 2. 模块定位

| 项 | 决策 |
|---|---|
| 模块名 | `pulse_config` |
| 公共头 | `src/pulse_config/include/pulse_config.h` |
| 源文件 | `src/pulse_config/src/config.cpp`（C++ 实现；配置树 + json.hpp 适配） |
| 第三方文件 | `src/pulse_config/json.hpp`（单文件 nlohmann::json，header-only，仅模块内部使用） |
| 构建形态 | **动态库 `pulse_config.dll/.so`**（关键，见 §6.4） |
| 依赖 | C++ 标准库 + 单文件 `json.hpp`（header-only，内部使用，不暴露到公共头） |
| 头文件管理 | 优先用 IDL 生成；若 IDL 工具暂不支持 opaque 对象相关语法，可先手写头文件，稳定后迁入 IDL |

---

## 3. 数据模型

### 3.1 类型

与 JSON 对应：

| PulseConfig 类型 | JSON 表示 | C 存储 |
|---|---|---|
| `PULSE_CONFIG_TYPE_NONE` | null / 不存在 | — |
| `PULSE_CONFIG_TYPE_BOOL` | true / false | `bool` |
| `PULSE_CONFIG_TYPE_INT` | 整数 | `int64_t` |
| `PULSE_CONFIG_TYPE_DOUBLE` | 浮点数 | `double` |
| `PULSE_CONFIG_TYPE_STRING` | 字符串 | `char*`（UTF-8，NUL 结尾） |
| `PULSE_CONFIG_TYPE_OBJECT` | 对象 | `std::vector<std::pair<std::string, NodePtr>>`（保持插入序） |
| `PULSE_CONFIG_TYPE_ARRAY` | 数组 | `std::vector<NodePtr>`（通用值数组） |

### 3.2 数字规则

- 解析时：JSON 数字若为整数值且落在 `int64_t` 范围内，存为 `INT`；否则存为 `DOUBLE`。
- `get_int`：`INT` 原样返回；`DOUBLE` 截断为 `int64_t`（越界返回 default）；`BOOL` 返回 0/1（可选，建议不做，严格返回 default）。
- `get_double`：`DOUBLE` 原样返回；`INT` 提升为 `double`。

### 3.3 深度限制

- 解析和序列化时维护深度计数器，最大深度 **64**，超过报错，防止恶意输入导致栈溢出。

---

## 4. 所有权模型（核心设计）

### 4.1 句柄与节点

- `PulseConfig` 是不透明句柄，实际指向内部节点（§6.1 的 `Node`）。
- 内部采用**每个节点独立引用计数**（与 OBS `obs_data_t` 一致）：
  - 父节点持有子节点的引用（父释放时递归释放子节点引用）。
  - `pulse_config_get_obj` / `pulse_config_get_array` 返回子节点指针，**不增加引用**（借用）。
  - 调用方若需长期持有子对象，可对返回的 `PulseConfig*` 调 `pulse_config_addref`，随后自行 `release`。

### 4.2 规则

| 场景 | 规则 |
|---|---|
| `pulse_config_create*` | 返回引用计数为 1 的新对象 |
| launcher → 插件 | launcher 持有根对象；插件在 `register` 调用期间借用；launcher 在 `load_packages` 返回后释放根 |
| 插件长期持有 | 插件 `pulse_config_addref(cfg)` 后持有，用完 `pulse_config_release` |
| 子对象指针 | 仅在父对象存活期间有效；长期持有需 `addref` 该子对象 |
| `to_json` 返回的字符串 | 调用方用 `pulse_config_free_string` 释放（不能用裸 `free`） |

### 4.3 生命周期时序

```
launcher:
    root = pulse_config_create_from_json_file("packages.json")
    packages = pulse_config_get_array(root, "packages")
    for each pkg:
        entry.config = pulse_config_get_obj(pkg, "config")   // 借用
        loader 调用 fn(app, entry.config)                     // 插件借用
    pulse_config_release(root)                                // 全部子对象随之失效
```

---

## 5. C API 设计

### 5.1 生命周期

```c
PULSE_CONFIG_API PulseConfig* pulse_config_create(void);
PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json(const char* json, size_t len);
PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json_file(const char* path);

PULSE_CONFIG_API void pulse_config_addref(PulseConfig* cfg);
PULSE_CONFIG_API void pulse_config_release(PulseConfig* cfg);          // 引用归零时销毁
PULSE_CONFIG_API void pulse_config_free_string(char* str);             // 释放 to_json 的返回值
```

语义：

- `create_from_json`：`len` 为字节数，`json` 可为 `NULL`（返回空对象）。解析失败返回 `NULL`，错误信息见 §8。
- `release(NULL)` 与 `addref(NULL)` 为 no-op。

### 5.2 查询

```c
PULSE_CONFIG_API bool pulse_config_has(const PulseConfig* cfg, const char* key);
PULSE_CONFIG_API EPulseConfigType pulse_config_get_type(const PulseConfig* cfg, const char* key);

PULSE_CONFIG_API bool        pulse_config_get_bool(const PulseConfig* cfg, const char* key, bool default_value);
PULSE_CONFIG_API int64_t     pulse_config_get_int(const PulseConfig* cfg, const char* key, int64_t default_value);
PULSE_CONFIG_API double      pulse_config_get_double(const PulseConfig* cfg, const char* key, double default_value);
PULSE_CONFIG_API const char* pulse_config_get_string(const PulseConfig* cfg, const char* key, const char* default_value);

PULSE_CONFIG_API PulseConfig*       pulse_config_get_obj(const PulseConfig* cfg, const char* key);   // 借用
PULSE_CONFIG_API PulseConfigArray*  pulse_config_get_array(const PulseConfig* cfg, const char* key); // 借用
```

语义：

- `key` 不存在 → 返回 `default_value`。
- `key` 存在但类型不匹配 → 按 §3.2 做有限兼容转换；无法转换时返回 `default_value`。
- `get_string` 返回的指针在 cfg 存活期间有效，调用方不得修改或释放。
- `get_obj` / `get_array` 对非对象/非数组节点返回 `NULL`。
- 数组元素为标量时，`get_*` 的 `key` 传 `NULL` 表示读取节点自身（例如 `pulse_config_get_string(element, NULL, NULL)` 读取字符串数组元素）。

### 5.3 数组

```c
PULSE_CONFIG_API size_t pulse_config_array_count(const PulseConfigArray* arr);
PULSE_CONFIG_API PulseConfig* pulse_config_array_get(const PulseConfigArray* arr, size_t index);  // 借用
```

`PulseConfigArray` 可作为 `PulseConfig` 的别名（内部同一节点类型），API 上分开以表达意图。

### 5.4 写入（阶段 2）

```c
PULSE_CONFIG_API void pulse_config_set_bool(PulseConfig* cfg, const char* key, bool value);
PULSE_CONFIG_API void pulse_config_set_int(PulseConfig* cfg, const char* key, int64_t value);
PULSE_CONFIG_API void pulse_config_set_double(PulseConfig* cfg, const char* key, double value);
PULSE_CONFIG_API void pulse_config_set_string(PulseConfig* cfg, const char* key, const char* value);
PULSE_CONFIG_API void pulse_config_set_obj(PulseConfig* cfg, const char* key, PulseConfig* value);  // 内部 addref
PULSE_CONFIG_API void pulse_config_set_array(PulseConfig* cfg, const char* key, PulseConfigArray* value);

PULSE_CONFIG_API bool pulse_config_remove(PulseConfig* cfg, const char* key);  // true = 成功删除
```

### 5.5 序列化

```c
PULSE_CONFIG_API char* pulse_config_to_json(const PulseConfig* cfg, size_t* out_len);       // 紧凑
PULSE_CONFIG_API char* pulse_config_to_json_pretty(const PulseConfig* cfg, size_t* out_len); // 2 空格缩进
```

- 返回 `malloc` 出的 NUL 结尾字符串，`*out_len` 不含 NUL（可为 `NULL`）。
- 必须用 `pulse_config_free_string` 释放。

### 5.6 合并（阶段 2，可选）

```c
// 把 defaults 与 overrides 深合并，返回新对象；overrides 字段优先。
PULSE_CONFIG_API PulseConfig* pulse_config_merge(const PulseConfig* defaults, const PulseConfig* overrides);
```

用于「插件默认配置树 ∪ 用户配置树」，对应 OBS `get_defaults` + 用户设置合并。

### 5.7 错误信息（可选）

```c
PULSE_CONFIG_API const char* pulse_config_last_error(void);
```

阶段 1 可只返回固定错误码；若实现，用线程局部存储保存最近一次解析错误（含行号/列号）。

---

## 6. 内部实现设计（C++ 实现 + json.hpp）

### 6.1 节点结构（C++）

参考 `others/obs/obs-data.h/c` 的 API 形状与所有权模型，用 C++ 标准库实现：

```cpp
enum class NodeType { None, Bool, Int, Double, String, Object, Array };

struct Node;
using NodePtr = std::shared_ptr<Node>;   // 阶段 1 用 shared_ptr 即可；C API 层再包 intrusive refcount

struct Node {
    NodeType type = NodeType::None;
    // 标量值
    bool b = false;
    int64_t i = 0;
    double d = 0.0;
    std::string s;
    // 对象：保持插入序
    std::vector<std::pair<std::string, NodePtr>> object;
    // 数组：通用值数组（注意：与 OBS 不同，OBS 数组只支持 object 元素）
    std::vector<NodePtr> array;
};
```

要点：

- 对象用 `vector<pair<string, NodePtr>>` 保持插入序，序列化顺序稳定；查找可线性扫描（配置树很小），或阶段 2 再加 `unordered_map` 索引。
- 数组是**通用值数组**，支持 `["a", 1, true, {...}]`。这是对 OBS 模型的关键泛化（OBS `obs_data_array` 只能存对象），因为 `packages.json` 里的 `dependencies` 就是字符串数组。
- 对外 C API 层使用 intrusive refcount（`std::atomic<int>` 或阶段 1 用普通 `int32_t`）把 `Node` 包装成不透明 `PulseConfig*`；`NodePtr` 只存在于模块内部。
- OBS item 的「单次分配布局」（struct + name + data 连续内存）是 C 技巧，C++ 下不需要。

### 6.2 JSON 解析/序列化：采用单文件 json.hpp

采用单文件 `json.hpp`（nlohmann::json single-header）。

**集成方式**：

- 文件放置：`src/pulse_config/src/json.hpp`。
- **仅模块内部使用**：`pulse_config.h` 不得 include `json.hpp`，不得在公共 API 暴露任何 nlohmann 类型。`json.hpp` 是 C++ 模板库，跨 dll 边界暴露会带来 ABI/编译期模板实例化问题。
- 解析：`nlohmann::json::parse(json_str)`，用 try/catch 捕获 `nlohmann::json::exception`，转为 `NULL` 返回 + `pulse_config_last_error()`（含 `exception.what()`，其信息通常带行号/列号）。
- 序列化：`nlohmann::json::dump()`（紧凑）与 `dump(2)`（pretty，2 空格缩进），结果转存为 `malloc` 字符串交给 `pulse_config_free_string` 释放。
- 深度/大小：nlohmann 默认无深度限制，需在解析前检查输入长度上限（如 16 MB），并在解析后遍历检查嵌套深度（> 64 报错），或使用 `SAX` 接口带深度计数（阶段 1 可先只做输入长度限制）。

### 6.3 JSON → 配置树 / 配置树 → JSON 转换

- 解析后遍历 `nlohmann::json`，转换为 §6.1 的 `Node` 树：
  - `boolean` → `Bool`
  - `number_integer` / `number_unsigned` → `Int`
  - `number_float` → `Double`
  - `string` → `String`
  - `object` → `Object`（按 `items()` 顺序插入）
  - `array` → `Array`（元素逐项转换，支持任意类型）
  - `null` → `None`
- 序列化时反向转换；字符串转义、UTF-8、pretty 缩进全部交给 `json.hpp`。
- 重复 key 策略：`nlohmann::json::parse` 默认**保留最后一个**（与 OBS 的 `JSON_REJECT_DUPLICATES` 不同）；阶段 1 接受此行为，阶段 2 如需严格校验可改用 `SAX` 检测重复 key。

### 6.4 构建形态：为什么必须是动态库

这是本方案最关键的工程决策：

- `PulseConfig` 对象由 launcher 创建、插件读取、最后可能由任一方释放。
- 对象的内存分配与释放必须发生在**同一个模块/同一个分配器**内。
- 若 `pulse_config` 以静态库形式链入 launcher 和各插件 dll，则每个二进制各有一份代码与 CRT 堆视图；一个 dll 里 `malloc` 的对象在另一个 dll 里 `free`，在跨 CRT 场景下会崩溃。
- 因此 `pulse_config` 必须与 `pulse_app` 一样编译为**动态库**，launcher 与所有插件链接同一个 `pulse_config.dll/.so`。

**接口约定**：所有返回需调用方释放的内存（`to_json` 字符串、错误字符串）统一通过 `pulse_config_free_string` / `pulse_config_release` 归还给该动态库。

**json.hpp 的位置**：`json.hpp` 虽为 header-only，但只能在 `pulse_config.dll` 的编译单元内实例化；公共头 `pulse_config.h` 不 include 它。这样模板实例化、异常、`std::string` 内存都在 `pulse_config` 模块内部，不跨越 dll 边界。

### 6.5 内存分配

- 模块内部：C++ 容器（`std::string` / `std::vector` / `shared_ptr`）自动管理，全部在 `pulse_config.dll` 内完成。
- 对外：`to_json` 返回的字符串用模块内 `malloc` 分配，外部通过 `pulse_config_free_string` 归还；`PulseConfig` 对象通过 `pulse_config_release` 归还。
- 禁止在公共头暴露任何 `std::*` 类型或 `json.hpp` 类型。

### 6.6 线程安全

- 阶段 1：不保证线程安全；`refcount` 用普通 `int32_t`。
- 阶段 2（若需要）：`refcount` 改为 `atomic_int`；文档中注明「同一对象不得并发写」。

---

## 7. 与 launcher / loader / 插件的集成

### 7.1 loader 接口变更

`src/pulse_package_loader/include/pulse_package_loader.h`：

```c
typedef struct PulseConfig PulseConfig;  // 由 pulse_config.h 引入

typedef EPulseResult (*PulseProcPackageRegisterFn)(PulseAppId app, PulseConfig* config);

typedef struct PulsePackageListEntry
{
    const char*          name;
    const char*          library;
    PulseConfig*         config;          // 替代 void* config
    // uint32_t config_size;              // 删除
    uint32_t             dependency_count;
    const char**         dependencies;
} PulsePackageListEntry;
```

`package_loader.cpp` 调用点：

```c
EPulseResult result = fn(app, entry->config);
```

### 7.2 launcher 集成

`examples/launcher/main.cpp` 伪代码：

```c
PulseConfig* root = pulse_config_create_from_json_file("packages.json");
if (!root) { fprintf(stderr, "bad packages.json: %s\n", pulse_config_last_error()); return 1; }

PulseConfigArray* packages = pulse_config_get_array(root, "packages");
size_t count = pulse_config_array_count(packages);

// 组装 entries（name/library/dependencies 均从树中读取）
PulsePackageListEntry* entries = ...;
for (size_t i = 0; i < count; ++i) {
    PulseConfig* pkg = pulse_config_array_get(packages, i);
    entries[i].name    = pulse_config_get_string(pkg, "name", NULL);
    entries[i].library = pulse_config_get_string(pkg, "library", NULL);
    entries[i].config  = pulse_config_get_obj(pkg, "config");   // 借用；无 config 为 NULL
    // dependencies：get_array + array_count + array_get + get_string
}

EPulsePackageLoadResult r = pulse_package_loader_load_packages(app, entries, count);
// load 返回后 entries 可释放，root 可释放
pulse_config_release(root);
```

### 7.3 插件侧集成（以 window 为例）

```c
PULSE_WINDOW_API EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config)
{
    PulseWindowPluginDesc desc = pulse_window_plugin_desc_default();

    if (config) {
        PulseConfig* pw = pulse_config_get_obj(config, "primary_window");
        if (pw) {
            const char* title = pulse_config_get_string(pw, "title", desc.primary_window.title);
            if (title) desc.primary_window.title = ecs_os_strdup(title);

            desc.primary_window.width  = (int32_t)pulse_config_get_int(pw, "width", desc.primary_window.width);
            desc.primary_window.height = (int32_t)pulse_config_get_int(pw, "height", desc.primary_window.height);
            desc.primary_window.resizable = pulse_config_get_bool(pw, "resizable", desc.primary_window.resizable);
            desc.primary_window.external_graphics_context =
                pulse_config_get_bool(pw, "external_graphics_context", desc.primary_window.external_graphics_context);
        }
        desc.sdl_init_flags = (uint32_t)pulse_config_get_int(config, "sdl_init_flags", desc.sdl_init_flags);
    }

    EPulseAppAddPluginResult r = pulse_add_window_plugin(app, &desc);
    ...
}
```

要点：

- 仍调用 `pulse_window_plugin_desc_default()` 作为默认值来源。
- `get_*` 的最后一个参数就是默认值，JSON 未提供时行为与现状 `config == NULL` 完全一致。
- `pulse_add_window_plugin` 会走既有 normalize/validate 逻辑，`struct_size`/`version` 照旧被覆盖，JSON 无法破坏 ABI 校验。

### 7.4 无需配置的插件

`input` / `renderer` / `snake` 等 register 改为：

```c
PULSE_INPUT_API EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config)
{
    if (config) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;  // 本插件不接受配置
    ...
}
```

### 7.5 packages.json 示例

```json
{
  "app": { "name": "pulse-launcher", "enable_restapi": true },
  "packages": [
    { "name": "PulseInputPlugin", "library": "pulse_input.dll" },
    {
      "name": "PulseWindowPlugin",
      "library": "pulse_window.dll",
      "dependencies": ["PulseInputPlugin"],
      "config": {
        "primary_window": {
          "title": "Pulse Launcher",
          "width": 1280,
          "height": 720,
          "resizable": false
        }
      }
    },
    {
      "name": "PulseAssetPlugin",
      "library": "pulse_asset.dll",
      "config": { "root_path": "examples/snake/assets", "max_requests_per_update": 64 }
    },
    {
      "name": "PulseGraphicPlugin",
      "library": "pulse_graphics.dll",
      "dependencies": ["PulseWindowPlugin", "PulseAssetPlugin"],
      "config": { "enable_debug_layer": true, "enable_vsync": true }
    }
  ]
}
```

---

## 8. 错误处理

- `create_from_json*` 失败返回 `NULL`，`pulse_config_last_error()` 返回形如 `json: parse error at line 3, column 12: expected ':'` 的信息。
- loader 对 `entry->config == NULL` 视为「无配置，使用插件默认」，不视为错误。
- 插件内 `get_*` 遇到类型不匹配按默认值兜底，不中断加载。
- 插件可自行做严格校验：对必填字段判断 `pulse_config_has`，缺失时返回 `PULSE_RESULT_ERROR_INVALID_ARGUMENT`。

---

## 9. 测试计划

| 层级 | 内容 |
|---|---|
| 单元测试（`tests/config`） | 解析各类型、嵌套、转义、unicode、数字边界、深度超限、错误位置；get 默认值与类型转换；数组；序列化 roundtrip；引用计数释放 |
| loader 集成 | 复用 `tests/package_loader` 现有假插件，改造成接收 `PulseConfig*` 并记录查询到的值，断言 launcher 传入的树正确 |
| 插件测试 | window/asset 的 desc 从 config 树构造，行为与直接传 desc 一致 |
| 冒烟 | launcher 读 packages.json 启动窗口（按项目规则使用 test-tool skill） |
| 工具校验 | 扩展 `tools/check_module_manifests.py`，校验 packages.json 中的 name/dependencies 与各包 `package.json` 一致 |

---

## 10. 实施步骤

1. **创建模块骨架**：`src/pulse_config/`（`include/pulse_config.h`、`src/config.cpp`、`src/json.hpp`、`package.json`、`xmake.lua` 或统一构建脚本），编译为动态库。
2. **实现核心**：节点/引用计数 → json.hpp 解析适配 → 查询 → 序列化。
3. **单元测试**：`tests/config/*` 跑通 `xmake test`。
4. **改 loader 签名**：`PulseProcPackageRegisterFn` 与 `PulsePackageListEntry`；同步修改 `tests/package_loader` 与 `examples/launcher`、`examples/snake_daslang` 的调用点。
5. **改各插件 register**：window、asset、graphics 实现 `get_*` 查询；input/renderer/snake 等拒绝非空 config。
6. **launcher 读 packages.json**：替换硬编码数组。
7. **全量回归**：`xmake test` + launcher 冒烟。
8. **文档更新**：`docs/v0.2` 与包加载说明同步。

---

## 11. 风险与对策

| 风险 | 对策 |
|---|---|
| 跨 dll 分配/释放崩溃 | `pulse_config` 编译为唯一动态库；所有释放走 API |
| `json.hpp` 跨 dll 边界暴露（模板实例化/ABI 问题） | 公共头 `pulse_config.h` 不 include `json.hpp`；只在 `config.cpp` 内部使用 |
| `json.hpp` 解析异常导致 C API 崩溃 | `parse`/`dump` 全部 try/catch，转错误码/`NULL` + `pulse_config_last_error()` |
| `json.hpp` 编译时间增加 | 只在一个 `.cpp` 中 include；预编译头可选 |
| 解析深度导致栈溢出 | 输入长度上限 + 深度限制 64（遍历校验，或 SAX 计数） |
| 重复 key 策略与 OBS 不同（nlohmann 默认保留最后一个） | 阶段 1 接受；阶段 2 如需严格校验用 SAX 检测 |
| `get_int` 对超大 double 的截断 | 范围检查，越界返回默认值 |
| key 拼写错误无编译期检查 | 插件内部用宏/常量定义 key；远期由 IDL 生成 typed wrapper |
| loader 签名变更波及面大 | 仓库内插件一次性同步修改；`config` 为 `NULL` 的默认路径行为不变 |
| 阶段 1 无写接口，后续 restapi 无法改配置 | API 预留 `set_*`/`merge`/`remove`，阶段 2 实现 |
| `to_json` 字符串释放错误 | 统一 `pulse_config_free_string`，测试覆盖 |

---

## 12. 文件清单（计划）

```
src/pulse_config/
├── include/pulse_config.h        # 公共 C API（IDL 生成；不得 include json.hpp）
├── idl/pulse_config.idl           # IDL 定义
├── idl/temp.pulse_config.h        # IDL 模板
├── src/config.cpp                # 节点、引用计数、get/set、json.hpp 适配
├── src/json.hpp                  # 单文件 nlohmann::json（header-only，仅模块内部使用）
└── package.json                  # 模块 manifest

tests/config/
├── test_json_parse.cpp
├── test_get_set.cpp
├── test_refcount.cpp
├── test_serialize.cpp
└── ...

docs/
├── launcher-json-config-research.md   # 研究背景
└── v0.2/pulse-config-design.md             # 本方案
```
