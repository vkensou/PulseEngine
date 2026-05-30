---
name: idl-guide
description: 教 agent 如何编写本项目的 IDL 文件（.idl + temp.*.h + 生成流程）
---
# IDL 编写指南

本项目的 C API 头文件通过 IDL（接口定义语言）自动生成，工具链位于 `tools/idl/`。

## 整体流程

```
<project>.idl         temp.<project>.h          <output>.h
(类型+函数定义)  →   (带 $占位符 的模板)   →   (生成的头文件)
```

```bat
lua generate-binding.lua <idl> <template> <binding> <output> <prefix> [indent]
# binding: "c" → bindings-c,  "zig" → bindings-zig
# prefix:  控制类型/函数命名前缀（如 "MyLib"）
```

## IDL 语法 → C 输出对照

以下用 prefix=`My` 演示（`EU`=EMy, `U`=My, `U_`=MY_, `L_`=my_）。复合类型（数组、指针等）使用zig语法。

### 基础类型（不生成输出）

下面是常用的类型，一般来说直接复制即可，可以根据需要补充。

```lua
typedef "bool"
typedef "char"
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
typedef "va_list"
typedef "void"
typedef "anyopaque"       -- 映射为 void
typedef "size_t"
typedef "cstring"         -- 映射为 const char*
```

### 常量

```lua
--- 描述
const_value.MaxCount { value = "16" }
```
```c
#define MY_MAX_COUNT 16
```

### 枚举

```lua
enum.Status { underscore, comment = "状态码" }
    .OK                  --- 成功
    .InvalidArg          --- 参数错误
    ()
```
```c
typedef enum MyStatus
{
    MY_OK,                                 /** ( 0) 成功                              */
    MY_INVALID_ARG,                        /** ( 1) 参数错误                           */

    MY_STATUS_COUNT

} MyStatus;
```
- `underscore`：项名 CamelCase → UPPER_SNAKE_CASE（`.InvalidArg` → `INVALID_ARG`）
- 无 `underscore`：项名原样大写（`.OK` → `OK`）
- 引用此枚举类型时写 `"Status::Enum"`

### 函数指针

```lua
funcptr.Callback
    "Status::Enum"        -- 返回值（第一个参数位置，不是 .ret）
    .ctx    "?*anyopaque" -- ? = 可选, * = 指针
    .data   "uint32_t"
```
```c
typedef MyStatus (*MyCallback)(void* ctx, uint32_t data);
```
- 返回值在**第一个位置**写，参数在 `.argname "type"` 写
- 生成的 typedef 名：`My` + `Callback` → `MyCallback`

### Opaque 句柄

```lua
id "BufferId"
```
```c
#define DEFINE_MY_OBJECT(name) typedef const struct name* name##Id;
DEFINE_MY_OBJECT(Buffer)       /* → typedef const struct Buffer* BufferId; */
```
- `$cids` 占位符输出上面两行
- 模板中需要先有 `#define DEFINE_MY_OBJECT(name) ...`（由 `compute_naming` 推导宏名）

### 结构体（有成员）

```lua
struct.Descriptor
    .size  "uint32_t"
    .label "cstring"
    .cb    "Callback"         -- 引用 funcptr 类型
    ()
```
```c
typedef struct MyDescriptor
{
    uint32_t size;
    const char* label;
    MyCallback cb;

} MyDescriptor;
```

### 结构体（空）

```lua
struct.OpaqueType()
```
```c
struct MyOpaqueType；
typedef struct MyOpaqueType MyOpaqueType;
```

### 自由函数

```lua
func.CreateBuffer "?BufferId"      --- 创建缓冲区
    .size "uint32_t"
    ()

func.DestroyBuffer "void"          --- 销毁
    .buf "BufferId"
    ()
```
```c
MyBufferId my_create_buffer(uint32_t size);

void my_destroy_buffer(MyBufferId buf);
```
- 返回值写在函数名**后面**（`.ret` 写法无效）
- 函数名中 CamelCase 自动转 snake_case（`CreateBuffer` → `create_buffer`）
- 前缀来自 `L_`（`my_`）

### 成员函数（类方法）

```lua
struct.Buffer              -- 先定义 Buffer 类型

func.Buffer.Map "void"     --- Buffer.Map → buffer_map
    .range "uint32_t"
    ()
```
```c
void my_buffer_map(MyBufferId _this, uint32_t range);
```
- `func.<Type>.<Method>`：类名 + `.` + 方法名
- `this` 指针自动生成（`MyBufferId _this`）
- bindings-c 会将函数参数展平(包括_this)，这个功能主要是为支持面向对象的语言预留

### flags（位标志）

```lua
flag.Access { bits = 32, base = 1 }
    .Read                 --- 读
    .Write                --- 写
    .ReadWrite { "Read", "Write" }   --- 组合
    ()
```
```c
typedef enum MyAccessFlagBits
{
    MY_ACCESS_READ = 0x1,
    MY_ACCESS_WRITE = 0x2,
    MY_ACCESS_READ_WRITE = MY_ACCESS_READ | MY_ACCESS_WRITE,

} MyAccessFlagBits;
typedef EMyFlags MyAccessFlags;
```

## 模板占位符

| 占位符 | 内容 |
|--------|------|
| `$cconsts` | `#define` 常量 |
| `$cenums` | 枚举 typedef |
| `$cflags` | flags typedef |
| `$cids` | opaque 句柄 (`DEFINE_*`) |
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

#define DEFINE_MY_OBJECT(name) typedef const struct name* name##Id;

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

## 命名前缀规则

`compute_naming(prefix)` 由 CLI 的 `<prefix>` 参数传入。以 `"MyLib"` 为例：

| 变量 | 推导 | 值 | 用途 |
|------|------|-----|------|
| `EU` | `"E" .. prefix` | `EMyLib` | enum/flag 类型名前缀 |
| `U` | `prefix` | `MyLib` | struct/id 类型名前缀 |
| `U_` | `prefix:upper() .. "_"` | `MYLIB_` | const 宏前缀 |
| `L_` | `prefix:lower() .. "_"` | `mylib_` | 函数名、枚举项前缀 |

- 传 `"CGPU"`（全大写）→ 传统 acronym 风格
- 传 `"MyLib"`（PascalCase）→ enum 类型 `EMyLibStatus`，struct `MyLibBuffer`
- 枚举项前缀始终来自 `L_:upper()`，如 `MYLIB_OK`

## 关键规则

1. **`---` 注释**只能放在枚举项、函数、funcptr 的**参数**上（这些返回 callable）。`const_value`、`enum.X {}`、`struct.X {}` 行不能有行内注释，注释放上一行。
2. **函数返回值**写在函数名和 `()` 之间（`.ret` 无效）。
3. **funcptr 返回值**写在第一参数位置（不是 `.ret`）。
4. **`cstring`** = `const char*`，**`anyopaque`** = `void`。
5. **`?`** = 可选：`?*anyopaque` = `void*`，`?BufferId` = 可空返回值。
6. **`*const Type`** = `const Type*`（先 `*` 后 `const`）。
7. 类型引用：枚举用 `"Status::Enum"`，funcptr 用 `"Callback"`，id 用 `"BufferId"`。

## 新增项目的步骤

1. 创建 `idl/` 目录，写 `<project>.idl` 和 `temp.<project>.h`
2. 在 `tools/idl/generate.bat` 加一行：
   ```bat
   lua generate-binding.lua <project>.idl temp.<project>.h c <output_path> <prefix> "    "
   ```
3. 运行 `generate.bat`
4. 生成的头文件**不要手动编辑**（下次生成会覆盖）
