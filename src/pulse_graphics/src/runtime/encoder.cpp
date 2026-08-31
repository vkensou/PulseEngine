#include "../graphics_internal.h"
#include "renderer.h"
#include "drawer.h"

static HGEGraphics::RenderPassEncoder* to_cpp_encoder(PulseRenderPassEncoder* encoder) {
    return reinterpret_cast<HGEGraphics::RenderPassEncoder*>(encoder);
}

static HGEGraphics::UploadEncoder* to_cpp_encoder(PulseUploadPassEncoder* encoder) {
    return reinterpret_cast<HGEGraphics::UploadEncoder*>(encoder);
}

static PulseAssetSystemId asset_system_from_encoder(PulseRenderPassEncoder* encoder) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    return (cpp_encoder && cpp_encoder->context) ? cpp_encoder->context->asset_system : nullptr;
}

extern "C" {

void pulse_render_pass_encoder_draw(PulseRenderPassEncoder* encoder, PulseMaterialHandle material, PulseMeshHandle mesh) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, material);
    if (!mat) return;
    PulseMeshData* m = pulse_graphics_internal::internal_borrow_mesh(as, mesh);
    if (!m) return;
    HGEGraphics::draw(cpp_encoder, mat, m);
}

void pulse_render_pass_encoder_draw_submesh(PulseRenderPassEncoder* encoder, PulseMaterialHandle material, PulseMeshHandle mesh, uint32_t idx_count, uint32_t first_idx, uint32_t vtx_count, uint32_t first_vtx) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, material);
    if (!mat) return;
    PulseMeshData* m = pulse_graphics_internal::internal_borrow_mesh(as, mesh);
    if (!m) return;
    HGEGraphics::draw_submesh(cpp_encoder, mat, m, idx_count, first_idx, vtx_count, first_vtx);
}

void pulse_render_pass_encoder_draw_procedure(PulseRenderPassEncoder* encoder, PulseMaterialHandle material, ECGPUPrimitiveTopology topology, uint32_t vertex_count) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, material);
    if (!mat) return;
    HGEGraphics::draw_procedure(cpp_encoder, mat, topology, vertex_count);
}

void pulse_render_pass_encoder_dispatch(PulseRenderPassEncoder* encoder, PulseComputeShaderHandle compute_shader, uint32_t x, uint32_t y, uint32_t z) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseComputeShaderData* cs = pulse_graphics_internal::internal_borrow_compute_shader(as, compute_shader);
    if (!cs) return;
    HGEGraphics::dispatch(cpp_encoder, cs, x, y, z);
}

void pulse_render_pass_encoder_set_global_texture(PulseRenderPassEncoder* encoder, PulseTextureHandle texture, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseTextureData* tex = pulse_graphics_internal::internal_borrow_texture(as, texture);
    if (!tex) return;
    HGEGraphics::set_global_texture(cpp_encoder, tex, (int)set, (int)binding);
}

void pulse_render_pass_encoder_set_global_buffer(PulseRenderPassEncoder* encoder, PulseGraphicsBufferHandle buffer, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseGraphicsBufferData* buf = pulse_graphics_internal::internal_borrow_buffer(as, buffer);
    if (!buf) return;
    HGEGraphics::set_global_buffer(cpp_encoder, buf, (int)set, (int)binding);
}

void pulse_render_pass_encoder_set_global_sampler(PulseRenderPassEncoder* encoder, PulseSamplerHandle sampler, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseSamplerData* smp = pulse_graphics_internal::internal_borrow_sampler(as, sampler);
    if (!smp) return;
    HGEGraphics::set_global_sampler(cpp_encoder, smp->handle, (int)set, (int)binding);
}

void pulse_render_pass_encoder_set_global_texture_handle(PulseRenderPassEncoder* encoder, PulseRGTextureHandle handle, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_texture_handle(cpp_encoder, handle, (int)set, (int)binding);
    }
}

void pulse_render_pass_encoder_set_global_buffer_handle(PulseRenderPassEncoder* encoder, PulseRGBufferHandle handle, uint32_t set, uint32_t binding) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_dynamic_buffer(cpp_encoder, handle, (int)set, (int)binding);
    }
}

void pulse_render_pass_encoder_set_global_buffer_offset(PulseRenderPassEncoder* encoder, PulseRGBufferHandle handle, uint32_t set, uint32_t binding, uint64_t offset, uint64_t size) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_global_buffer_with_offset_size(cpp_encoder, handle, (int)set, (int)binding, offset, size);
    }
}

void pulse_render_pass_encoder_set_viewport(PulseRenderPassEncoder* encoder, float x, float y, float w, float h, float min_d, float max_d) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_viewport(cpp_encoder, x, y, w, h, min_d, max_d);
    }
}

void pulse_render_pass_encoder_set_scissor(PulseRenderPassEncoder* encoder, uint32_t x, uint32_t y, uint32_t w, uint32_t h) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (cpp_encoder) {
        HGEGraphics::set_scissor(cpp_encoder, x, y, w, h);
    }
}

void pulse_render_pass_encoder_push_constants(PulseRenderPassEncoder* encoder, PulseShaderHandle shader, const char* name, const void* data) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    PulseAssetSystemId as = asset_system_from_encoder(encoder);
    if (!cpp_encoder || !as) return;

    PulseShaderData* shader_data = pulse_graphics_internal::internal_borrow_shader(as, shader);
    if (!shader_data) return;
    HGEGraphics::push_constants(cpp_encoder, shader_data, name, data);
}

void pulse_upload_pass_encoder_upload(PulseUploadPassEncoder* encoder, uint64_t offset, uint64_t length, const void* data) {
    auto* cpp_encoder = to_cpp_encoder(encoder);
    if (!cpp_encoder) return;

    HGEGraphics::upload(cpp_encoder, offset, length, data);
}

} // extern "C"
