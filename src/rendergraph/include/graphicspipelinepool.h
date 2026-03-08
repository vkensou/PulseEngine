#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"
#include <string.h>
#include "compare.h"

namespace HGEGraphics
{
	struct Shader;
	struct Mesh;
	struct RenderPassEncoder;

	struct PSOKey
	{
		Shader* shader;
		CGPUVertexLayout vertex_layout;
		ECGPUPrimitiveTopology prim_topology;
		CGPUBlendStateDescriptor blend_desc;
		CGPUDepthStateDescriptor depth_desc;
		CGPURasterizerStateDescriptor rasterizer_state;
		CGPURenderPassId render_pass;
		uint32_t subpass;
		uint32_t render_target_count;
	};

	struct PSOKeyHasher
	{
		inline size_t operator()(const PSOKey& key) const
		{
			size_t seed = 0;

			hash_combine(seed, key.shader);
			hash_combine(seed, key.vertex_layout);
			hash_combine(seed, key.prim_topology);
			hash_combine(seed, key.blend_desc);
			hash_combine(seed, key.depth_desc);
			hash_combine(seed, key.rasterizer_state);
			hash_combine(seed, key.render_pass);
			hash_combine(seed, key.subpass);
			hash_combine(seed, key.render_target_count);

			return seed;
		}
	};

	struct PSOKeyEq
	{
		inline bool operator()(const PSOKey& a, const PSOKey& b) const
		{
			return a.shader == b.shader && a.vertex_layout == b.vertex_layout && a.prim_topology == b.prim_topology 
				&& a.blend_desc == b.blend_desc && a.rasterizer_state == b.rasterizer_state && a.render_pass == b.render_pass
				&& a.subpass == b.subpass && a.render_target_count == b.render_target_count;
		}
	};

	struct GraphicsPipeline
	{
		PSOKey descriptor() const
		{
			return _descriptor;
		}
		CGPURenderPipelineId handle;
		PSOKey _descriptor;
	};

	class GraphicsPipelinePool
		: public ResourcePool<PSOKey, GraphicsPipeline, true, true, PSOKeyHasher, PSOKeyEq>
	{
	public:
		GraphicsPipelinePool(CGPUDeviceId device, GraphicsPipelinePool* upstream, std::pmr::memory_resource* const memory_resource);

		GraphicsPipeline* getGraphicsPipeline(RenderPassEncoder* encoder, Shader* shader, Mesh* mesh);
		GraphicsPipeline* getGraphicsPipeline(RenderPassEncoder* encoder, Shader* shader, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout);

		// 通过ResourcePool继承
		virtual GraphicsPipeline* getResource_impl(const PSOKey& descriptor) override;

		virtual void destroyResource_impl(GraphicsPipeline* resource) override;

		bool dynamicStateT1Enabled() const { return dynamic_state_t1; }
		bool dynamicStateT2Enabled() const { return dynamic_state_t2; }
		bool dynamicStateT3Enabled() const { return dynamic_state_t3; }

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		ECGPUDynamicStateFeaturesFlags _dynamic_state_features{ 0 };
		bool dynamic_state_t1{ false };
		bool dynamic_state_t2{ false };
		bool dynamic_state_t3{ false };
		std::pmr::polymorphic_allocator<> allocator;
	};
}