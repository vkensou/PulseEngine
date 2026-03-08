#pragma once

#include "stdint.h"
#include "rendergraph.h"
#include "drawer.h"
#include "HandmadeMath.h"
#include <taskflow/taskflow.hpp>

typedef struct oval_update_context
{
    float delta_time;
    float time_since_startup;
    double delta_time_double;
    double time_since_startup_double;
    int fps;
} oval_update_context;

typedef struct oval_render_context
{
    float delta_time;
    float time_since_startup;
    float render_interpolation_time;
    double delta_time_double;
    double time_since_startup_double;
    double render_interpolation_time_double;
    size_t currentRenderPacketFrame;
    int fps;
} oval_render_context;

typedef struct oval_submit_context
{
    size_t submitRenderPacketFrame;
} oval_submit_context;

typedef void (*oval_on_submit)(struct oval_device_t* device, oval_submit_context submit_context, HGEGraphics::rendergraph_t& rg, HGEGraphics::texture_handle_t rg_back_buffer);
typedef tf::Taskflow(*oval_on_update)(struct oval_device_t* device, oval_update_context update_context);
typedef void (*oval_on_imgui)(struct oval_device_t* device, oval_render_context render_context);
typedef void (*oval_on_render)(struct oval_device_t* device, oval_render_context render_context);

typedef enum update_frequency_mode_e
{
    UPDATE_FREQUENCY_MODE_FIXED,
    UPDATE_FREQUENCY_MODE_VARIABLE,
} update_frequency_mode_e;

typedef enum render_frequency_mode_e
{
    RENDER_FREQUENCY_MODE_VSYNC,
    RENDER_FREQUENCY_MODE_UNLIMITED,
    RENDER_FREQUENCY_MODE_LIMITED,
} render_frequency_mode_e;

typedef struct oval_device_descriptor
{
    void* userdata;
    oval_on_update on_update;
    oval_on_render on_render;
    oval_on_imgui on_imgui;
    oval_on_submit on_submit;
    uint16_t width;
    uint16_t height;
    update_frequency_mode_e update_frequecy_mode;
    double fixed_update_time_step;
    double max_update_time_step;
    render_frequency_mode_e render_frequecy_mode;
    bool render_need_interpolate;
    uint16_t target_fps;
    bool enable_capture;
    bool enable_profile;
    bool enable_gpu_validation;
} oval_device_descriptor;

typedef struct oval_device_t {
    const oval_device_descriptor descriptor;
    uint16_t width;
    uint16_t height;
} oval_device_t;

typedef struct oval_window_descriptor
{
    const char* title;
    uint16_t width;
    uint16_t height;
    bool resizable;
    oval_on_imgui on_imgui;
} oval_window_descriptor;

typedef struct oval_window_t {
    void* userdata;
    uint16_t width;
    uint16_t height;
    oval_on_imgui on_imgui;
} oval_window_t;

typedef struct oval_graphics_transfer_queue* oval_graphics_transfer_queue_t;

oval_device_t* oval_create_device(const oval_device_descriptor* device_descriptor);
void oval_runloop(oval_device_t* device);
void oval_free_device(oval_device_t* device);
void oval_render_debug_capture(oval_device_t* device);
void oval_query_render_profile(oval_device_t* device, uint32_t* length, const char*** names, const float** durations);

oval_window_t* oval_create_window(oval_device_t* device, const oval_window_descriptor* window_descriptor);
void oval_free_window(oval_device_t* device, oval_window_t* window);

HGEGraphics::Texture* oval_create_texture(oval_device_t* device, const CGPUTextureDescriptor& desc);
HGEGraphics::Texture* oval_create_texture_from_buffer(oval_device_t* device, const CGPUTextureDescriptor& desc, void* data, uint64_t size);
HGEGraphics::Texture* oval_load_texture(oval_device_t* device, const char* filepath, bool mipmap);
void oval_free_texture(oval_device_t* device, HGEGraphics::Texture* texture);
HGEGraphics::Mesh* oval_load_mesh(oval_device_t* device, const char* filepath);
HGEGraphics::Mesh* oval_create_mesh_from_buffer(oval_device_t* device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, const uint8_t* vertex_data, const uint8_t* index_data, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader);
HGEGraphics::Mesh* oval_create_dynamic_mesh(oval_device_t* device, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride);
void oval_free_mesh(oval_device_t* device, HGEGraphics::Mesh* mesh);
HGEGraphics::Shader* oval_create_shader(oval_device_t* device, const std::string& vertPath, const std::string& fragPath, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state);
HGEGraphics::Shader* oval_create_shader(oval_device_t* device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state);
void oval_free_shader(oval_device_t* device, HGEGraphics::Shader* shader);
HGEGraphics::ComputeShader* oval_create_compute_shader(oval_device_t* device, const std::string& compPath);
void oval_free_compute_shader(oval_device_t* device, HGEGraphics::ComputeShader* shader);
CGPUSamplerId oval_create_sampler(oval_device_t* device, const struct CGPUSamplerDescriptor* desc);
void oval_free_sampler(oval_device_t* device, CGPUSamplerId sampler);
HGEGraphics::Buffer* oval_create_buffer(oval_device_t* device, const CGPUBufferDescriptor* desc);
void oval_free_buffer(oval_device_t* device, HGEGraphics::Buffer* buffer);
HGEGraphics::Material* oval_create_material(oval_device_t* device, HGEGraphics::Shader* shader);
void oval_free_material(oval_device_t* device, HGEGraphics::Material* material);
bool oval_texture_prepared(oval_device_t* device, HGEGraphics::Texture* texture);
bool oval_mesh_prepared(oval_device_t* device, HGEGraphics::Mesh* mesh);
HGEGraphics::Buffer* oval_mesh_get_vertex_buffer(oval_device_t* device, HGEGraphics::Mesh* mesh);
oval_graphics_transfer_queue_t oval_graphics_transfer_queue_alloc(oval_device_t* device);
void oval_graphics_transfer_queue_submit(oval_device_t* device, oval_graphics_transfer_queue_t queue);
uint8_t* oval_graphics_transfer_queue_transfer_data_to_buffer(oval_graphics_transfer_queue_t queue, uint64_t size, HGEGraphics::Buffer* buffer);
uint8_t* oval_graphics_transfer_queue_transfer_data_to_texture_full(oval_graphics_transfer_queue_t queue, HGEGraphics::Texture* texture, bool generate_mipmap, uint8_t generate_mipmap_from, uint64_t* size);
uint8_t* oval_graphics_transfer_queue_transfer_data_to_texture_slice(oval_graphics_transfer_queue_t queue, HGEGraphics::Texture* texture, uint32_t mipmap, uint32_t slice, uint64_t* size);
uint8_t* oval_graphics_set_mesh_vertex_data(oval_device_t* device, HGEGraphics::Mesh* mesh, uint64_t* size);
uint8_t* oval_graphics_set_mesh_index_data(oval_device_t* device, HGEGraphics::Mesh* mesh, uint64_t* size);
uint8_t* oval_graphics_set_texture_data_slice(oval_device_t* device, HGEGraphics::Texture* texture, uint32_t mipmap, uint32_t slice, uint64_t* size);
