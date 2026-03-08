#pragma once

#include <stdint.h>

namespace HGEGraphics
{
    inline uint32_t murmur3(const uint32_t* key, size_t wordCount, uint32_t seed) noexcept {
        uint32_t h = seed;
        size_t i = wordCount;
        do {
            uint32_t k = *key++;
            k *= 0xcc9e2d51u;
            k = (k << 15u) | (k >> 17u);
            k *= 0x1b873593u;
            h ^= k;
            h = (h << 13u) | (h >> 19u);
            h = (h * 5u) + 0xe6546b64u;
        } while (--i);
        h ^= wordCount;
        h ^= h >> 16u;
        h *= 0x85ebca6bu;
        h ^= h >> 13u;
        h *= 0xc2b2ae35u;
        h ^= h >> 16u;
        return h;
    }

    template<typename T>
    struct MurmurHashFn {
        uint32_t operator()(const T& key) const noexcept {
            static_assert(0 == (sizeof(key) & 3u), "Hashing requires a size that is a multiple of 4.");
            return murmur3((const uint32_t*)&key, sizeof(key) / 4, 0);
        }
    };

    // combines two hashes together, faster but less good
    template<class T>
    inline size_t murmur3_combine_fast(size_t seed, const T& v) noexcept {
        MurmurHashFn<T> hasher;
        return seed ^ (hasher(v) << 1u);
    }

    // 辅助函数：组合哈希值
    template<typename T>
    void hash_combine(size_t& seed, const T& value)
    {
        std::hash<T> hasher;
        seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
}

namespace std {
	template <>
	struct hash<CGPUVertexLayout> {
		size_t operator()(const CGPUVertexLayout& a) const noexcept {
			size_t seed = 0;

			HGEGraphics::hash_combine(seed, a.attribute_count);
			HGEGraphics::hash_combine(seed, a.p_attributes);

			return seed;
		}
	};

	template <>
	struct hash<CGPUBlendStateDescriptor> {
		size_t operator()(const CGPUBlendStateDescriptor& a) const noexcept {
			size_t seed = 0;

			HGEGraphics::hash_combine(seed, a.attachment_count);
			HGEGraphics::hash_combine(seed, a.p_attachments);
			HGEGraphics::hash_combine(seed, a.alpha_to_coverage);
			HGEGraphics::hash_combine(seed, a.independent_blend);

			return seed;
		}
	};

	template <>
	struct hash<CGPUDepthStateDescriptor> {
		size_t operator()(const CGPUDepthStateDescriptor& a) const noexcept {
			size_t seed = 0;

			HGEGraphics::hash_combine(seed, a.depth_test);
			HGEGraphics::hash_combine(seed, a.depth_write);
			HGEGraphics::hash_combine(seed, a.depth_op);
			HGEGraphics::hash_combine(seed, a.stencil_test);
			HGEGraphics::hash_combine(seed, a.stencil_read_mask);
			HGEGraphics::hash_combine(seed, a.stencil_write_mask);
			HGEGraphics::hash_combine(seed, a.stencil_front_op);
			HGEGraphics::hash_combine(seed, a.stencil_front_fail_op);
			HGEGraphics::hash_combine(seed, a.depth_front_fail_op);
			HGEGraphics::hash_combine(seed, a.stencil_front_pass_op);
			HGEGraphics::hash_combine(seed, a.stencil_back_op);
			HGEGraphics::hash_combine(seed, a.stencil_back_fail_op);
			HGEGraphics::hash_combine(seed, a.depth_back_fail_op);
			HGEGraphics::hash_combine(seed, a.stencil_back_pass_op);

			return seed;
		}
	};

	template <>
	struct hash<CGPURasterizerStateDescriptor> {
		size_t operator()(const CGPURasterizerStateDescriptor& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.cull_mode);
			HGEGraphics::hash_combine(seed, a.depth_bias);
			HGEGraphics::hash_combine(seed, a.slope_scaled_depth_bias);
			HGEGraphics::hash_combine(seed, a.fill_mode);
			HGEGraphics::hash_combine(seed, a.front_face);
			HGEGraphics::hash_combine(seed, a.enable_multi_sample);
			HGEGraphics::hash_combine(seed, a.enable_scissor);
			HGEGraphics::hash_combine(seed, a.enable_depth_clamp);

			return seed;
		}
	};

	template <>
	struct hash<CGPUBufferDescriptor> {
		size_t operator()(const CGPUBufferDescriptor& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.size);
			HGEGraphics::hash_combine(seed, a.count_buffer);
			HGEGraphics::hash_combine(seed, a.name);
			HGEGraphics::hash_combine(seed, a.descriptors);
			HGEGraphics::hash_combine(seed, a.memory_usage);
			HGEGraphics::hash_combine(seed, a.format);
			HGEGraphics::hash_combine(seed, a.flags);
			HGEGraphics::hash_combine(seed, a.first_element);
			HGEGraphics::hash_combine(seed, a.element_count);
			HGEGraphics::hash_combine(seed, a.element_stride);
			HGEGraphics::hash_combine(seed, a.owner_queue);
			HGEGraphics::hash_combine(seed, a.start_state);
			HGEGraphics::hash_combine(seed, a.prefer_on_device);
			HGEGraphics::hash_combine(seed, a.prefer_on_host);

			return seed;
		}
	};

	template <>
	struct hash<CGPUDescriptorSetDescriptor> {
		size_t operator()(const CGPUDescriptorSetDescriptor& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.root_signature);
			HGEGraphics::hash_combine(seed, a.set_index);

			return seed;
		}
	};

	template <>
	struct hash<CGPUFramebufferDescriptor> {
		size_t operator()(const CGPUFramebufferDescriptor& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.renderpass);
			HGEGraphics::hash_combine(seed, a.attachment_count);
			HGEGraphics::hash_combine(seed, a.p_attachments[0]);
			HGEGraphics::hash_combine(seed, a.p_attachments[1]);
			HGEGraphics::hash_combine(seed, a.p_attachments[2]);
			HGEGraphics::hash_combine(seed, a.p_attachments[3]);
			HGEGraphics::hash_combine(seed, a.p_attachments[4]);
			HGEGraphics::hash_combine(seed, a.p_attachments[5]);
			HGEGraphics::hash_combine(seed, a.p_attachments[6]);
			HGEGraphics::hash_combine(seed, a.p_attachments[7]);
			HGEGraphics::hash_combine(seed, a.p_attachments[8]);
			HGEGraphics::hash_combine(seed, a.width);
			HGEGraphics::hash_combine(seed, a.height);
			HGEGraphics::hash_combine(seed, a.layers);

			return seed;
		}
	};

	template <>
	struct hash<CGPURenderPassDescriptor> {
		size_t operator()(const CGPURenderPassDescriptor& a) const noexcept {
			return HGEGraphics::MurmurHashFn<CGPURenderPassDescriptor>()(a);;
		}
	};

	template <>
	struct hash<CGPUColorAttachment> {
		size_t operator()(const CGPUColorAttachment& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.format);
			HGEGraphics::hash_combine(seed, a.load_action);
			HGEGraphics::hash_combine(seed, a.store_action);

			return seed;
		}
	};

	template <>
	struct hash<CGPUDepthStencilAttachment> {
		size_t operator()(const CGPUDepthStencilAttachment& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.format);
			HGEGraphics::hash_combine(seed, a.depth_load_action);
			HGEGraphics::hash_combine(seed, a.depth_store_action);
			HGEGraphics::hash_combine(seed, a.stencil_load_action);
			HGEGraphics::hash_combine(seed, a.stencil_store_action);

			return seed;
		}
	};

	template <>
	struct hash<CGPUTextureViewDescriptor> {
		size_t operator()(const CGPUTextureViewDescriptor& a) const noexcept {
			size_t seed = 0;

			// 组合各个字段的哈希值
			HGEGraphics::hash_combine(seed, a.name);
			HGEGraphics::hash_combine(seed, a.texture);
			HGEGraphics::hash_combine(seed, a.format);
			HGEGraphics::hash_combine(seed, a.usages);
			HGEGraphics::hash_combine(seed, a.aspects);
			HGEGraphics::hash_combine(seed, a.dims);
			HGEGraphics::hash_combine(seed, a.base_array_layer);
			HGEGraphics::hash_combine(seed, a.array_layer_count);
			HGEGraphics::hash_combine(seed, a.base_mip_level);
			HGEGraphics::hash_combine(seed, a.mip_level_count);

			return seed;
		}
	};
}