#pragma once

#include "resourcepool.h"
#include "cgpu/api.h"
#include "hash.h"
#include <string.h>

struct pulse_compute_shader_data_t;

namespace HGEGraphics
{
	using CPSOKey = pulse_compute_shader_data_t*;

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

		ComputePipeline* getComputePipeline(pulse_compute_shader_data_t* shader);

		// Í¨¹ý ResourcePool ¼Ì³Ð
		virtual ComputePipeline* getResource_impl(const CPSOKey& descriptor) override;

		virtual void destroyResource_impl(ComputePipeline* resource) override;

	private:
		CGPUDeviceId device{ CGPU_NULLPTR };
		std::pmr::polymorphic_allocator<> allocator;
	};
}