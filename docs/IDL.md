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
#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id; typedef const struct name* Const_##name##Id;
DEFINE_PULSE_OBJECT(PulseApp)  /* → typedef struct PulseApp* PulseAppId; typedef const struct PulseApp* Const_PulseAppId; */
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

### ECS Component

组件是可注册到 ECS 的结构体，语法与 `struct` 相同：

```lua
component.Window
    .structSize "uint32_t"
    .title      "cstring"
    ()
```

除结构体 typedef 外，额外生成组件实体声明：

```c
typedef struct PulseWindow
{
    uint32_t             struct_size;
    const char*          title;

} PulseWindow;
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseWindow);
```

组件的 flecs 反射由 `flecs` binding 生成，字段可用 `noreflex` / `min` / `max` 等标注控制，见
[flecs 反射生成](#flecs-反射生成)。

### ECS Tag

标签是空结构体（不允许有字段），实体符号统一带 `Id` 后缀：

```lua
tag.PrimaryWindow()
```

```c
typedef struct PulsePrimaryWindow{} PulsePrimaryWindow;
PULSE_WINDOW_API extern ECS_TAG_DECLARE(PulsePrimaryWindowId);
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

func.App.GetState
    "State::Enum"
    ()

func.App.Run { mut }
    "Result::Enum"
    ()
```

生成的第一个参数是 `_this` 句柄。成员函数默认是`const`，类似c++的`const`成员函数。如果想修改`_this`，需要设置`mut`属性。

```c
EPulseState pulse_app_get_state(Const_PulseAppId _this);
EPulseResult pulse_app_run(PulseAppId _this);
```

---

## 语义注解

IDL 支持在参数、返回值、结构体字段上声明 C 无法表达的语义如生命周期。生成的 C 头文件会以 C23 属性 `[[pulse::*]]` 形式输出，**纯注解，不影响编译与运行时**。

| 注解 | 语义 | 生成的 C 属性 |
|------|------|---------------|
| `?` | 可空 | `[[pulse::optional]]` |
| `retain` | 被调方在调用返回后仍引用该参数，调用方须确保其存活到文档规定的时点 | `[[pulse::retain]]` |
| `owner` | 所有权转移：被调方接管并负责释放，或调用方获得返回值所有权 | `[[pulse::owner]]` |
| `out` | 输出参数，被调方写入；类型自动加一层指针 | `[[pulse::out]]` |

### 举例

```lua
func.Func1
    "?AppId" { ret = { owner = true } }
    .desc  "*const PluginDesc"
    .ctx   "?*anyopaque" { retain = true }
    .outError   "cstring" { out = true }
    ()

struct.Test
    .field1           "*int32_t"
    .field2           "?*int32_t"
    .field3           "*int32_t" { owner = true }
    .field4           "*int32_t" { retain = true }
    ()
```

生成的c代码：

```c
[[pulse::optional]] [[pulse::owner]] PulseAppId pulse_func1(const PluginDesc* desc,  [[pulse::optional]] [[pulse::retain]] void* ctx, [[pulse::out]] const char** out_error);

typedef struct PulseTest
{
    int32_t*             field1;
    [[pulse::optional]]
    int32_t*             field2;
    [[pulse::owner]]
    int32_t*             field3;
    [[pulse::retain]]
    int32_t*             field4;

} PulseTest;
```

注解顺序固定为：`optional → owner → retain → out`。默认语义（无注解时）：**借用、非空、仅输入**。

建议：生成模板最好在开头/结尾带如下代码，用于减少编译器的警告/报错：

```c
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
```

```c
#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
```

## 数组 / 字节块（Pulse_Array / Pulse_Blob）

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
    Pulse_Blob(vs_data);
    Pulse_Blob(fs_data);
    Pulse_Array(const PulseShaderProperty, properties);
} PulseShaderCreateFromBinaryDesc;
```

宏定义位于 `pulse_platform.h`：

```c
#define Pulse_Array(T, field) T* p_##field; size_t field##_count
#define Pulse_Array_Param(T, param) T* p_##param, size_t param##_count
#define Pulse_Blob(field) const void* p_##field; size_t field##_size
#define Pulse_Blob_Param(param) const void* p_##param, size_t param##_size
```

---

## flecs 反射生成

同一份 idl 还能生成 flecs 的组件反射注册代码，输出路径由命令行指定（约定为
`src/pulse_<module>/src/pulse_<module>_reflection.h`）：

```bat
lua generate-binding.lua <idl> temp.flecs_reflection.h flecs <output.h> <prefix> [indent]
```

示例（pulse_transform）：

```bat
lua54.exe generate-binding.lua ..\..\src\pulse_transform\idl\pulse_transform.idl temp.flecs_reflection.h flecs ..\..\src\pulse_transform\src\pulse_transform_reflection.h Pulse "    "
```

生成一个内联函数，函数名由 idl 文件名推导（`pulse_<module>_register_reflection`）：注册 idl 中所有
`component`/`tag` 的 ecs id（`ecs_id(PulseX) = comp.id()`，tag 为
`ecs_id(PulseXId) = PulseXId = comp.id()`），并按 idl 声明顺序添加成员反射。

```c
inline void pulse_transform_register_reflection(ecs_world_t* world) {
    {
        flecs::component<PulseLocalTransform> comp(world, "PulseLocalTransform");
        ecs_id(PulseLocalTransform) = comp.id();
        comp.member("translation", &PulseLocalTransform::translation);
        ...
    }
}
```

模块在自己的组件注册函数里调用它即可，生命周期钩子、`EcsWith` 等逻辑仍留在手写代码中：

```cpp
#include "pulse_transform_reflection.h"

void register_components(ecs_world_t* world) {
    pulse_transform_register_reflection(world);
    ecs_add_pair(world, ecs_id(PulseLocalTransform), EcsWith, ecs_id(PulseWorldTransform));
}
```

### 反射规则与标注

成员名取 idl 字段的 C 成员名（`justPressed` → `just_pressed`）。默认全部反射，例外如下：

| 情况 | 生成结果 |
|------|----------|
| 指针 / 切片 / 可空 / `cstring`（`*T`、`[]T`、`?*T`） | 自动跳过，C 反射无法表达 |
| `ecs_entity_t` 字段（含数组） | 按 flecs 引用生成：`comp.member(ecs_id(ecs_entity_t), name, count, offsetof(...))` |
| 非实体类型的固定数组 `[N] T` | 成员指针形式，flecs 自动推导元素类型与长度 |
| `{ entity = true }` | 字段类型不是 `ecs_entity_t`，但语义是实体引用 |
| `{ min = .., max = .. }` | 追加 `.range(min, max)`，供编辑器/数值校验使用 |
| 字段 `{ noreflex = true }` | 不反射该字段 |
| 组件 `{ noreflex = true }` | 只注册 id，不反射任何成员 |
| 组件 `{ external = true }` | 类型由外部头文件定义：不生成 C 结构体与 `ECS_COMPONENT_DECLARE`，只生成反射，字段名按 idl 原样使用 |

`external` 用于把第三方/外部定义的 C 结构体纳入反射，例如 pulse_math 中的 HandmadeMath 类型：

```lua
component.HMM_Vec3 { external = true }
    .X "float"
    .Y "float"
    .Z "float"
    ()

component.Camera
    .windowEntity "ecs_entity_t"
    .fov          "float" { min = 0.0, max = 180.0 }
    .reserved     "uint32_t" { noreflex = true }
    ()
```

已接入反射生成的模块见 `tools/idl/generate.bat`；模块按需把自己的 `flecs` 生成命令加进去。

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
| `$ccomponents` | component：结构体 typedef + ECS_COMPONENT_DECLARE |
| `$ctags` | tag：空结构体 typedef + ECS_TAG_DECLARE（`Pulse<Name>Id`） |
| `$c99decl` | C 函数声明 |

模板示例：

```c
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id; typedef const struct name* Const_##name##Id;

$cconsts
$cenums
$cids
$cfuncptrs
$cstructs
$ccomponents
$ctags
$c99decl

#ifdef __cplusplus
}
#endif
```

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
4. 在 `tools/idl/generate.bat` 中添加生成命令（需要 ECS 组件反射的，再加一条 `flecs` 生成命令）
5. 运行 `generate.bat` 生成 `include/<module>.h` 与 `src/<module>_reflection.h`
6. 在模块的组件注册函数中调用生成的 `pulse_<module>_register_reflection(world)`
7. 生成的头文件**不要手动编辑**（下次生成会覆盖）

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
