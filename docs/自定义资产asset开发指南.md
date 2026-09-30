# 自定义资产asset开发指南

面向开发者：如何在自己的插件里引入一种新的 asset 类型。覆盖类型注册、loader 编写、异步加载状态机、依赖、settings、引用计数与完整接入流程。

本文是自足的：照本文写就能跑通。遇到语义分歧时以源码为准，权威实现位置见 §10。

## 0. 全景

`pulse_asset` 是一个与资源内容完全解耦的异步加载系统。它只负责三件事：

1. **注册表**：记录每种 asset 类型的内存布局（size/align/destroy）和 loader（按扩展名或 builder 标识匹配）。
2. **加载队列**：读文件（或收内存数据）、驱动 loader 回调、管理依赖等待，全部在主线程的 ECS `OnLoad` 阶段推进。
3. **存储与生命周期**：为每个 asset 分配 slot（index+generation），维护路径缓存、依赖图和 pin 引用计数，引用归零时回调 destroy 并回收。

引入一种自定义 asset 类型 = 四步：

| 步骤 | 做什么 | 参考 |
|---|---|---|
| 1 | IDL 定义 type id、Handle/Request/Data 结构与公共 API，生成公共头 | `src/pulse_prefab/idl/pulse_prefab.idl` |
| 2 | `pulse_asset_system_register_type` 注册类型 | `src/pulse_prefab/src/load_prefab.cpp` |
| 3 | `pulse_asset_system_register_loader` 注册一个或多个 loader | `src/pulse_graphics/src/loader/` |
| 4 | 插件 build 钩子注册、shutdown 钩子 `force_unload`、封装 `pulse_load_xxx` 等 API | `src/pulse_prefab/src/prefab_plugin.cpp` |

几个现成消费者的复杂度梯度：

- `pulse_prefab`：一个类型、一个文件 loader、带 dtor，最简完整样例。
- `pulse_font`：两个类型（Font + FontChain）、文件 loader（ttf/otf/ttc/fnt）+ builder loader 生成链、settings，展示"链式"资产结构与 shutdown 强制卸载，是 prefab 之后的第二个完整样例。
- `pulse_datatable`：一个类型、一个 loader，展示 registry 作为 user_data 的用法。
- `pulse_graphics`：8 种类型、每种多个 loader（文件 + 内存 + builder）、settings、异步上传（PENDING）、动态依赖（WAIT_DEPENDENCIES）、settings 深拷贝，全特性样例。

## 1. 核心概念

### 1.1 type_id

`type_id` 是 `uint64_t`，全局唯一，非 0。

以下是已经被pulse内置插件占用的type_id：

| 号段 | 归属 | 已用 |
|---|---|---|
| `0x1000+` | pulse_graphics | TEXTURE=0x1000、GRAPHICS_BUFFER=0x1001、SAMPLER=0x1002、SHADER_LIBRARY=0x1003、SHADER=0x1004、COMPUTE_SHADER=0x1005、MESH=0x1006、MATERIAL=0x1007 |
| `0x2000+` | pulse_prefab | PREFAB=0x2000 |
| `0x3000+` | pulse_datatable | DATA_TABLE=0x3000 |
| `0x4000+` | pulse_font | FONT=0x4000、FONT_CHAIN=0x4001 |

type id 必须在 IDL 里用 `const_value` 定义（进公共头），并在注释里声明号段归属，照抄 prefab 的写法：

```
--- Asset type id range 0x2000+ belongs to pulse_prefab (graphics uses 0x1000+)
const_value.TypePrefab { value = "UINT64_C(0x2000)" }
```

### 1.2 Handle / Request / DepRef

三个结构体位目前布局完全相同（`{type_id, index, generation}`），语义不同：

| 类型 | 什么时候拿到 | 含义 |
|---|---|---|
| `PulseAssetRequest` | load/build 入口的返回值 | 加载收据。加载期间用它查询状态、取消 |
| `PulseAssetHandle` | `pulse_asset_system_get_handle(request)`，仅在 LOADED 时返回有效值 | 使用凭证。用它 retain/release/borrow |
| `PulseAssetDepRef` | 由 handle/request 转换 | 依赖表里引用别的 asset |

对外模块的瘦类型 `PulseXxxHandle` / `PulseXxxRequest`（只有 index+generation，type_id 隐含在转换函数里）与互转、查询函数都由 `pulse_asset.h` 提供的宏生成：

```c
PULSE_DEFINE_ASSET_TYPE(foo, PULSE_TYPE_FOO, PulseFooHandle, PulseFooRequest)
```

它依次生成：
- 两个 typedef 结构体 `PulseFooHandle` / `PulseFooRequest`（`{uint32_t index; uint32_t generation;}`），IDL 里不再写 `struct.FooHandle`/`struct.FooRequest`，改用 `typedef "PulseFooHandle"` 引用即可；
- `pulse_foo_to_handle(PulseFooHandle) -> PulseAssetHandle` 和 `pulse_foo_request_to_asset_request(PulseFooRequest) -> PulseAssetRequest`；
- `pulse_foo_get_handle(app, request)`（未 LOADED 返回无效 handle）、`pulse_foo_is_ready`、`pulse_foo_is_alive`、`pulse_foo_get_error`。

子宏可拆开单用：`PULSE_DEFINE_ASSET_HANDLE_TYPE(handle_type, request_type)`、`PULSE_DEFINE_ASSET_CONVERSIONS(name, type_enum, handle_type, request_type)`、`PULSE_DEFINE_ASSET_GET_HANDLE(name, handle_type, request_type)`、`PULSE_DEFINE_ASSET_STATUS(name, request_type)`。get_handle 有业务逻辑的模块只取其余子宏组合、跳过 GET_HANDLE 子宏，自己实现 `pulse_xxx_get_handle`。（pulse_font 曾用此法把字体登记进注册表；现已改为无注册表设计——handle 即资产凭证，face 数据经 `pulse_asset_system_borrow` 直取，get_handle 走标准宏。）

这行写在 IDL 的 temp 头模板里（手写区，见 `src/pulse_prefab/idl/temp.pulse_prefab.h`），不是 IDL 生成的；要放在 `$cstructs` 之前——若模块 IDL 的结构体字段引用了瘦 handle 类型，typedef 必须先于结构体定义可见。

### 1.3 三种加载来源

| 来源 | 入口 | loader 匹配方式 | bytes 来源 |
|---|---|---|---|
| File | `pulse_asset_system_load` | 路径扩展名 → `extensions` | 资产系统读文件（经 pulse_vfs 路径） |
| Memory | `pulse_asset_system_load_from_memory` | 同上（仍需 path 定扩展名） | 调用者数据被拷贝进资产池，调用后原内存可释放 |
| Builder | `pulse_asset_system_build` / `build_sync` | `loader_identifier` → 无扩展名的 loader | 无 bytes，数据全在 settings 里 |

builder loader 的注册特征：`extensions` 为空（`nullptr` 或 `""`），靠 `loader_identifier` 索引；`build_sync` 会立即同步执行首轮 step，适合过程化创建（见 `create_texture.cpp`、`create_sampler.cpp`）。

### 1.4 状态机

```
EMPTY → WAITING_LOAD → LOADING → PROCESSING → LOADED
              ↑            │         │  ↑          │
              │            ↓         │  └──────────┤ (step 返回 PENDING，下帧重入)
              │   WAITING_DEPENDENCIES ←───────────┘ (step 返回 WAIT_DEPENDENCIES)
              │
        任何加载态 ──→ FAILED / PENDING_DELETE(引用归零且加载未完)
```

- `pulse_asset_system_get_state(request)` 返回当前状态。
- `is_ready` = 状态为 LOADED；`is_alive` = 非 EMPTY/FAILED/PENDING_DELETE（即加载中也算 alive）。
- `get_error(request)` 返回失败原因字符串（loader 通过 `*out_error` 提供，会被拷贝存储，静态字符串即可）。

### 1.5 驱动方式与线程模型

- pulse_asset插件构建时安装 ECS 系统 `PulseAssetProcessLoadRequests`（`EcsOnLoad` 阶段，immediate），每帧推进加载队列，每帧最多处理 `max_requests_per_update` 个 job（默认 8，可通过 `PulseAssetPluginDesc` 调整）。
- 所有插件 build 完成后，`post_build` 会 `drain_loads()` 循环推进直到队列稳定，保证构建期请求的资产在主循环开始前就绪。
- **目前所有 loader 回调（ctor/step/dtor/destroy）都在主线程执行**。step 里禁止长时间阻塞；耗时工作切成多帧，用 PENDING 返回（texture 上传就是这个模式）。

## 2. 注册类型：PulseAssetTypeDesc

类型描述 asset 数据本体的内存布局和销毁方式。资产系统按 `size`/`align` 在自己的内存池里分配 Data 结构，loader 通过 `ctx->out_asset` 拿到指针填充。

```cpp
void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_PREFAB;
    type_desc.size = sizeof(PulsePrefabData);
    type_desc.align = alignof(PulsePrefabData);
    type_desc.destroy = destroy_prefab;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}
```

字段语义：

| 字段 | 说明 |
|---|---|
| `type_id` | 非 0，全局唯一，重复注册返回 `PULSE_RESULT_ERROR_INVALID_STATE` |
| `size` / `align` | Data 结构的大小与对齐；`size` 必须 > 0，`align` 必须是 2 的幂 |
| `destroy` | `void (*)(void* ptr, void* user_data)`。**只在 asset 曾成功 LOADED 后被调用**（卸载或 force_unload 时），负责释放 Data 持有的外部资源；Data 内存本身由资产系统回收 |
| `user_data` | 原样传给 destroy。prefab 传 app（destroy 里要删 ECS 实体），graphics 传 CGPU device（destroy 里要释放 GPU 资源），datatable 传 registry |

destroy 示例（graphics texture，释放 GPU 资源）：

```cpp
static void destroy_texture(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseTextureData* data = static_cast<PulseTextureData*>(ptr);
    if (data->view) cgpu_device_free_texture_view(device, data->view);
    if (data->handle) cgpu_device_free_texture(device, data->handle);
}
```

注意：**加载中途 FAILED 的 asset 不会调用 destroy**。step 失败前必须自己清理已分配的资源。

## 3. 注册 Loader：PulseAssetLoaderDesc

一个类型可以注册多个 loader，按扩展名（文件/内存加载）或 `loader_identifier`（builder）分派。同一类型下扩展名不允许重叠，builder 标识不允许重复。

```cpp
void register_prefab_load_loader(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_PREFAB;
    ld.extensions = "prefab";
    ld.ctor = nullptr;
    ld.dtor = dtor_prefab_load;
    ld.step = step_prefab_load;
    ld.loader_size = sizeof(prefab_load_state);
    ld.loader_align = alignof(prefab_load_state);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);
}
```

字段语义：

| 字段 | 说明 |
|---|---|
| `extensions` | 逗号分隔的小写扩展名列表，如 `"png,jpg,bmp,tga"`、`"ktx,ktx2"`。为空则是 builder loader |
| `loader_identifier` | builder loader 的匹配键；文件 loader 忽略此字段（但会透传到 `ctx->loader_identifier`） |
| `ctor` | 可选。loader state 分配并清零后调用一次，返回非 `PULSE_RESULT_OK` 直接 FAILED |
| `step` | **必填**。加载主循环，可被多帧重复调用，见 §4 |
| `dtor` | 可选。job 结束时（成功、失败、取消都会）调用一次，清理 loader state 持有的资源 |
| `loader_size` / `loader_align` | 跨帧 loader state 结构的大小与对齐，由资产系统分配并**清零**；为 0 则 state 指针为 nullptr |
| `settings_size` / `settings_align` | 加载请求 `settings` 结构的大小与对齐，见 §5 |
| `settings_size_fn` / `settings_copy_fn` | 可选、必须成对，settings 含嵌套指针时提供深拷贝，见 §5.2 |
| `user_data` | 原样透传到 `ctx->user_data`（ctor/step/dtor/settings 回调都能拿到）。graphics 用它传 CGPU device |

注册前置条件：`type_id` 对应的类型必须已注册，否则返回 `PULSE_RESULT_ERROR_NOT_FOUND`。所以插件 build 钩子里先 register_type 再 register_loader。

## 4. step 回调契约

```cpp
typedef EPulseAssetLoaderStatus (*PulseProcAssetLoaderStepFn)(void* state, const PulseAssetLoadTask* ctx, const char** out_error);
```

每帧对未完成 job 调用一次，直到返回终态。返回值：

| 返回值 | 含义 |
|---|---|
| `PULSE_ASSET_LOADER_STATUS_DONE` | 加载完成。资产系统再校验一遍依赖全部就绪后转 LOADED |
| `PULSE_ASSET_LOADER_STATUS_FAILED` | 失败。必须设置 `*out_error`（会被拷贝进 slot，静态字符串即可） |
| `PULSE_ASSET_LOADER_STATUS_PENDING` | 未完成，下帧重入。用于异步工作（texture 等 GPU 上传完成） |
| `PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES` | 本帧通过 dependency_hint 声明了新依赖，挂起等依赖全部 LOADED 后重入，见 §6.2 |

`PulseAssetLoadTask`（即 `ctx`）关键字段：

| 字段 | 说明 |
|---|---|
| `app` / `asset_system` | 用于发起子加载、查询状态、调模块 API |
| `type_id` / `request` | 当前 asset 的身份；`request` 可转成模块瘦 handle（texture loader 用它构造 `PulseTextureHandle` 关联上传回调） |
| `path` | 归一化后的资源路径；builder 加载时为 name，可能为 nullptr |
| `p_bytes` / `bytes_size` | 文件/内存数据。builder 来源时为空 |
| `out_asset` | **Data 结构指针**（类型注册时声明的 size/align），step 的职责就是填充它 |
| `settings` | 请求携带的 settings 拷贝（可能为 nullptr），见 §5 |
| `loader_identifier` | 注册的 builder 标识（文件 loader 为 nullptr） |
| `user_data` | loader 注册时的 user_data |
| `p_dependencies` / `dependencies_count` | 当前已声明的依赖列表（含动态添加的） |
| `dependency_hint` | 仅在 step 调用期间有效，用于动态添加依赖，见 §6.2 |
| `source` | `FILE` / `MEMORY` / `BUILDER` |

跨帧状态放在 loader state（`state` 参数，已清零）里。step 会被重入，典型写法是用 state 里的 bool/阶段字段做幂等判断：

```cpp
EPulseAssetLoaderStatus step_texture_stb(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* s = static_cast<TextureLoaderState*>(state);
    if (!s->upload_requested) {
        auto* texture = static_cast<PulseTextureData*>(ctx->out_asset);
        auto* pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(ctx->p_bytes), static_cast<int>(ctx->bytes_size), &w, &h, &comp, 4);
        if (!pixels) {
            *out_error = "texture stb loader: texture parse failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        init_and_queue_upload(ctx, texture, pixels, &s->upload_completed);
        stbi_image_free(pixels);
        s->upload_requested = true;
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_STATUS_DONE;
    }
    return PULSE_ASSET_LOADER_STATUS_PENDING;
}
```

（对真实代码的简化节选，`init_and_queue_upload` 对应原文的纹理创建 + staging 上传两段。）

同步 loader（prefab、material、datatable）则在一次 step 里解析 + 填充 `out_asset` 后直接返回 DONE。

## 5. Settings

settings 是加载请求携带的、传给 loader 的自定义参数结构（如 `PulseTextureLoadDesc { filepath, generate_mipmaps }`）。

### 5.1 字节拷贝（默认）

注册时 `settings_size = sizeof(YourSettings)`、`settings_align = alignof(YourSettings)`。加载入口会把调用者的 settings **按字节拷贝**进资产池，step 里通过 `ctx->settings` 读取。调用者的原结构在入口返回后即可释放——但如果结构里有指针，指针指向的内存不会被拷贝，**必须存活到 job 结束**，否则提供深拷贝回调。

### 5.2 深拷贝（嵌套指针）

提供成对的 `settings_size_fn` + `settings_copy_fn`：

- `settings_size_fn(settings, user_data)`：返回深拷贝所需总字节数（结构体 + 全部嵌套数据）。
- `settings_copy_fn(dst, src, byte_size, user_data)`：结构体字节已被 memcpy 到 dst 头部，回调负责把嵌套数据排布进 dst 块内并把嵌套指针改指到块内。布局必须与 size_fn 计算的一致。

完整参考 `src/pulse_graphics/src/loader/create_texture.cpp`（name 字符串 + 像素数据两段嵌套数据的排布）。

校验规则：`settings_size == 0` 时不允许提供 size_fn/copy_fn；size_fn 与 copy_fn 必须同时提供或同时为空。

## 6. 依赖

依赖让"材质引用着色器和贴图"这类资产图正确加载：依赖未就绪时挂起，依赖失败时级联失败，加载完成后依赖被 pin 住不会被提前卸载。

### 6.1 静态依赖（请求时已知）

load/build desc 里直接带 `dependencies` 数组：

```cpp
PulseAssetDependency dep = { pulse_asset_system_to_asset_dep_ref_from_handle(asset_system, shader_handle), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED };
PulseAssetLoadDesc desc{};
desc.struct_size = sizeof(PulseAssetLoadDesc);
desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
desc.type_id = PULSE_TYPE_MATERIAL;
desc.path = path;
desc.p_dependencies = &dep;
desc.dependencies_count = 1;
pulse_asset_system_load(asset_system, &desc);
```

`EPulseLoadDependencyRequirement` 语义：

| 值 | 依赖未 LOADED | 依赖 FAILED / 句柄无效 |
|---|---|---|
| `REQUIRED` | 挂起等待（WAITING_DEPENDENCIES） | 本 asset 级联 FAILED |
| `OPTIONAL` | 不等，直接继续 | 忽略 |

### 6.2 动态依赖（step 里解析出来才知道）

材质文件要先解析才知道引用了哪些贴图，用这个模式（完整参考 `load_material.cpp`）：

1. 首次进入 step：解析 `ctx->p_bytes`，对每个依赖调用对应模块的 load API 发起子加载（路径缓存保证与别处共享同一实例），拿到子 request。
2. 用 `pulse_asset_load_task_add_dependency(ctx->dependency_hint, dep_ref, requirement)` 把每个子 request 登记为依赖，dep_ref 用 `pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, req)` 转换。
3. 在 loader state 里记下"依赖已声明"，返回 `PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES`。
4. 依赖全部 LOADED 后 step 重入：对每个依赖 `pulse_xxx_get_handle` + `pulse_asset_system_borrow` 拿到依赖的 Data 指针，填充自己的 `out_asset`，返回 DONE。

`dependency_hint` 只在 step 调用期间有效，不要存进 loader state。

prefab 用的是另一种等价写法：不返回 WAIT_DEPENDENCIES，而是发起子加载后轮询 `pulse_asset_system_get_state`，任一依赖仍在加载态就返回 PENDING（见 `load_prefab.cpp` 的 `step_prefab_load`）。区别：WAIT_DEPENDENCIES 由依赖图精确驱动并建立 pin 关系，PENDING 轮询每帧重查；**需要依赖被本 asset pin 住时必须走 add_dependency**。

## 7. 加载入口与路径缓存

- 同一 `(type_id, 归一化path)` 的 File/Memory 加载命中缓存直接返回同一 request（不重复读文件、不重复跑 loader）。归一化统一路径分隔符与大小写。
- `PULSE_ASSET_LOAD_SKIP_CACHE` 强制绕过缓存新建实例。
- builder 加载不走缓存，每次都是新实例。
- 加载入口返回无效 request 的原因：类型未注册、扩展名没有匹配的 loader、path 为空、memory 来源 data/size 无效、builder 标识不匹配。
- `pulse_asset_system_find_loaded(type_id, path)`：查询某路径是否已 LOADED，是则返回 handle。
- `pulse_asset_system_get_path(handle)`：反查某 handle 对应的归一化路径（builder 资产无固定文件路径，返回其 name 或 nullptr）。
- `pulse_asset_system_cancel(request)`：放弃一次加载（本质是 release 请求持有的那份引用）。

## 8. 访问、引用计数与卸载

### 8.1 正常访问路径

```cpp
PulseFooRequest request = pulse_load_foo(app, "assets/a.foo");
// 之后某帧：
if (pulse_foo_is_ready(app, request)) {
    PulseFooHandle handle = pulse_foo_get_handle(app, request);
    void* ptr = nullptr;
    EPulseBorrowErrorCode err = PULSE_BORROW_ERROR_CODE_ASSET_SYSTEM_IS_INVALID;
    if (pulse_asset_system_borrow(asset_system, pulse_foo_to_handle(handle), &ptr, &err)) {
        PulseFooData* data = static_cast<PulseFooData*>(ptr);
    }
}
```

`get_handle` 只在 LOADED 时返回有效 handle；`borrow` 在非 PENDING_DELETE 时返回 Data 指针。模块内部普遍封装 `internal_borrow_xxx` 帮助函数（见 `graphics_internal.h`）。

### 8.2 引用计数

- 加载入口成功返回时 slot `pin_count = 1`（请求本身持有一份引用）。**命中缓存的重复加载返回同一 request，不会增加 pin**。
- `retain(handle)` / `release(handle)` 手动增减引用；release 到 0 且无 dependent 时立即卸载（调 destroy、回收 slot、generation+1，旧 handle 失效）。
- 加载完成时依赖被 commit 进依赖图：dependent 存在也会阻止依赖被卸载；retain/release 会级联 pin/unpin 已 commit 的依赖。
- 加载中 release 到 0：转 PENDING_DELETE，job 收尾后卸载。
- retain/release/borrow 都有 out 错误码（`EPulseRetainErrorCode` 等），over-release、pending-delete、句柄失效可区分。

### 8.3 卸载

- 自然卸载：引用归零自动发生。
- `pulse_asset_system_force_unload_assets(type_id)`：无视引用计数，取消该类型所有在途 job 并销毁全部实例。**每个拥有 asset 类型的插件必须在自己的 shutdown 钩子里调用它**，否则插件卸载后 destroy 回调指向已卸载的代码：

```cpp
void prefab_plugin_shutdown(PulseAppId app, void* ctx) {
    (void)ctx;
    pulse_asset_system_force_unload_assets(pulse_get_asset_system(app), PULSE_TYPE_PREFAB);
}
```

- `pulse_asset_system_mark_modified(handle)`：给 LOADED 的 asset 版本号 +1，供上层做热更/脏检查。

## 9. 实操：接入一种新 asset 类型

以在插件 `pulse_foo` 中引入 `.foo` 文本资产（内容是一个整数）为例，走完全流程。

### 9.1 IDL（`src/pulse_foo/idl/pulse_foo.idl`）

```
version(1)

typedef "bool"
typedef "int32_t"
typedef "uint32_t"
typedef "uint64_t"

--- From pulse_app.h
typedef "EPulseAppAddPluginResult"
typedef "PulseAppId"

const_value.FooPluginDescVersion { value = "1u" }

--- Asset type id range 0x5000+ belongs to pulse_foo
const_value.TypeFoo { value = "UINT64_C(0x5000)" }

--- From pulse_asset.h (defined by PULSE_DEFINE_ASSET_TYPE in pulse_foo.h)
typedef "PulseFooHandle"
typedef "PulseFooRequest"

struct.FooData
    .value "int32_t"
    ()

func.AddFooPlugin
    "EPulseAppAddPluginResult"
    .app "PulseAppId"
    ()

func.LoadFoo
    "PulseFooRequest"
    .app "PulseAppId"
    .filepath "cstring"
    ()
```

temp 头（`idl/temp.pulse_foo.h`）手写区加一行类型宏（照抄 `temp.pulse_prefab.h` 的结构，`#include "pulse_asset.h"`，放在 `$cids` 之后、`$cstructs` 之前）：

```c
PULSE_DEFINE_ASSET_TYPE(foo, PULSE_TYPE_FOO, PulseFooHandle, PulseFooRequest)
```

它会生成瘦结构 typedef、互转函数和 get_handle/is_ready/is_alive/get_error 查询函数，IDL 里不再声明这些 func。

然后用 idl 工具生成 `include/pulse_foo.h`（禁止手改生成物，流程见 [IDL 文档](IDL.md)）。

### 9.2 类型与 loader 实现（`src/pulse_foo/src/load_foo.cpp`）

```cpp
#include "foo_internal.h"

#include <cstdio>
#include <string>

namespace pulse_foo_internal {

static void destroy_foo(void* ptr, void* user_data) {
    (void)ptr;
    (void)user_data;
}

static EPulseAssetLoaderStatus step_foo_load(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    (void)state;
    std::string text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
    int value = 0;
    if (std::sscanf(text.c_str(), "%d", &value) != 1) {
        *out_error = "foo loader: file is not a single integer";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    auto* data = static_cast<PulseFooData*>(ctx->out_asset);
    data->value = value;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_foo_type(PulseAssetSystemId asset_system) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_FOO;
    type_desc.size = sizeof(PulseFooData);
    type_desc.align = alignof(PulseFooData);
    type_desc.destroy = destroy_foo;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void register_foo_loader(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_FOO;
    ld.extensions = "foo";
    ld.step = step_foo_load;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = 0;
    ld.settings_align = 0;
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_foo_internal

extern "C" {

PulseFooRequest pulse_load_foo(PulseAppId app, const char* filepath) {
    PulseFooRequest result{};
    if (!app || !filepath || !filepath[0]) {
        return result;
    }
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_FOO;
    desc.path = filepath;
    PulseAssetRequest request = pulse_asset_system_load(pulse_get_asset_system(app), &desc);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
```

get_handle/is_ready/is_alive/get_error 不用手写——`PULSE_DEFINE_ASSET_TYPE` 已在生成的 `pulse_foo.h` 里给出（见 §9.1）。只有 get_handle 需要附加业务逻辑时，才在模块源文件里自己实现 `pulse_foo_get_handle`，此时 temp 头改用子宏组合、跳过 GET_HANDLE 子宏以免重定义。

### 9.3 插件接线（`src/pulse_foo/src/foo_plugin.cpp`）

照抄 `prefab_plugin.cpp`：`PulsePluginDesc.dependencies` 声明 `"pulse_asset"`，build 钩子里取 `pulse_get_asset_system(app)` 后先 `register_foo_type` 再 `register_foo_loader`，shutdown 钩子里 `pulse_asset_system_force_unload_assets(asset_system, PULSE_TYPE_FOO)`。

包接入（package.json、package_register.cpp、xmake target、launcher.manifest.json）见[包文档](Package.md)。

## 10. 校验规则速查与权威实现

注册/加载失败先看这里，都是硬校验：

| 调用 | 失败条件 → 结果 |
|---|---|
| `register_type` | desc 为空、type_id==0、struct_size/version 不匹配、size==0、align 非 2 的幂 → `INVALID_ARGUMENT`；type_id 重复 → `INVALID_STATE` |
| `register_loader` | desc 为空、struct_size/version 不匹配、type_id==0、step 为空、loader_size>0 但 align 非 2 的幂、settings_size>0 但 align 非 2 的幂、settings_size==0 却给了 size_fn/copy_fn、size_fn 与 copy_fn 不成对 → `INVALID_ARGUMENT`；类型未注册 → `NOT_FOUND`；扩展名或 builder 标识冲突 → `INVALID_STATE` |
| `load` / `load_from_memory` / `build` | 返回无效 request：类型未注册、找不到匹配 loader、path 空、memory 无数据、builder 标识不匹配、依赖数组指针与数量不一致 |

权威实现位置：

| 内容 | 文件 |
|---|---|
| 公共 API / 结构定义 | `src/pulse_asset/include/pulse_asset.h`（IDL：`src/pulse_asset/idl/pulse_asset.idl`） |
| 注册表与校验 | `src/pulse_asset/src/asset_registry.cpp` |
| 加载入口、缓存、settings 拷贝、依赖初检 | `src/pulse_asset/src/asset.cpp` |
| 加载队列、loader 生命周期、状态推进 | `src/pulse_asset/src/asset_loading.cpp` |
| slot 存储、依赖图、pin/unpin、卸载 | `src/pulse_asset/src/asset_storage.cpp` |
| 插件接线、ECS 系统安装、C API | `src/pulse_asset/src/asset_plugin.cpp` |

参考实现索引：

| 想看的特性 | 去哪看 |
|---|---|
| 最简完整接入（类型 + 单 loader + dtor + destroy + shutdown） | `src/pulse_prefab/src/load_prefab.cpp`、`prefab_plugin.cpp` |
| 多 loader 按扩展名分派 | `src/pulse_graphics/src/loader/load_texture.cpp`（stb + ktx 两个 loader） |
| 异步多帧加载（PENDING） | `load_texture.cpp` 的 `step_texture_stb` / `step_texture_ktx` |
| settings 字节拷贝 | `load_texture.cpp`（`PulseTextureLoadDesc`） |
| settings 深拷贝（size_fn + copy_fn） | `src/pulse_graphics/src/loader/create_texture.cpp` |
| builder loader / build_sync | `create_texture.cpp`、`create_sampler.cpp`、`create_buffer.cpp` |
| 动态依赖（WAIT_DEPENDENCIES + add_dependency） | `src/pulse_graphics/src/loader/load_material.cpp` |
| 依赖轮询（PENDING 写法） | `load_prefab.cpp` 的 `step_prefab_load` |
| user_data 传模块级 registry | `src/pulse_datatable/src/datatable_loader.cpp` |
| 模块侧加载/查询 API 封装、internal_borrow 帮助函数 | `src/pulse_graphics/src/graphics_internal.h` |
