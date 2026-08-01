#pragma once

#include "cgpu/api.h"
#include "resourcepool.h"
#include "hash.h"
#include "compare.h"

namespace HGEGraphics
{
	struct RenderPass
	{
		CGPURenderPassDescriptor descriptor() const
		{
			return _descriptor;
		}
		CGPURenderPassId renderPass;
		CGPURenderPassDescriptor _descriptor;
	};

	static_assert(sizeof(CGPURenderPassDescriptor) == sizeof(ECGPUSampleCountFlags) + sizeof(CGPUColorAttachment) * CGPU_MAX_MRT_COUNT + sizeof(CGPUDepthStencilAttachment));

	class RenerPassPool
		: public ResourcePool<CGPURenderPassDescriptor, RenderPass, true, true>
	{
	public:
		RenerPassPool(CGPUDeviceId device, std::pmr::memory_resource* const memory_resource);

		RenderPass* getRenderPass(const CGPURenderPassDescriptor& descriptor);

	protected:
		// Í¨¹ý ResourcePool ¼Ì³Ð
		virtual RenderPass* getResource_impl(const CGPURenderPassDescriptor& descriptor) override;
		virtual void destroyResource_impl(RenderPass* resource) override;

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		std::pmr::polymorphic_allocator<> allocator;
	};
}