#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"
#include <string.h>

struct PulseComputeShaderData;

namespace HGEGraphics
{
	using CPSOKey = PulseComputeShaderData*;

	struct ComputePipeline
	{
		CPSOKey descriptor() const
		{
			return _descriptor;
		}
		CGPUComputePipelineId handle;
		CPSOKey _descriptor;
	};

	class ComputePipelinePool
		: public ResourcePool<CPSOKey, ComputePipeline, true, true>
	{
	public:
		ComputePipelinePool(CGPUDeviceId device, ComputePipelinePool* upstream, std::pmr::memory_resource* const memory_resource);

		ComputePipeline* getComputePipeline(PulseComputeShaderData* shader);

		// 通过 ResourcePool 继承
		virtual ComputePipeline* getResource_impl(const CPSOKey& descriptor) override;

		virtual void destroyResource_impl(ComputePipeline* resource) override;

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		std::pmr::polymorphic_allocator<> allocator;
	};
}