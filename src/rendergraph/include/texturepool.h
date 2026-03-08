#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"

namespace HGEGraphics
{
	struct TextureDescriptor
	{
		uint16_t width = 0;
		uint16_t height = 0;
		uint16_t depth = 0;
		uint16_t mipLevels = 0;
		ECGPUTextureFormat format = CGPU_TEXTURE_FORMAT_UNDEFINED;

		bool operator==(const TextureDescriptor& other) const;
	};

	struct TextureDescriptorHasher
	{
		size_t operator()(const TextureDescriptor& xyz) const {
			size_t seed = 0;

			hash_combine(seed, xyz.width);
			hash_combine(seed, xyz.height);
			hash_combine(seed, xyz.depth);
			hash_combine(seed, xyz.mipLevels);
			hash_combine(seed, xyz.format);

			return seed;
		}
	};

	struct Texture;
	struct TextureWrap
	{
		TextureDescriptor descriptor() const
		{
			return _descriptor;
		}
		TextureDescriptor _descriptor;
		Texture* texture;
	};

	class TexturePool
		: public ResourcePool<TextureDescriptor, TextureWrap, false, true, TextureDescriptorHasher>
	{
	public:
		TexturePool(TexturePool* upstream, std::pmr::memory_resource* const memory_resource);

		TextureWrap* getTexture(uint16_t width, uint16_t height, uint16_t depth, ECGPUTextureFormat format);
	};

	class CgpuTexturePool
		: public TexturePool
	{
	public:
		CgpuTexturePool(CGPUDeviceId device, CGPUQueueId gfx_queue, TexturePool* upstream, std::pmr::memory_resource* const memory_resource);

	protected:
		TextureWrap* getResource_impl(const TextureDescriptor& descriptor) override;
		void destroyResource_impl(TextureWrap* resource) override;

	private:
		CGPUDeviceId device;
		CGPUQueueId gfx_queue;
		std::pmr::polymorphic_allocator<> allocator;
	};
}
