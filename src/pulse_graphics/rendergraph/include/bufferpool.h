#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"
#include "compare.h"

namespace HGEGraphics
{
	struct BufferWrap
	{
		CGPUBufferDescriptor descriptor() const
		{
			return _descriptor;
		}
		CGPUBufferId handle;
		CGPUBufferDescriptor _descriptor;
		ECGPUResourceStateFlags cur_state;
	};

	class BufferPool
		: public ResourcePool<CGPUBufferDescriptor, BufferWrap, false, true>
	{
	public:
		BufferPool(CGPUDeviceId device, BufferPool* upstream, std::pmr::memory_resource* const memory_resource);

	protected:
		BufferWrap* getResource_impl(const CGPUBufferDescriptor& descriptor) override;
		void destroyResource_impl(BufferWrap* resource) override;

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		std::pmr::polymorphic_allocator<> allocator;
	};
}