#include "internal.h"

#include <new>
#include <vector>

namespace pulse_graphics_internal {

bool create_renderer(pulse_graphics_state* state) {
    PulseGraphicsRenderer& renderer = state->renderer;

    CGPUInstanceDescriptor instance_desc{};
    instance_desc.backend = state->desc.backend;
    instance_desc.enable_debug_layer = state->desc.enable_debug_layer;
    instance_desc.enable_gpu_based_validation = state->desc.enable_gpu_based_validation;
    instance_desc.enable_set_name = state->desc.enable_debug_layer;

    renderer.instance = cgpu_create_instance(&instance_desc);
    if (!renderer.instance) {
        return false;
    }

    uint32_t adapter_count = 0;
    cgpu_instance_enum_adapters(renderer.instance, &adapter_count, CGPU_NULLPTR);
    if (adapter_count == 0) {
        return false;
    }

    std::vector<CGPUAdapterId> adapters(adapter_count);
    cgpu_instance_enum_adapters(renderer.instance, &adapter_count, adapters.data());
    renderer.adapter = adapters[0];

    CGPUQueueGroupDescriptor queue_group{};
    queue_group.queue_type = CGPU_QUEUE_TYPE_GRAPHICS;
    queue_group.queue_count = 1;

    CGPUDeviceDescriptor device_desc{};
    device_desc.queue_group_count = 1;
    device_desc.p_queue_groups = &queue_group;

    renderer.device = cgpu_adapter_create_device(renderer.adapter, &device_desc);
    if (!renderer.device) {
        return false;
    }

    renderer.graphics_queue =
        cgpu_device_get_queue(renderer.device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
    if (!renderer.graphics_queue) {
        return false;
    }

    renderer.present_queue = renderer.graphics_queue;
    renderer.backend = state->desc.backend;
    renderer.swapchain_format = state->desc.swapchain_format;
    renderer.image_count = state->desc.image_count;
    renderer.frame_index = 0;

    CGPURenderPassDescriptor render_pass_desc{};
    render_pass_desc.sample_count = CGPU_SAMPLE_COUNT_1;
    render_pass_desc.color_attachments[0].format = renderer.swapchain_format;
    render_pass_desc.color_attachments[0].load_action = CGPU_LOAD_ACTION_CLEAR;
    render_pass_desc.color_attachments[0].store_action = CGPU_STORE_ACTION_STORE;
    renderer.render_pass =
        cgpu_device_create_render_pass(renderer.device, &render_pass_desc);
    if (!renderer.render_pass) {
        return false;
    }

    state->frames.resize(renderer.image_count);
    for (frame_data& frame : state->frames) {
        if (!frame.init(renderer.device, renderer.graphics_queue)) {
            return false;
        }
    }

    return true;
}

void destroy_renderer(pulse_graphics_state* state) {
    PulseGraphicsRenderer& renderer = state->renderer;

    if (renderer.graphics_queue) {
        cgpu_queue_wait_idle(renderer.graphics_queue);
    }

    for (frame_data& frame : state->frames) {
        frame.destroy();
    }
    state->frames.clear();

    if (renderer.render_pass) {
        cgpu_device_free_render_pass(renderer.device, renderer.render_pass);
        renderer.render_pass = CGPU_NULLPTR;
    }

    if (renderer.graphics_queue) {
        cgpu_device_free_queue(renderer.device, renderer.graphics_queue);
        renderer.graphics_queue = CGPU_NULLPTR;
        renderer.present_queue = CGPU_NULLPTR;
    }

    if (renderer.device) {
        cgpu_adapter_free_device(renderer.device->adapter, renderer.device);
        renderer.device = CGPU_NULLPTR;
    }

    if (renderer.instance) {
        cgpu_free_instance(renderer.instance);
        renderer.instance = CGPU_NULLPTR;
    }

    renderer.adapter = CGPU_NULLPTR;
}

} // namespace pulse_graphics_internal
