#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"
#include <string.h>

namespace HGEGraphics
{
	struct TextureView
	{
		CGPUTextureViewDescriptor descriptor() const
		{
			return _descriptor;
		}
		CGPUTextureViewId handle;
		CGPUTextureViewDescriptor _descriptor;
	};

	class TextureViewPool
		: public ResourcePool<CGPUTextureViewDescriptor, TextureView, true, true>
	{
	public:
		TextureViewPool(TextureViewPool* upstream, std::pmr::memory_resource* const memory_resource);

		void destroyRelativeTexture(CGPUTextureId texture);

	protected:
		// Í¨¹ý ResourcePool ¼Ì³Ð
		virtual TextureView* getResource_impl(const CGPUTextureViewDescriptor& descriptor) override;

		virtual void destroyResource_impl(TextureView* resource) override;

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		std::pmr::polymorphic_allocator<> allocator;
	};
}