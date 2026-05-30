#pragma once

#include "cgpu/api.h"
#include "resource_type.h"
#include "pulse_renderer_asset.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct pulse_rendergraph_t pulse_rendergraph_t;
typedef struct pulse_renderpass_encoder_t pulse_renderpass_encoder_t;
typedef struct pulse_upload_encoder_t pulse_upload_encoder_t;

typedef enum pulse_depth_bits_t {
    PULSE_DEPTH_D32 = 32,
    PULSE_DEPTH_D24 = 24,
    PULSE_DEPTH_D16 = 16,
} pulse_depth_bits_t;

typedef struct pulse_renderpass_builder_t {
    pulse_rendergraph_t* render_graph;
    void* pass_node;
    pulse_index_t pass_index;
} pulse_renderpass_builder_t;

typedef void (*pulse_renderpass_executable_t)(pulse_renderpass_encoder_t* encoder, void* userdata);
typedef void (*pulse_uploadpass_executable_t)(pulse_upload_encoder_t* encoder, void* userdata);

pulse_rendergraph_t* pulse_rendergraph_create(uint32_t estimate_resource_count, uint32_t estimate_pass_count, uint32_t estimate_edge_count, void* blit_shader, CGPUSamplerId blit_sampler);
void pulse_rendergraph_destroy(pulse_rendergraph_t* self);
void pulse_rendergraph_reset(pulse_rendergraph_t* self);

bool pulse_rendergraph_texture_handle_valid(pulse_texture_handle_t handle);
bool pulse_rendergraph_buffer_handle_valid(pulse_buffer_handle_t handle);

pulse_texture_handle_t pulse_rendergraph_declare_texture(pulse_rendergraph_t* self);
pulse_texture_handle_t pulse_rendergraph_import_texture(pulse_rendergraph_t* self, pulse_texture_data_t* imported);
pulse_texture_handle_t pulse_rendergraph_import_backbuffer(pulse_rendergraph_t* self, pulse_backbuffer_data_t* imported_backbuffer);
pulse_buffer_handle_t  pulse_rendergraph_declare_buffer(pulse_rendergraph_t* self);
pulse_buffer_handle_t  pulse_rendergraph_import_buffer(pulse_rendergraph_t* self, pulse_buffer_data_t* imported);
pulse_buffer_handle_t  pulse_rendergraph_import_dynamic_buffer(pulse_rendergraph_t* self, void* imported);
pulse_buffer_handle_t  pulse_rendergraph_declare_uniform_buffer_quick(pulse_rendergraph_t* self, uint32_t size, void* data);
pulse_texture_handle_t pulse_rendergraph_declare_texture_subresource(pulse_rendergraph_t* self, pulse_texture_handle_t parent, uint8_t mip_level, uint8_t array_slice);

void pulse_rendergraph_texture_set_extent(pulse_rendergraph_t* self, pulse_texture_handle_t texture, uint32_t width, uint32_t height, uint32_t depth);
void pulse_rendergraph_texture_set_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture, ECGPUTextureFormat format);
void pulse_rendergraph_texture_set_depth_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture, pulse_depth_bits_t depth_bits, bool need_stencil);
uint32_t pulse_rendergraph_texture_get_width(pulse_rendergraph_t* self, pulse_texture_handle_t texture);
uint32_t pulse_rendergraph_texture_get_height(pulse_rendergraph_t* self, pulse_texture_handle_t texture);
uint32_t pulse_rendergraph_texture_get_depth(pulse_rendergraph_t* self, pulse_texture_handle_t texture);
ECGPUTextureFormat pulse_rendergraph_texture_get_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture);

void pulse_rendergraph_buffer_set_size(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, uint32_t size);
void pulse_rendergraph_buffer_set_type(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, ECGPUResourceTypeFlags type);
void pulse_rendergraph_buffer_set_usage(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, ECGPUMemoryUsage usage);
void pulse_rendergraph_buffer_set_hold_on_last(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer);

pulse_renderpass_builder_t pulse_rendergraph_add_renderpass(pulse_rendergraph_t* self, const char* name);
pulse_renderpass_builder_t pulse_rendergraph_add_computepass(pulse_rendergraph_t* self, const char* name);
pulse_renderpass_builder_t pulse_rendergraph_add_holdpass(pulse_rendergraph_t* self, const char* name);
void pulse_rendergraph_add_uploadtexturepass(pulse_rendergraph_t* self, const char* name, pulse_texture_handle_t texture, uint8_t mip_level, uint8_t slice, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata);
void pulse_rendergraph_add_uploadtexturepass_ex(pulse_rendergraph_t* self, const char* name, pulse_texture_handle_t texture, uint8_t mip_level, uint8_t slice, uint64_t size, uint64_t offset, void* data, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata);
void pulse_rendergraph_add_uploadbufferpass(pulse_rendergraph_t* self, const char* name, pulse_buffer_handle_t buffer, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata);
void pulse_rendergraph_add_uploadbufferpass_ex(pulse_rendergraph_t* self, const char* name, pulse_buffer_handle_t buffer, uint64_t size, uint64_t offset, void* data, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata);
void pulse_rendergraph_add_generate_mipmap(pulse_rendergraph_t* self, pulse_texture_handle_t texture, uint8_t from_mip_level);
void pulse_rendergraph_present(pulse_rendergraph_t* self, pulse_texture_handle_t texture);

void pulse_renderpass_add_color_attachment(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture, ECGPULoadAction load_action, uint32_t clear_color, ECGPUStoreAction store_action);
void pulse_renderpass_add_depth_attachment(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture, ECGPULoadAction depth_load_action, float clear_depth, ECGPUStoreAction depth_store_action, ECGPULoadAction stencil_load_action, uint8_t clear_stencil, ECGPUStoreAction stencil_store_action);
void pulse_renderpass_sample(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture);
void pulse_renderpass_use_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer);
void pulse_renderpass_use_buffer_as(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer, ECGPUResourceStateFlags state);
void pulse_renderpass_set_executable(pulse_renderpass_builder_t* self, pulse_renderpass_executable_t executable, uint32_t passdata_size, void** out_passdata);
void pulse_computepass_sample(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture);
void pulse_computepass_use_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer);
void pulse_computepass_use_buffer_as(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer, ECGPUResourceStateFlags state);
void pulse_computepass_readwrite_texture(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture);
void pulse_computepass_readwrite_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer);
void pulse_computepass_set_executable(pulse_renderpass_builder_t* self, pulse_renderpass_executable_t executable, uint32_t passdata_size, void** out_passdata);

uint32_t pulse_rendergraph_add_edge(pulse_rendergraph_t* self, pulse_index_t from, pulse_index_t to, ECGPUResourceStateFlags usage);

CGPUBufferId pulse_rendergraph_resolve_buffer(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t buffer_handle);
CGPUTextureViewId pulse_rendergraph_resolve_texture_view(pulse_renderpass_encoder_t* encoder, pulse_texture_handle_t texture_handle);

#ifdef __cplusplus
}
#endif
