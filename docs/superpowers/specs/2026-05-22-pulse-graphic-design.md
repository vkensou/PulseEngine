# pulse_graphic Design Spec

## Overview

实现 `pulse_graphic` 模块——纯 C API 的 GPU 图形资源管理层。封装 `renderer.h` 中 C++ 的 Shader/Mesh/Texture/Buffer/Material 概念，通过 `pulse_asset` 的 ref-count 机制管理生命周期，扩展 rendergraph encoder 提供 C 版 draw call API。

同时扩展 `pulse_asset`（依赖加载 + memory 加载）和 `pulse_cgpu_render`（多 callback 优先级队列）。

最终目标：支撑 `examples/snake/main.cpp` 中所有渲染相关能力。

---

## Architecture

```
pulse_app
  ├── pulse_window
  ├── pulse_asset ─────────── pulse_graphic 注册 type + loader
  ├── pulse_cgpu_render ───── 多 callback 优先级队列
  │     └── rendergraph ───── encoder 扩展 (opaque pointer)
  └── pulse_graphic (NEW) ─── GPU 资源 C API
        ├── include/pulse_graphic.h
        └── src/graphic_*.cpp (内部调用 renderer.h C++ 函数)
```

---

## 1. Handle Types

每个 GPU 资源类型用结构体封装 `pulse_asset_handle`，达到类型安全：

```c
typedef struct pulse_shader_t    { pulse_asset_handle asset; } pulse_shader_t;
typedef struct pulse_mesh_t      { pulse_asset_handle asset; } pulse_mesh_t;
typedef struct pulse_texture_t   { pulse_asset_handle asset; } pulse_texture_t;
typedef struct pulse_buffer_t    { pulse_asset_handle asset; } pulse_buffer_t;
typedef struct pulse_material_t  { pulse_asset_handle asset; } pulse_material_t;
typedef struct pulse_sampler_t   { pulse_asset_handle asset; } pulse_sampler_t;
```

`{0}` 哨兵表示 invalid。

---

## 2. GPU Resource Data Types

资产 slot 内存储的内部数据结构（通过 `pulse_asset_acquire` 获取 `ptr` 后 cast 使用）：

```c
typedef struct pulse_shader_data {
    CGPURootSignatureId root_sig;
    CGPUShaderEntryDescriptor vs;
    CGPUShaderEntryDescriptor ps;
    CGPUBlendStateDescriptor blend_desc;
    CGPUBlendAttachmentState blend_attachments[8];
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
} pulse_shader_data_t;

typedef struct pulse_compute_shader_data {
    CGPURootSignatureId root_sig;
    CGPUShaderEntryDescriptor cs;
} pulse_compute_shader_data_t;

typedef struct pulse_mesh_data {
    CGPUVertexLayout vertex_layout;
    uint32_t vertex_stride, index_stride;
    uint32_t vertices_count, index_count;
    ECGPUPrimitiveTopology prim_topology;
    CGPUBufferId vertex_buffer;
    CGPUBufferId index_buffer;
    bool has_index_buffer;
} pulse_mesh_data_t;

typedef struct pulse_texture_data {
    CGPUTextureId handle;
    CGPUTextureViewId view;
    uint32_t width, height, depth, mip_levels;
    ECGPUTextureFormat format;
} pulse_texture_data_t;

typedef struct pulse_buffer_data {
    CGPUBufferId handle;
    ECGPUResourceTypeFlags type;
    uint64_t size;
} pulse_buffer_data_t;

typedef struct pulse_material_binding {
    enum { BIND_BUFFER, BIND_TEXTURE, BIND_SAMPLER } type;
    uint32_t set, binding;
    pulse_asset_handle resource;  // buffer/texture/sampler handle
} pulse_material_binding_t;

typedef struct pulse_material_data {
    pulse_asset_handle shader;
    // Internal binding list stored alongside in slot-allocated memory;
    // accessed via acquire for draw resolution.
} pulse_material_data_t;

typedef struct pulse_sampler_data {
    CGPUSamplerId handle;
} pulse_sampler_data_t;
```

---

## 3. Creation API (同步，从内存立即创建)

所有创建函数内部：
1. 从 `pulse_cgpu_render` 获取 `CGPUDeviceId`
2. 调用 `cgpu_*` 分配 GPU 对象
3. 向 `pulse_asset` 注册 slot → 状态直接 `LOADED`
4. 返回 typed handle

```c
// Shader
pulse_shader_t pulse_graphic_shader_create_from_binary(
    pulse_app_t app,
    const void* vs_data, uint32_t vs_size,
    const void* fs_data, uint32_t fs_size,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state);

pulse_shader_t pulse_graphic_compute_shader_create_from_binary(
    pulse_app_t app,
    const void* cs_data, uint32_t cs_size);

// Mesh
pulse_mesh_t pulse_graphic_mesh_create_from_data(
    pulse_app_t app,
    const void* vertex_data, uint32_t vertex_count, uint32_t vertex_stride,
    const void* index_data,  uint32_t index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout);

pulse_mesh_t pulse_graphic_mesh_create_dynamic(
    pulse_app_t app,
    uint32_t max_vertex_count, uint32_t vertex_stride,
    uint32_t max_index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout);

// Texture
pulse_texture_t pulse_graphic_texture_create_from_data(
    pulse_app_t app,
    const CGPUTextureDescriptor* desc,
    const void* pixel_data, uint64_t pixel_data_size);

// Buffer
pulse_buffer_t pulse_graphic_buffer_create(
    pulse_app_t app,
    const CGPUBufferDescriptor* desc,
    const void* data, uint64_t data_size);

// Sampler
pulse_sampler_t pulse_graphic_sampler_create(
    pulse_app_t app,
    const CGPUSamplerDescriptor* desc);

// Material
pulse_material_t pulse_graphic_material_create(
    pulse_app_t app,
    pulse_shader_t shader);

// Material Bindings
void pulse_graphic_material_bind_buffer(pulse_app_t, pulse_material_t*, uint32_t set, uint32_t binding, pulse_buffer_t);
void pulse_graphic_material_bind_texture(pulse_app_t, pulse_material_t*, uint32_t set, uint32_t binding, pulse_texture_t);
void pulse_graphic_material_bind_sampler(pulse_app_t, pulse_material_t*, uint32_t set, uint32_t binding, pulse_sampler_t);
```

---

## 4. Load API (异步，从文件加载)

内部流程（以 shader 为例）：
1. `pulse_asset_load(vert_path)` + `pulse_asset_load(frag_path)` 加载 .spv 源文件
2. `pulse_asset_load_with_deps(app, SHADER_TYPE, NULL, {vs, fs}, 2)` 创建复合 GPU asset
3. `WAITING_DEPENDENCIES` → spv 全部 `LOADED` → `PROCESSING` → 编译 GPU shader → `LOADED`

```c
pulse_shader_t  pulse_graphic_shader_load(app, vert_path, frag_path, blend, depth, rasterizer);
pulse_shader_t  pulse_graphic_compute_shader_load(app, comp_path);
pulse_mesh_t    pulse_graphic_mesh_load(app, filepath);
pulse_texture_t pulse_graphic_texture_load(app, filepath, bool mipmap);
```

---

## 5. Acquire/Release (类型安全)

```c
pulse_shader_data_t*          pulse_graphic_shader_acquire(pulse_app_t, pulse_shader_t*);
pulse_compute_shader_data_t*  pulse_graphic_compute_shader_acquire(pulse_app_t, pulse_shader_t*);
pulse_mesh_data_t*            pulse_graphic_mesh_acquire(pulse_app_t, pulse_mesh_t*);
pulse_texture_data_t*         pulse_graphic_texture_acquire(pulse_app_t, pulse_texture_t*);
pulse_buffer_data_t*          pulse_graphic_buffer_acquire(pulse_app_t, pulse_buffer_t*);
pulse_material_data_t*        pulse_graphic_material_acquire(pulse_app_t, pulse_material_t*);
pulse_sampler_data_t*         pulse_graphic_sampler_acquire(pulse_app_t, pulse_sampler_t*);

void pulse_graphic_shader_release(pulse_app_t, pulse_shader_t*);
// ... 同理其他类型
```

内部调用 `pulse_asset_acquire/release`。

---

## 6. Encoder Extension (Draw API)

在 `pulse_renderpass_encoder_t*` 上扩展。内部 cast 为 `RenderPassEncoder*`，解析 handle → C++ 对象 → 调用 `drawer.h` 函数。

```c
// Draw Calls
void pulse_encoder_draw(encoder, pulse_material_t material, pulse_mesh_t mesh);
void pulse_encoder_draw_submesh(encoder, material, mesh, idx_count, first_idx, vtx_count, first_vtx);
void pulse_encoder_draw_procedure(encoder, material, topology, vertex_count);
void pulse_encoder_dispatch(encoder, pulse_shader_t compute_shader, uint32_t x, y, z);

// Global Bindings (pulse_graphic resources)
void pulse_encoder_set_global_texture(encoder, pulse_texture_t texture, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer(encoder, pulse_buffer_t buffer, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_sampler(encoder, pulse_sampler_t sampler, uint32_t set, uint32_t binding);

// Global Bindings (rendergraph dynamic resources)
void pulse_encoder_set_global_texture_handle(encoder, pulse_texture_handle_t handle, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer_handle(encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer_offset(encoder, handle, set, binding, uint64_t offset, uint64_t size);

// Other
void pulse_encoder_set_viewport(encoder, x, y, w, h, min_d, max_d);
void pulse_encoder_set_scissor(encoder, x, y, w, h);
void pulse_encoder_push_constants(encoder, pulse_shader_t shader, const char* name, const void* data);
```

---

## 7. pulse_cgpu_render Extension（多 Callback 优先级）

### API

```c
typedef struct pulse_cgpu_renderer_record_callback_desc {
    pulse_cgpu_render_record_callback callback;
    void* user_data;
    int32_t priority;   // 越小越先执行
} pulse_cgpu_renderer_record_callback_desc;

void pulse_cgpu_render_add_record_callback(pulse_app_t app, const pulse_cgpu_renderer_record_callback_desc* desc);
void pulse_cgpu_render_remove_record_callback(pulse_app_t app, pulse_cgpu_render_record_callback callback);
```

旧 `pulse_cgpu_render_set_record_callback` 标记为 deprecated。

### 优先级约定

| Priority | Owner | Purpose |
|----------|-------|---------|
| -1000 | pulse_graphic | internal upload passes |
| 0 | user | main render pass |
| 1000 | imgui/debug | overlay rendering |

### 内部变更

- `render_internal.h`: `// record_callback 单回调` → `std::pmr::vector<pulse_cgpu_renderer_record_callback_desc> record_callbacks` 按 priority 排序
- `render_systems.cpp` RecordGraph 阶段：遍历 sorted callbacks → 逐个调用

---

## 8. pulse_asset Extension

### 8a. Load from Memory

```c
pulse_asset_handle pulse_asset_load_from_memory(
    pulse_app_t app,
    uint64_t type_id,
    const char* name,           // 去重标识
    const void* data,
    uint64_t size);
```

- `name` 用于 `path_cache` 去重
- 跳过文件 I/O，bytes 直接传给 loader

### 8b. Dependency Loading

```c
pulse_asset_handle pulse_asset_load_with_deps(
    pulse_app_t app,
    uint64_t type_id,
    const char* path,
    const pulse_asset_handle* dependencies,
    uint32_t dependency_count);
```

### 8c. Data Structures

`AssetSlot` 扩展：
```cpp
std::vector<pulse_asset_handle> dependencies;
std::vector<pulse_asset_handle> dependents;  // reverse
uint32_t unresolved_count;
```

### 8d. Pipeline Changes

`process_load_requests_system()` 新增阶段：
- 遍历 `WAITING_DEPENDENCIES` 状态的 slots
- 检查所有 `dependencies` 是否 `LOADED`
- 全部就绪 → `PROCESSING`
- 任一 `FAILED` → 本 slot `FAILED`

### 8e. Acquire/Release Semantics

- `pulse_asset_acquire(parent)` 自动 cascade acquire 所有 `dependencies`
- `pulse_asset_release(parent)` 自动 cascade release 所有 `dependencies`

---

## 9. Internal: Upload Callback

pulse_graphic 在插件初始化时注册一个 priority=-1000 的 record callback。它负责：

1. **动态资源上传**：遍历 pulse_graphic 内部维护的 "pending upload" 队列
   - `pulse_graphic_mesh_update_vertices(mesh, data, count)` 将上传请求入队
   - upload callback 中为每个 pending item 创建 `rendergraph_add_uploadbufferpass`
2. **资源缓存同步**：更新内部 handle → C++ 对象映射，为后续 encoder resolve 做准备

用户也可主动调用：
```c
void pulse_graphic_mesh_update_vertices(pulse_app_t, pulse_mesh_t* mesh, const void* data, uint32_t count);
void pulse_graphic_mesh_update_indices(pulse_app_t, pulse_mesh_t* mesh, const void* data, uint32_t count);
```

这些操作在 record_callback 之外调用（如 ECS system 中），pulse_graphic 将其排队，upload callback 中执行。

### Registered Type IDs

```c
#define PULSE_TYPE_SHADER          UINT64_C(0x1000)
#define PULSE_TYPE_COMPUTE_SHADER  UINT64_C(0x1001)
#define PULSE_TYPE_MESH            UINT64_C(0x1002)
#define PULSE_TYPE_TEXTURE         UINT64_C(0x1003)
#define PULSE_TYPE_BUFFER          UINT64_C(0x1004)
#define PULSE_TYPE_MATERIAL        UINT64_C(0x1005)
#define PULSE_TYPE_SAMPLER         UINT64_C(0x1006)
```

---

## 10. File Layout

```
src/pulse_graphic/
├── include/
│   └── pulse_graphic.h
└── src/
    ├── graphic_internal.h
    ├── graphic_plugin.cpp         # 插件注册 + type/loader 注册 + upload callback
    ├── graphic_shader.cpp         # Shader / ComputeShader create/load/destroy
    ├── graphic_mesh.cpp           # Mesh create/load/destroy
    ├── graphic_texture.cpp        # Texture create/load/destroy
    ├── graphic_buffer.cpp         # Buffer create/destroy
    ├── graphic_material.cpp       # Material create/bind/destroy
    ├── graphic_sampler.cpp        # Sampler create/destroy
    ├── graphic_encoder.cpp        # Encoder draw/bind 扩展
    └── graphic_upload.cpp         # 内部 upload pass 管理
```

### Dependencies

```
pulse_graphic
  ├── pulse_app (插件系统)
  ├── pulse_asset (type 注册 + ref-count)
  ├── pulse_cgpu_render (device + record callback)
  ├── cgpu (CGPU C API)
  └── rendergraph (encoder + resource handle)
```

### Files Changed

| Module | Files | Change |
|--------|-------|--------|
| pulse_graphic | (new) 10 files | 新建 |
| pulse_asset | `pulse_asset.h`, `asset_internal.h`, `asset_loading.cpp`, `asset_storage.cpp` | 依赖加载 + memory 加载 |
| pulse_cgpu_render | `pulse_cgpu_render.h`, `render_internal.h`, `render_systems.cpp` | 多 callback |
| xmake.lua | root | 新增 pulse_graphic target + deps |

---

## 11. Data Flow Summary

### Init
```
pulse_graphic_add_plugin(app)
  → pulse_asset_register_type() x7 (SHADER, MESH, TEXTURE, BUFFER, MATERIAL, SAMPLER, COMPUTE_SHADER)
  → pulse_asset_register_loader() x7
  → pulse_cgpu_render_add_record_callback(upload_cb, priority=-1000)
```

### Sync Create (memory)
```
shader = pulse_graphic_shader_create_from_binary(app, vs_data, fs_data, ...)
  → 获取 CGPUDeviceId
  → cgpu_create_shader → GPU handle
  → pulse_asset 分配 slot → LOADED
  → return pulse_shader_t{.asset = slot}
```

### Async Load (file)
```
shader = pulse_graphic_shader_load(app, "vs.spv", "fs.spv", ...)
  → vs_handle = pulse_asset_load(TYPE_BYTECODE, "vs.spv")
  → fs_handle = pulse_asset_load(TYPE_BYTECODE, "fs.spv")
  → handle = pulse_asset_load_with_deps(SHADER_TYPE, NULL, {vs, fs})
  → WAITING_DEPENDENCIES → (vs+fs LOADED) → PROCESSING → LOADED
```

### Per-Frame Render
```
[RecordGraph phase]
  foreach callback (sorted by priority):
    callback(app, graph, user_data)
      → rendergraph_declare_texture/buffer
      → rendergraph_add_renderpass
      → renderpass_set_executable(cb with passdata)

[Execute phase]
  compile → execute → in executable callback:
    pulse_encoder_set_global_buffer_offset(encoder, ubo, 0, 0, 0, sizeof(PassData))
    for each object:
      pulse_encoder_set_global_buffer_offset(encoder, obj_ubo, 0, 2, offset, sizeof(ObjectData))
      pulse_encoder_draw(encoder, material, mesh)
        → resolve mesh/material handle → GPU objects → cgpu_render_encoder_draw_indexed
```

---

## Scope & Deliverables

### Deliverables
1. **pulse_graphic** — 新建模块，完整 C API
2. **pulse_asset** — 依赖加载 + memory 加载 扩展
3. **pulse_cgpu_render** — 多 callback 优先级队列 重构
4. **xmake.lua** — 新增 target

### Out of Scope
- Hot-reload / 资源变更检测
- 多线程资源创建
- CBV/SRV/UAV 显式区分（binding 类型由 shader 反射决定）
- 多 queue 异步提交
