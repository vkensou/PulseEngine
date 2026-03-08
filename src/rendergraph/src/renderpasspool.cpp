#include "renderpasspool.h"

namespace HGEGraphics
{
	RenerPassPool::RenerPassPool(CGPUDeviceId device, std::pmr::memory_resource* const memory_resource)
		: ResourcePool(12, nullptr, memory_resource), device(device), allocator(memory_resource)
	{
	}
	RenderPass* RenerPassPool::getRenderPass(const CGPURenderPassDescriptor& descriptor)
	{
		return getResource(descriptor);
	}

	RenderPass* RenerPassPool::getResource_impl(const CGPURenderPassDescriptor& key)
	{
		auto cgpuRenderPass = cgpu_device_create_render_pass(device, &key);

		auto renderPass = allocator.new_object<RenderPass>();
		renderPass->renderPass = cgpuRenderPass;
		renderPass->_descriptor = key;
		return renderPass;
	}

	void RenerPassPool::destroyResource_impl(RenderPass* resource)
	{
		cgpu_device_free_render_pass(device, resource->renderPass);
		resource->renderPass = CGPU_NULLPTR;
		resource->_descriptor = {};
		allocator.delete_object(resource);
	}
}