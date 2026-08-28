# IDL 代码生成指南

本项目的 C API 头文件通过 IDL（Interface Definition Language）自动生成，工具链位于 `tools/idl/`。

---

## 整体流程

```
<module>.idl         temp.<module>.h          <module>.h
(接口定义)    →    (模板占位符)        →    (最终 C 头文件)
```

生成命令：

```bat
lua generate-binding.lua <idl> <template> <binding> <output> <prefix> [indent]
# binding: "c"  → 输出 C 头文件
# binding: "zig" → 输出 Zig 绑定
```

示例（pulse_app）：

```bat
lua generate-binding.lua pulse_app.idl temp.pulse_app.h c include/pulse_app.h Pulse "    "
```

---

## IDL 语法

本质上就是lua文件。以下用 prefix=`Pulse` 演示。

### 基础类型声明

这些只是类型映射声明，不生成输出。每个 `.idl` 文件开头都需要。

```lua
typedef "bool"
typedef "float"
typedef "double"
typedef "int8_t"
typedef "int32_t"
typedef "int64_t"
typedef "uint8_t"
typedef "uint16_t"
typedef "uint32_t"
typedef "uint64_t"
typedef "uintptr_t"
typedef "size_t"
typedef "cstring"         -- 映射为 const char*
typedef "anyopaque"       -- 映射为 void
typedef "void"
typedef "va_list"
```

### 常量

```lua
const_value.PluginDescVersion { value = "1u" }
```
编译结果：
```c
#define PULSE_PLUGIN_DESC_VERSION 1u
```

### 枚举

```lua
enum.Result { underscore, comment = "Pulse Result:" }
    .OK
    .ErrorInvalidArgument
    ()
```

`underscore` 将 CamelCase 项名转为 UPPER_SNAKE_CASE。无 `underscore` 则原样大写。引用时写 `"Result::Enum"`。

编译结果：
```c
typedef enum EPulseResult
{
    PULSE_RESULT_OK,
    PULSE_RESULT_ERROR_INVALID_ARGUMENT,
    PULSE_RESULT_COUNT
} EPulseResult;
```

#### 枚举指定值

枚举项可用 `{ value = ... }` 指定显式值，数字或字符串均原样透传：

```lua
enum.DepthBits { underscore }
    .D32 { value = 32 }    --- 32-bit depth
    .D24 { value = 24 }
    .D16 { value = 16 }
    ()
```

编译结果：
```c
typedef enum EPulseDepthBits
{
    PULSE_DEPTH_BITS_D32 = 32,
    PULSE_DEPTH_BITS_D24 = 24,
    PULSE_DEPTH_BITS_D16 = 16,
} EPulseDepthBits;
```

注意：任一枚举项带 `value` 时，自动追加的 `COUNT` 项会被跳过（否则自动递增语义会错误）。因此此类枚举不能用 `"DepthBits::Count"` 作数组尺寸。

### 位标志（flags）

```lua
flag.WindowPlugin { bits = 32, base = 1, underscore }
    .CreatePrimary
    .InstallRunner
    .Default { "CreatePrimary", "InstallRunner" }   -- 组合值
    ()
```

编译结果：
```c
typedef enum EPulseWindowPluginFlagBits
{
    PULSE_WINDOW_PLUGIN_CREATE_PRIMARY = 0x1,
    PULSE_WINDOW_PLUGIN_INSTALL_RUNNER = 0x2,
    PULSE_WINDOW_PLUGIN_DEFAULT = PULSE_WINDOW_PLUGIN_CREATE_PRIMARY | PULSE_WINDOW_PLUGIN_INSTALL_RUNNER,
} EPulseWindowPluginFlagBits;
typedef EPulseFlags EPulseWindowPluginFlags;
```

### Opaque 句柄

```lua
id "AppId"
```

编译结果：
```c
#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id;
DEFINE_PULSE_OBJECT(PulseApp)  /* → typedef struct PulseApp* PulseAppId; */
```

### 结构体

```lua
struct.PluginDesc
    .structSize "uint32_t"
    .name       "cstring"
    .ctx        "?*anyopaque"
    .build      "PluginBuildFn"
    ()
```

`()` 结尾。可选指针用 `?*`。`?*anyopaque` → `void*`，`?cstring` → `const char*`（可为 null）。

```c
typedef struct PulsePluginDesc
{
    uint32_t             struct_size;
    const char*          name;
    void*                ctx;
    PulseProcPluginBuildFn build;
} PulsePluginDesc;
```

空结构体：

```lua
struct.App()
```

```c
struct PulseApp;
typedef struct PulseApp PulseApp;
```

### 函数指针

```lua
funcptr.PluginBuildFn
    "Result::Enum"          -- 返回值（第一个位置）
    .app    "AppId"
    .ctx    "?*anyopaque"
```

```c
typedef EPulseResult (*PulseProcPluginBuildFn)(PulseAppId app, void* ctx);
```

### 自由函数

```lua
func.CreateApp
    "?AppId"                -- 返回值（可空）
    .name "?cstring"
    ()

func.DestroyApp
    "void"
    .app "AppId"
    ()
```

CamelCase 自动转为 snake_case。`?AppId` 表示可能返回 null。

```c
PulseAppId pulse_create_app(const char* name);
void pulse_destroy_app(PulseAppId app);
```

### 成员函数（类方法）

```lua
struct.App()

func.App.Run
    "Result::Enum"
    ()
```

生成的第一个参数是 `_this` 句柄：

```c
EPulseResult pulse_app_run(PulseAppId _this);
```

---

## 模板占位符

模板文件（`temp.<module>.h`）使用 `$` 占位符标记生成内容的插入位置：

| 占位符 | 生成内容 |
|--------|---------|
| `$cconsts` | `#define` 常量 |
| `$cenums` | 枚举 typedef |
| `$cflags` | flags typedef |
| `$cids` | opaque 句柄 |
| `$cfuncptrs` | 函数指针 typedef |
| `$cstructs` | 结构体 typedef |
| `$c99decl` | C 函数声明 |

模板示例：

```c
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFINE_PULSE_OBJECT(name) typedef const struct name* name##Id;

$cconsts
$cenums
$cids
$cfuncptrs
$cstructs
$c99decl

#ifdef __cplusplus
}
#endif
```

---

## 语义注解（v0.2 引入）

IDL 支持在参数、返回值、结构体字段上声明 C 无法表达的语义。生成的 C 头文件会以 C23 属性 `[[pulse::*]]` 形式输出，**纯注解，不影响编译与运行时**。

### 参数/字段/返回值注解表

直接跟一条"类型串 + 注解表"：

```lua
func.AddPlugin
    "Result::Enum"
    .ctx   "?*anyopaque" { retain = true }
    .desc  "*const PluginDesc"
    ()
```

| 注解 | 语义 | 生成的 C 属性 |
|------|------|---------------|
| `retain` | 被调方在调用返回后仍引用该参数，调用方须确保其存活到文档规定的时点 | `[[pulse::retain]]` |
| `owner` | 所有权转移：被调方接管并负责释放，或调用方获得返回值所有权 | `[[pulse::owner]]` |
| `out` | 输出参数，被调方写入；类型自动加一层指针 | `[[pulse::out]]` |

默认语义（无注解时）：**借用、非空、仅输入**。

### `?` → 可空

`?` 前缀自动生成 `[[pulse::optional]]`，参数、返回值、字段、funcptr 参数均生效，无需写表。

### `{ mut }` 成员函数

成员函数（`func.X.Y`）默认 const `_this`；会修改对象的成员函数加 `{ mut }` 标记：

```lua
func.App.Run { mut }
    "AppRunResult::Enum"
    ()

func.App.ShouldQuit
    "bool"
    ()
```

```c
EPulseAppRunResult pulse_app_run(PulseAppId _this);
bool pulse_app_should_quit(Const_PulseAppId _this);
```

opaque 句柄（`id`）有 const 变体：`DEFINE_PULSE_OBJECT` 同时生成 `XxxId` 与 `Const_XxxId`，后者为 `const struct Xxx*`。

### 返回值注解

返回值语义写在返回类型字符串之后的表中（键为 `ret`）：

```lua
func.CreateApp
    "?AppId" { ret = { owner = true } }
    ()
```

```c
[[pulse::optional]] [[pulse::owner]] PulseAppId pulse_create_app(void);
```

发出顺序固定：`optional → owner → retain → out`。

### 切片 / 定长数组（Pulse_Array / Pulse_Blob）

IDL 写 slice（Zig 风格）`[]const T` / `[]T`，生成器按上下文自动选择宏：

| IDL | struct 字段 | 函数参数 |
|-----|-------------|----------|
| `[]const T` | `Pulse_Array(const T, name)` | `Pulse_Array_Param(const T, name)` |
| `[]anyopaque`（字节块） | `Pulse_Blob(name)` | `Pulse_Blob_Param(name)` |

```lua
struct.ShaderCreateFromBinaryDesc
    .vsData  "[]const anyopaque"
    .fsData  "[]const anyopaque"
    .properties "[]const ShaderProperty"
    ()
```

```c
typedef struct PulseShaderCreateFromBinaryDesc
{
    [[pulse::optional]]
    const void*          vs_data;
    size_t               vs_data_count;
    ...
    Pulse_Array(const PulseShaderProperty, properties);
} PulseShaderCreateFromBinaryDesc;
```

宏定义位于 `pulse_platform.h`：

```c
#define Pulse_Array(T, field) T* field; size_t field##_count
#define Pulse_Array_Param(T, param) T* param, size_t param##_count
#define Pulse_Blob(field) const void* field; size_t field##_size
#define Pulse_Blob_Param(param) const void* param, size_t param##_size
```

生成的头文件在开头/结尾带 pragma push/pop，压制编译器对未知属性 `[[pulse::*]]` 的警告（clang `-Wunknown-attributes`、gcc `-Wattributes`、MSVC C5030）。生成头要求 C23 或 C++ 消费（属性是 C23 特性）。

---

## 命名前缀规则

`generate-binding.lua` 的 `<prefix>` 参数控制命名风格：

| 变量 | 推导（prefix=Pulse） | 用途 |
|------|---------------------|------|
| `EU` | `EPulse` | 枚举/flag 类型名前缀 |
| `U` | `Pulse` | struct/id 类型名前缀 |
| `U_` | `PULSE_` | 常量宏前缀 |
| `L_` | `pulse_` | 函数名、枚举项前缀 |

常见 prefix 取值：`Pulse`（PascalCase）、`CGPU`（全大写 acronym）。

---

## 新增一个模块的步骤

1. 在模块目录下创建 `idl/` 子目录
2. 编写 `<module>.idl`（类型 + 函数定义）
3. 编写 `temp.<module>.h`（模板，含 `$` 占位符）
4. 在 `tools/idl/generate.bat` 中添加生成命令
5. 运行 `generate.bat` 生成 `include/<module>.h`
6. 生成的头文件**不要手动编辑**（下次生成会覆盖）

---

## 关键规则

1. **返回值**写在函数名和 `()` 之间（不写在 `.ret` 里）
2. **funcptr 返回值**写在第一参数位置（不是 `.ret`）
3. **`---` 注释**只能放在枚举项、函数参数和 funcptr 参数上（这些位置返回 callable，支持行内注释）。`const_value`、`enum.X {}`、`struct.X {}` 行不能有行内注释，注释放上一行
4. **`cstring`** = `const char*`，**`anyopaque`** = `void`
5. 涉及到指针等的复合类型，采用zig规则
6. **`?`** = 可选/可空：`?*anyopaque` = `void*`，`?BufferId` = 可空返回值
7. **`*const Type`** = `const Type*`（先 `*` 后 `const`）
8. 类型引用：枚举用 `"Result::Enum"`，funcptr 直接用 `"Callback"`（不加后缀），id 用 `"AppId"`
