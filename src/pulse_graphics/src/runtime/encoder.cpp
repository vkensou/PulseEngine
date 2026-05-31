#include "../graphics_internal.h"
#include "renderer.h"
#include "drawer.h"

static HGEGraphics::RenderPassEncoder* to_cpp_encoder(pulse_renderpass_encoder_t* encoder) {
    return reinterpret_cast<HGEGraphics::RenderPassEncoder*>(encoder);
}

extern "C" {

void pulse_graphics_encoder_draw(pulse_renderpass_encoder_t* encoder, PulseMaterial material, PulseMesh mesh) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::draw(cpp_encoder, static_cast<pulse_material_data_t*>(material.ptr), static_cast<pulse_mesh_data_t*>(mesh.ptr));
    }
}

void pulse_graphics_encoder_set_global_texture_handle(pulse_renderpass_encoder_t* encoder, pulse_texture_handle_t handle, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_texture_handle(cpp_encoder, handle, (int)set, (int)binding);
    }
}

void pulse_graphics_encoder_set_global_buffer_handle(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_dynamic_buffer(cpp_encoder, handle, (int)set, (int)binding);
    }
}

void pulse_graphics_encoder_set_global_buffer_offset(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding, uint64_t offset, uint64_t size) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_buffer_with_offset_size(cpp_encoder, handle, (int)set, (int)binding, offset, size);
    }
}

void pulse_graphics_encoder_set_viewport(pulse_renderpass_encoder_t* encoder, float x, float y, float w, float h, float min_d, float max_d) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_viewport(cpp_encoder, x, y, w, h, min_d, max_d);
    }
}

void pulse_graphics_encoder_set_scissor(pulse_renderpass_encoder_t* encoder, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_scissor(cpp_encoder, x, y, w, h);
    }
}

void pulse_graphics_encoder_push_constants(pulse_renderpass_encoder_t* encoder, PulseShader shader, const char* name, const void* data) {
    (void)encoder; (void)shader; (void)name; (void)data;
}

void pulse_graphics_encoder_draw_submesh(pulse_renderpass_encoder_t* encoder, PulseMaterial material, PulseMesh mesh, uint32_t idx_count, uint32_t first_idx, uint32_t vtx_count, uint32_t first_vtx) {
    (void)encoder; (void)material; (void)mesh; (void)idx_count; (void)first_idx; (void)vtx_count; (void)first_vtx;
}

void pulse_graphics_encoder_draw_procedure(pulse_renderpass_encoder_t* encoder, PulseMaterial material, ECGPUPrimitiveTopology topology, uint32_t vertex_count) {
    (void)encoder; (void)material; (void)topology; (void)vertex_count;
}

void pulse_graphics_encoder_dispatch(pulse_renderpass_encoder_t* encoder, PulseComputeShader compute_shader, uint32_t x, uint32_t y, uint32_t z) {
    (void)encoder; (void)compute_shader; (void)x; (void)y; (void)z;
}

void pulse_graphics_encoder_set_global_texture(pulse_renderpass_encoder_t* encoder, PulseTexture texture, uint32_t set, uint32_t binding) {
    (void)encoder; (void)texture; (void)set; (void)binding;
}

void pulse_graphics_encoder_set_global_buffer(pulse_renderpass_encoder_t* encoder, PulseBuffer buffer, uint32_t set, uint32_t binding) {
    (void)encoder; (void)buffer; (void)set; (void)binding;
}

void pulse_graphics_encoder_set_global_sampler(pulse_renderpass_encoder_t* encoder, PulseSampler sampler, uint32_t set, uint32_t binding) {
    (void)encoder; (void)sampler; (void)set; (void)binding;
}

} // extern "C"
