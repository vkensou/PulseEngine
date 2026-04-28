// this file is generated via Daslang automatic binder
// all user modifications will be lost after this file is re-generated

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1102:16
struct CGPULogger_GeneratedAnnotation : ManagedStructureAnnotation<CGPULogger> {
	CGPULogger_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPULogger", ml, "CGPULogger") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(user_data)>("user_data","user_data");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1109:16
struct CGPUAllocator_GeneratedAnnotation : ManagedStructureAnnotation<CGPUAllocator> {
	CGPUAllocator_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUAllocator", ml, "CGPUAllocator") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(user_data)>("user_data","user_data");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1123:16
struct CGPUInstanceDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUInstanceDescriptor> {
	CGPUInstanceDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUInstanceDescriptor", ml, "CGPUInstanceDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(backend)>("backend","backend");
		addField<DAS_BIND_MANAGED_FIELD(enable_debug_layer)>("enable_debug_layer","enable_debug_layer");
		addField<DAS_BIND_MANAGED_FIELD(enable_gpu_based_validation)>("enable_gpu_based_validation","enable_gpu_based_validation");
		addField<DAS_BIND_MANAGED_FIELD(enable_set_name)>("enable_set_name","enable_set_name");
		addField<DAS_BIND_MANAGED_FIELD(logger)>("logger","logger");
		addField<DAS_BIND_MANAGED_FIELD(allocator)>("allocator","allocator");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1134:16
struct CGPUInstanceFeatures_GeneratedAnnotation : ManagedStructureAnnotation<CGPUInstanceFeatures> {
	CGPUInstanceFeatures_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUInstanceFeatures", ml, "CGPUInstanceFeatures") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(specialization_constant)>("specialization_constant","specialization_constant");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1140:16
struct CGPUInstance_GeneratedAnnotation : ManagedStructureAnnotation<CGPUInstance> {
	CGPUInstance_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUInstance", ml, "CGPUInstance") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(proc_table)>("proc_table","proc_table");
		addField<DAS_BIND_MANAGED_FIELD(surfaces_table)>("surfaces_table","surfaces_table");
		addField<DAS_BIND_MANAGED_FIELD(runtime_table)>("runtime_table","runtime_table");
		addField<DAS_BIND_MANAGED_FIELD(backend)>("backend","backend");
		addField<DAS_BIND_MANAGED_FIELD(enable_set_name)>("enable_set_name","enable_set_name");
		addField<DAS_BIND_MANAGED_FIELD(logger)>("logger","logger");
		addField<DAS_BIND_MANAGED_FIELD(allocator)>("allocator","allocator");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1152:16
struct CGPUVendorPreset_GeneratedAnnotation : ManagedStructureAnnotation<CGPUVendorPreset> {
	CGPUVendorPreset_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUVendorPreset", ml, "CGPUVendorPreset") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device_id)>("device_id","device_id");
		addField<DAS_BIND_MANAGED_FIELD(vendor_id)>("vendor_id","vendor_id");
		addField<DAS_BIND_MANAGED_FIELD(driver_version)>("driver_version","driver_version");
		addField<DAS_BIND_MANAGED_FIELD(gpu_name)>("gpu_name","gpu_name");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1161:16
struct CGPUAdapterDetail_GeneratedAnnotation : ManagedStructureAnnotation<CGPUAdapterDetail> {
	CGPUAdapterDetail_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUAdapterDetail", ml, "CGPUAdapterDetail") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(uniform_buffer_alignment)>("uniform_buffer_alignment","uniform_buffer_alignment");
		addField<DAS_BIND_MANAGED_FIELD(upload_buffer_texture_alignment)>("upload_buffer_texture_alignment","upload_buffer_texture_alignment");
		addField<DAS_BIND_MANAGED_FIELD(upload_buffer_texture_row_alignment)>("upload_buffer_texture_row_alignment","upload_buffer_texture_row_alignment");
		addField<DAS_BIND_MANAGED_FIELD(max_vertex_input_bindings)>("max_vertex_input_bindings","max_vertex_input_bindings");
		addField<DAS_BIND_MANAGED_FIELD(wave_lane_count)>("wave_lane_count","wave_lane_count");
		addField<DAS_BIND_MANAGED_FIELD(host_visible_vram_budget)>("host_visible_vram_budget","host_visible_vram_budget");
		addField<DAS_BIND_MANAGED_FIELD(dynamic_state_features)>("dynamic_state_features","dynamic_state_features");
		addField<DAS_BIND_MANAGED_FIELD(support_host_visible_vram)>("support_host_visible_vram","support_host_visible_vram");
		addField<DAS_BIND_MANAGED_FIELD(multidraw_indirect)>("multidraw_indirect","multidraw_indirect");
		addField<DAS_BIND_MANAGED_FIELD(support_geom_shader)>("support_geom_shader","support_geom_shader");
		addField<DAS_BIND_MANAGED_FIELD(support_tessellation)>("support_tessellation","support_tessellation");
		addField<DAS_BIND_MANAGED_FIELD(is_uma)>("is_uma","is_uma");
		addField<DAS_BIND_MANAGED_FIELD(is_virtual)>("is_virtual","is_virtual");
		addField<DAS_BIND_MANAGED_FIELD(is_cpu)>("is_cpu","is_cpu");
		addField<DAS_BIND_MANAGED_FIELD(support_tiled_buffer)>("support_tiled_buffer","support_tiled_buffer");
		addField<DAS_BIND_MANAGED_FIELD(support_tiled_texture)>("support_tiled_texture","support_tiled_texture");
		addField<DAS_BIND_MANAGED_FIELD(support_tiled_volume)>("support_tiled_volume","support_tiled_volume");
		addField<DAS_BIND_MANAGED_FIELD(support_shading_rate)>("support_shading_rate","support_shading_rate");
		addField<DAS_BIND_MANAGED_FIELD(support_shading_rate_mask)>("support_shading_rate_mask","support_shading_rate_mask");
		addField<DAS_BIND_MANAGED_FIELD(support_shading_rate_sv)>("support_shading_rate_sv","support_shading_rate_sv");
		addField<DAS_BIND_MANAGED_FIELD(format_supports)>("format_supports","format_supports");
		addField<DAS_BIND_MANAGED_FIELD(vendor_preset)>("vendor_preset","vendor_preset");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1188:16
struct CGPUAdapter_GeneratedAnnotation : ManagedStructureAnnotation<CGPUAdapter> {
	CGPUAdapter_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUAdapter", ml, "CGPUAdapter") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(instance)>("instance","instance");
		addField<DAS_BIND_MANAGED_FIELD(proc_table_cache)>("proc_table_cache","proc_table_cache");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1195:16
struct CGPUQueueGroupDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueueGroupDescriptor> {
	CGPUQueueGroupDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueueGroupDescriptor", ml, "CGPUQueueGroupDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(queue_type)>("queue_type","queue_type");
		addField<DAS_BIND_MANAGED_FIELD(queue_count)>("queue_count","queue_count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1202:16
struct CGPUDeviceDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDeviceDescriptor> {
	CGPUDeviceDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDeviceDescriptor", ml, "CGPUDeviceDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(disable_pipeline_cache)>("disable_pipeline_cache","disable_pipeline_cache");
		addField<DAS_BIND_MANAGED_FIELD(queue_group_count)>("queue_group_count","queue_group_count");
		addField<DAS_BIND_MANAGED_FIELD(p_queue_groups)>("p_queue_groups","p_queue_groups");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1210:16
struct CGPUDevice_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDevice> {
	CGPUDevice_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDevice", ml, "CGPUDevice") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(adapter)>("adapter","adapter");
		addField<DAS_BIND_MANAGED_FIELD(proc_table_cache)>("proc_table_cache","proc_table_cache");
		addField<DAS_BIND_MANAGED_FIELD(next_texture_id)>("next_texture_id","next_texture_id");
		addField<DAS_BIND_MANAGED_FIELD(is_lost)>("is_lost","is_lost");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1219:16
struct CGPUQueue_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueue> {
	CGPUQueue_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueue", ml, "CGPUQueue") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
		addField<DAS_BIND_MANAGED_FIELD(index)>("index","index");
		addField<DAS_BIND_MANAGED_FIELD(support_timestamp)>("support_timestamp","support_timestamp");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1228:16
struct CGPUFence_GeneratedAnnotation : ManagedStructureAnnotation<CGPUFence> {
	CGPUFence_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUFence", ml, "CGPUFence") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1234:16
struct CGPUSemaphore_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSemaphore> {
	CGPUSemaphore_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSemaphore", ml, "CGPUSemaphore") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1240:16
struct CGPUCommandPool_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCommandPool> {
	CGPUCommandPool_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCommandPool", ml, "CGPUCommandPool") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(queue)>("queue","queue");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1246:16
struct CGPUCommandBuffer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCommandBuffer> {
	CGPUCommandBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCommandBuffer", ml, "CGPUCommandBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(pool)>("pool","pool");
		addField<DAS_BIND_MANAGED_FIELD(current_dispatch)>("current_dispatch","current_dispatch");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1254:16
struct CGPUQueryPool_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueryPool> {
	CGPUQueryPool_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueryPool", ml, "CGPUQueryPool") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(count)>("count","count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1261:16
struct CGPUComputePassEncoder_GeneratedAnnotation : ManagedStructureAnnotation<CGPUComputePassEncoder> {
	CGPUComputePassEncoder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUComputePassEncoder", ml, "CGPUComputePassEncoder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1267:16
struct CGPURenderPassEncoder_GeneratedAnnotation : ManagedStructureAnnotation<CGPURenderPassEncoder> {
	CGPURenderPassEncoder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURenderPassEncoder", ml, "CGPURenderPassEncoder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1273:16
struct CGPURenderPass_GeneratedAnnotation : ManagedStructureAnnotation<CGPURenderPass> {
	CGPURenderPass_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURenderPass", ml, "CGPURenderPass") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1279:16
struct CGPUFramebufferInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUFramebufferInfo> {
	CGPUFramebufferInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUFramebufferInfo", ml, "CGPUFramebufferInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1286:16
struct CGPUFramebuffer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUFramebuffer> {
	CGPUFramebuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUFramebuffer", ml, "CGPUFramebuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(info)>("info","info");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1293:16
struct CGPUStateBuffer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUStateBuffer> {
	CGPUStateBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUStateBuffer", ml, "CGPUStateBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(cmd)>("cmd","cmd");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1300:16
struct CGPURasterStateEncoder_GeneratedAnnotation : ManagedStructureAnnotation<CGPURasterStateEncoder> {
	CGPURasterStateEncoder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURasterStateEncoder", ml, "CGPURasterStateEncoder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1306:16
struct CGPUShaderStateEncoder_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderStateEncoder> {
	CGPUShaderStateEncoder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderStateEncoder", ml, "CGPUShaderStateEncoder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1312:16
struct CGPUUserStateEncoder_GeneratedAnnotation : ManagedStructureAnnotation<CGPUUserStateEncoder> {
	CGPUUserStateEncoder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUUserStateEncoder", ml, "CGPUUserStateEncoder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1318:16
struct CGPUBinder_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBinder> {
	CGPUBinder_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBinder", ml, "CGPUBinder") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(cmd)>("cmd","cmd");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1325:16
struct CGPURootSignaturePoolDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPURootSignaturePoolDescriptor> {
	CGPURootSignaturePoolDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURootSignaturePoolDescriptor", ml, "CGPURootSignaturePoolDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1331:16
struct CGPURootSignaturePool_GeneratedAnnotation : ManagedStructureAnnotation<CGPURootSignaturePool> {
	CGPURootSignaturePool_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURootSignaturePool", ml, "CGPURootSignaturePool") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(pipeline_type)>("pipeline_type","pipeline_type");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1338:16
struct CGPUVertexInput_GeneratedAnnotation : ManagedStructureAnnotation<CGPUVertexInput> {
	CGPUVertexInput_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUVertexInput", ml, "CGPUVertexInput") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(semantics)>("semantics","semantics");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1346:16
struct CGPUShaderResource_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderResource> {
	CGPUShaderResource_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderResource", ml, "CGPUShaderResource") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(name_hash)>("name_hash","name_hash");
		addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
		addField<DAS_BIND_MANAGED_FIELD(dim)>("dim","dim");
		addField<DAS_BIND_MANAGED_FIELD(set)>("set","set");
		addField<DAS_BIND_MANAGED_FIELD(binding)>("binding","binding");
		addField<DAS_BIND_MANAGED_FIELD(size)>("size","size");
		addField<DAS_BIND_MANAGED_FIELD(offset)>("offset","offset");
		addField<DAS_BIND_MANAGED_FIELD(stages)>("stages","stages");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1360:16
struct CGPUShaderReflection_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderReflection> {
	CGPUShaderReflection_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderReflection", ml, "CGPUShaderReflection") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(entry_name)>("entry_name","entry_name");
		addField<DAS_BIND_MANAGED_FIELD(stage)>("stage","stage");
		addField<DAS_BIND_MANAGED_FIELD(vertex_input_count)>("vertex_input_count","vertex_input_count");
		addField<DAS_BIND_MANAGED_FIELD(p_vertex_inputs)>("p_vertex_inputs","p_vertex_inputs");
		addField<DAS_BIND_MANAGED_FIELD(shader_resource_count)>("shader_resource_count","shader_resource_count");
		addField<DAS_BIND_MANAGED_FIELD(p_shader_resources)>("p_shader_resources","p_shader_resources");
		addField<DAS_BIND_MANAGED_FIELD(thread_group_sizes)>("thread_group_sizes","thread_group_sizes");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1372:16
struct CGPUShaderLibrary_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderLibrary> {
	CGPUShaderLibrary_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderLibrary", ml, "CGPUShaderLibrary") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(entry_count)>("entry_count","entry_count");
		addField<DAS_BIND_MANAGED_FIELD(p_entry_reflections)>("p_entry_reflections","p_entry_reflections");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1381:15
struct CGPUConstantSpecializationImpl_GeneratedAnnotation : ManagedStructureAnnotation<CGPUConstantSpecializationImpl> {
	CGPUConstantSpecializationImpl_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUConstantSpecializationImpl", ml, "CGPUConstantSpecializationImpl") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(u)>("u","u");
		addField<DAS_BIND_MANAGED_FIELD(i)>("i","i");
		addField<DAS_BIND_MANAGED_FIELD(f)>("f","f");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1389:16
struct CGPUConstantSpecialization_GeneratedAnnotation : ManagedStructureAnnotation<CGPUConstantSpecialization> {
	CGPUConstantSpecialization_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUConstantSpecialization", ml, "CGPUConstantSpecialization") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(constant_id)>("constant_id","constant_id");
		addField<DAS_BIND_MANAGED_FIELD(spec)>("spec","spec");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1396:16
struct CGPUShaderEntryDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderEntryDescriptor> {
	CGPUShaderEntryDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderEntryDescriptor", ml, "CGPUShaderEntryDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(library)>("library","library");
		addField<DAS_BIND_MANAGED_FIELD(entry)>("entry","entry");
		addField<DAS_BIND_MANAGED_FIELD(stage)>("stage","stage");
		addField<DAS_BIND_MANAGED_FIELD(constant_count)>("constant_count","constant_count");
		addField<DAS_BIND_MANAGED_FIELD(p_constants)>("p_constants","p_constants");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1406:16
struct CGPURootSignatureDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPURootSignatureDescriptor> {
	CGPURootSignatureDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURootSignatureDescriptor", ml, "CGPURootSignatureDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(shader_count)>("shader_count","shader_count");
		addField<DAS_BIND_MANAGED_FIELD(p_shaders)>("p_shaders","p_shaders");
		addField<DAS_BIND_MANAGED_FIELD(static_sampler_count)>("static_sampler_count","static_sampler_count");
		addField<DAS_BIND_MANAGED_FIELD(p_static_samplers)>("p_static_samplers","p_static_samplers");
		addField<DAS_BIND_MANAGED_FIELD(p_static_sampler_names)>("p_static_sampler_names","p_static_sampler_names");
		addField<DAS_BIND_MANAGED_FIELD(push_constant_count)>("push_constant_count","push_constant_count");
		addField<DAS_BIND_MANAGED_FIELD(p_push_constant_names)>("p_push_constant_names","p_push_constant_names");
		addField<DAS_BIND_MANAGED_FIELD(pool)>("pool","pool");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1419:16
struct CGPUParameterTable_GeneratedAnnotation : ManagedStructureAnnotation<CGPUParameterTable> {
	CGPUParameterTable_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUParameterTable", ml, "CGPUParameterTable") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(resources_count)>("resources_count","resources_count");
		addField<DAS_BIND_MANAGED_FIELD(p_resources)>("p_resources","p_resources");
		addField<DAS_BIND_MANAGED_FIELD(set_index)>("set_index","set_index");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1427:16
struct CGPURootSignature_GeneratedAnnotation : ManagedStructureAnnotation<CGPURootSignature> {
	CGPURootSignature_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURootSignature", ml, "CGPURootSignature") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(table_count)>("table_count","table_count");
		addField<DAS_BIND_MANAGED_FIELD(p_tables)>("p_tables","p_tables");
		addField<DAS_BIND_MANAGED_FIELD(push_constant_count)>("push_constant_count","push_constant_count");
		addField<DAS_BIND_MANAGED_FIELD(p_push_constants)>("p_push_constants","p_push_constants");
		addField<DAS_BIND_MANAGED_FIELD(static_sampler_count)>("static_sampler_count","static_sampler_count");
		addField<DAS_BIND_MANAGED_FIELD(p_static_samplers)>("p_static_samplers","p_static_samplers");
		addField<DAS_BIND_MANAGED_FIELD(pipeline_type)>("pipeline_type","pipeline_type");
		addField<DAS_BIND_MANAGED_FIELD(pool)>("pool","pool");
		addField<DAS_BIND_MANAGED_FIELD(pool_sig)>("pool_sig","pool_sig");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1442:16
struct CGPUDescriptorSet_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDescriptorSet> {
	CGPUDescriptorSet_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDescriptorSet", ml, "CGPUDescriptorSet") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
		addField<DAS_BIND_MANAGED_FIELD(index)>("index","index");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1449:16
struct CGPUVertexAttribute_GeneratedAnnotation : ManagedStructureAnnotation<CGPUVertexAttribute> {
	CGPUVertexAttribute_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUVertexAttribute", ml, "CGPUVertexAttribute") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(semantic_name)>("semantic_name","semantic_name");
		addField<DAS_BIND_MANAGED_FIELD(array_size)>("array_size","array_size");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(binding)>("binding","binding");
		addField<DAS_BIND_MANAGED_FIELD(offset)>("offset","offset");
		addField<DAS_BIND_MANAGED_FIELD(elem_stride)>("elem_stride","elem_stride");
		addField<DAS_BIND_MANAGED_FIELD(rate)>("rate","rate");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1461:16
struct CGPUVertexLayout_GeneratedAnnotation : ManagedStructureAnnotation<CGPUVertexLayout> {
	CGPUVertexLayout_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUVertexLayout", ml, "CGPUVertexLayout") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(attribute_count)>("attribute_count","attribute_count");
		addField<DAS_BIND_MANAGED_FIELD(p_attributes)>("p_attributes","p_attributes");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1468:16
struct CGPUBlendAttachmentState_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBlendAttachmentState> {
	CGPUBlendAttachmentState_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBlendAttachmentState", ml, "CGPUBlendAttachmentState") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(enable)>("enable","enable");
		addField<DAS_BIND_MANAGED_FIELD(src_factor)>("src_factor","src_factor");
		addField<DAS_BIND_MANAGED_FIELD(dst_factor)>("dst_factor","dst_factor");
		addField<DAS_BIND_MANAGED_FIELD(src_alpha_factor)>("src_alpha_factor","src_alpha_factor");
		addField<DAS_BIND_MANAGED_FIELD(dst_alpha_factor)>("dst_alpha_factor","dst_alpha_factor");
		addField<DAS_BIND_MANAGED_FIELD(blend_op)>("blend_op","blend_op");
		addField<DAS_BIND_MANAGED_FIELD(blend_alpha_op)>("blend_alpha_op","blend_alpha_op");
		addField<DAS_BIND_MANAGED_FIELD(color_mask)>("color_mask","color_mask");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1481:16
struct CGPUBlendStateDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBlendStateDescriptor> {
	CGPUBlendStateDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBlendStateDescriptor", ml, "CGPUBlendStateDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(attachment_count)>("attachment_count","attachment_count");
		addField<DAS_BIND_MANAGED_FIELD(p_attachments)>("p_attachments","p_attachments");
		addField<DAS_BIND_MANAGED_FIELD(alpha_to_coverage)>("alpha_to_coverage","alpha_to_coverage");
		addField<DAS_BIND_MANAGED_FIELD(independent_blend)>("independent_blend","independent_blend");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1490:16
struct CGPUDepthStateDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDepthStateDescriptor> {
	CGPUDepthStateDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDepthStateDescriptor", ml, "CGPUDepthStateDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(depth_test)>("depth_test","depth_test");
		addField<DAS_BIND_MANAGED_FIELD(depth_write)>("depth_write","depth_write");
		addField<DAS_BIND_MANAGED_FIELD(depth_op)>("depth_op","depth_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_test)>("stencil_test","stencil_test");
		addField<DAS_BIND_MANAGED_FIELD(stencil_read_mask)>("stencil_read_mask","stencil_read_mask");
		addField<DAS_BIND_MANAGED_FIELD(stencil_write_mask)>("stencil_write_mask","stencil_write_mask");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_op)>("stencil_front_op","stencil_front_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_fail_op)>("stencil_front_fail_op","stencil_front_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(depth_front_fail_op)>("depth_front_fail_op","depth_front_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_pass_op)>("stencil_front_pass_op","stencil_front_pass_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_op)>("stencil_back_op","stencil_back_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_fail_op)>("stencil_back_fail_op","stencil_back_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(depth_back_fail_op)>("depth_back_fail_op","depth_back_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_pass_op)>("stencil_back_pass_op","stencil_back_pass_op");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1509:16
struct CGPURasterizerStateDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPURasterizerStateDescriptor> {
	CGPURasterizerStateDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURasterizerStateDescriptor", ml, "CGPURasterizerStateDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(cull_mode)>("cull_mode","cull_mode");
		addField<DAS_BIND_MANAGED_FIELD(depth_bias)>("depth_bias","depth_bias");
		addField<DAS_BIND_MANAGED_FIELD(slope_scaled_depth_bias)>("slope_scaled_depth_bias","slope_scaled_depth_bias");
		addField<DAS_BIND_MANAGED_FIELD(fill_mode)>("fill_mode","fill_mode");
		addField<DAS_BIND_MANAGED_FIELD(front_face)>("front_face","front_face");
		addField<DAS_BIND_MANAGED_FIELD(enable_multi_sample)>("enable_multi_sample","enable_multi_sample");
		addField<DAS_BIND_MANAGED_FIELD(enable_scissor)>("enable_scissor","enable_scissor");
		addField<DAS_BIND_MANAGED_FIELD(enable_depth_clamp)>("enable_depth_clamp","enable_depth_clamp");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1522:16
struct CGPURenderPipelineDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPURenderPipelineDescriptor> {
	CGPURenderPipelineDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURenderPipelineDescriptor", ml, "CGPURenderPipelineDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(dynamic_state)>("dynamic_state","dynamic_state");
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
		addField<DAS_BIND_MANAGED_FIELD(vertex_shader)>("vertex_shader","vertex_shader");
		addField<DAS_BIND_MANAGED_FIELD(tesc_shader)>("tesc_shader","tesc_shader");
		addField<DAS_BIND_MANAGED_FIELD(tese_shader)>("tese_shader","tese_shader");
		addField<DAS_BIND_MANAGED_FIELD(geom_shader)>("geom_shader","geom_shader");
		addField<DAS_BIND_MANAGED_FIELD(fragment_shader)>("fragment_shader","fragment_shader");
		addField<DAS_BIND_MANAGED_FIELD(vertex_layout)>("vertex_layout","vertex_layout");
		addField<DAS_BIND_MANAGED_FIELD(blend_state)>("blend_state","blend_state");
		addField<DAS_BIND_MANAGED_FIELD(depth_state)>("depth_state","depth_state");
		addField<DAS_BIND_MANAGED_FIELD(rasterizer_state)>("rasterizer_state","rasterizer_state");
		addField<DAS_BIND_MANAGED_FIELD(render_pass)>("render_pass","render_pass");
		addField<DAS_BIND_MANAGED_FIELD(subpass)>("subpass","subpass");
		addField<DAS_BIND_MANAGED_FIELD(render_target_count)>("render_target_count","render_target_count");
		addField<DAS_BIND_MANAGED_FIELD(sample_count)>("sample_count","sample_count");
		addField<DAS_BIND_MANAGED_FIELD(prim_topology)>("prim_topology","prim_topology");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1543:16
struct CGPURenderPipeline_GeneratedAnnotation : ManagedStructureAnnotation<CGPURenderPipeline> {
	CGPURenderPipeline_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURenderPipeline", ml, "CGPURenderPipeline") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1550:16
struct CGPUComputePipelineDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUComputePipelineDescriptor> {
	CGPUComputePipelineDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUComputePipelineDescriptor", ml, "CGPUComputePipelineDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
		addField<DAS_BIND_MANAGED_FIELD(compute_shader)>("compute_shader","compute_shader");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1557:16
struct CGPUComputePipeline_GeneratedAnnotation : ManagedStructureAnnotation<CGPUComputePipeline> {
	CGPUComputePipeline_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUComputePipeline", ml, "CGPUComputePipeline") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1564:16
struct CGPUCompiledShader_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCompiledShader> {
	CGPUCompiledShader_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCompiledShader", ml, "CGPUCompiledShader") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1571:16
struct CGPULinkedShader_GeneratedAnnotation : ManagedStructureAnnotation<CGPULinkedShader> {
	CGPULinkedShader_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPULinkedShader", ml, "CGPULinkedShader") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1578:16
struct CGPUDescriptorSetDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDescriptorSetDescriptor> {
	CGPUDescriptorSetDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDescriptorSetDescriptor", ml, "CGPUDescriptorSetDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(root_signature)>("root_signature","root_signature");
		addField<DAS_BIND_MANAGED_FIELD(set_index)>("set_index","set_index");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1585:16
struct CGPUBufferParams_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferParams> {
	CGPUBufferParams_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferParams", ml, "CGPUBufferParams") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(offsets)>("offsets","offsets");
		addField<DAS_BIND_MANAGED_FIELD(sizes)>("sizes","sizes");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1592:16
struct CGPUUavParams_GeneratedAnnotation : ManagedStructureAnnotation<CGPUUavParams> {
	CGPUUavParams_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUUavParams", ml, "CGPUUavParams") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(uav_mip_slice)>("uav_mip_slice","uav_mip_slice");
		addField<DAS_BIND_MANAGED_FIELD(blend_mip_chain)>("blend_mip_chain","blend_mip_chain");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1599:15
struct CGPUDescriptorDataParams_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDescriptorDataParams> {
	CGPUDescriptorDataParams_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDescriptorDataParams", ml, "CGPUDescriptorDataParams") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(buffers_params)>("buffers_params","buffers_params");
		addField<DAS_BIND_MANAGED_FIELD(uav_params)>("uav_params","uav_params");
		addField<DAS_BIND_MANAGED_FIELD(enable_stencil_resource)>("enable_stencil_resource","enable_stencil_resource");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1607:15
struct CGPUDescriptorDataResource_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDescriptorDataResource> {
	CGPUDescriptorDataResource_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDescriptorDataResource", ml, "CGPUDescriptorDataResource") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(ptrs)>("ptrs","ptrs");
		addField<DAS_BIND_MANAGED_FIELD(textures)>("textures","textures");
		addField<DAS_BIND_MANAGED_FIELD(samplers)>("samplers","samplers");
		addField<DAS_BIND_MANAGED_FIELD(buffers)>("buffers","buffers");
		addField<DAS_BIND_MANAGED_FIELD(render_pipelines)>("render_pipelines","render_pipelines");
		addField<DAS_BIND_MANAGED_FIELD(compute_pipelines)>("compute_pipelines","compute_pipelines");
		addField<DAS_BIND_MANAGED_FIELD(descriptor_sets)>("descriptor_sets","descriptor_sets");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1619:16
struct CGPUDescriptorData_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDescriptorData> {
	CGPUDescriptorData_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDescriptorData", ml, "CGPUDescriptorData") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(binding)>("binding","binding");
		addField<DAS_BIND_MANAGED_FIELD(binding_type)>("binding_type","binding_type");
		addField<DAS_BIND_MANAGED_FIELD(params)>("params","params");
		addField<DAS_BIND_MANAGED_FIELD(resources)>("resources","resources");
		addField<DAS_BIND_MANAGED_FIELD(count)>("count","count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1630:16
struct CGPUBufferInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferInfo> {
	CGPUBufferInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferInfo", ml, "CGPUBufferInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(size)>("size","size");
		addField<DAS_BIND_MANAGED_FIELD(cpu_mapped_address)>("cpu_mapped_address","cpu_mapped_address");
		addField<DAS_BIND_MANAGED_FIELD(descriptors)>("descriptors","descriptors");
		addField<DAS_BIND_MANAGED_FIELD(memory_usage)>("memory_usage","memory_usage");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1639:16
struct CGPUBuffer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBuffer> {
	CGPUBuffer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBuffer", ml, "CGPUBuffer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(info)>("info","info");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1646:16
struct CGPUTextureInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureInfo> {
	CGPUTextureInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureInfo", ml, "CGPUTextureInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
		addField<DAS_BIND_MANAGED_FIELD(depth)>("depth","depth");
		addField<DAS_BIND_MANAGED_FIELD(mip_levels)>("mip_levels","mip_levels");
		addField<DAS_BIND_MANAGED_FIELD(array_size_minus_one)>("array_size_minus_one","array_size_minus_one");
		addField<DAS_BIND_MANAGED_FIELD(size_in_bytes)>("size_in_bytes","size_in_bytes");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(sample_count)>("sample_count","sample_count");
		addField<DAS_BIND_MANAGED_FIELD(unique_id)>("unique_id","unique_id");
		addField<DAS_BIND_MANAGED_FIELD(aspect_mask)>("aspect_mask","aspect_mask");
		addField<DAS_BIND_MANAGED_FIELD(owns_image)>("owns_image","owns_image");
		addField<DAS_BIND_MANAGED_FIELD(is_cube)>("is_cube","is_cube");
		addField<DAS_BIND_MANAGED_FIELD(is_allocation_dedicated)>("is_allocation_dedicated","is_allocation_dedicated");
		addField<DAS_BIND_MANAGED_FIELD(is_restrict_dedicated)>("is_restrict_dedicated","is_restrict_dedicated");
		addField<DAS_BIND_MANAGED_FIELD(is_aliasing)>("is_aliasing","is_aliasing");
		addField<DAS_BIND_MANAGED_FIELD(is_tiled)>("is_tiled","is_tiled");
		addField<DAS_BIND_MANAGED_FIELD(is_imported)>("is_imported","is_imported");
		addField<DAS_BIND_MANAGED_FIELD(can_alias)>("can_alias","can_alias");
		addField<DAS_BIND_MANAGED_FIELD(can_export)>("can_export","can_export");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1670:16
struct CGPUTiledSubresourceInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTiledSubresourceInfo> {
	CGPUTiledSubresourceInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTiledSubresourceInfo", ml, "CGPUTiledSubresourceInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(layer)>("layer","layer");
		addField<DAS_BIND_MANAGED_FIELD(mip_level)>("mip_level","mip_level");
		addField<DAS_BIND_MANAGED_FIELD(width_in_tiles)>("width_in_tiles","width_in_tiles");
		addField<DAS_BIND_MANAGED_FIELD(height_in_tiles)>("height_in_tiles","height_in_tiles");
		addField<DAS_BIND_MANAGED_FIELD(depth_in_tiles)>("depth_in_tiles","depth_in_tiles");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1680:16
struct CGPUTiledTextureInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTiledTextureInfo> {
	CGPUTiledTextureInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTiledTextureInfo", ml, "CGPUTiledTextureInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(tile_size)>("tile_size","tile_size");
		addField<DAS_BIND_MANAGED_FIELD(total_tiles_count)>("total_tiles_count","total_tiles_count");
		addField<DAS_BIND_MANAGED_FIELD(alive_tiles_count)>("alive_tiles_count","alive_tiles_count");
		addField<DAS_BIND_MANAGED_FIELD(tile_width_in_texels)>("tile_width_in_texels","tile_width_in_texels");
		addField<DAS_BIND_MANAGED_FIELD(tile_height_in_texels)>("tile_height_in_texels","tile_height_in_texels");
		addField<DAS_BIND_MANAGED_FIELD(tile_depth_in_texels)>("tile_depth_in_texels","tile_depth_in_texels");
		addField<DAS_BIND_MANAGED_FIELD(subresources)>("subresources","subresources");
		addField<DAS_BIND_MANAGED_FIELD(packed_mip_start)>("packed_mip_start","packed_mip_start");
		addField<DAS_BIND_MANAGED_FIELD(packed_mip_count)>("packed_mip_count","packed_mip_count");
		addField<DAS_BIND_MANAGED_FIELD(alive_pack_count)>("alive_pack_count","alive_pack_count");
		addField<DAS_BIND_MANAGED_FIELD(pack_unaligned)>("pack_unaligned","pack_unaligned");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1696:16
struct CGPUTexture_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTexture> {
	CGPUTexture_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTexture", ml, "CGPUTexture") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(info)>("info","info");
		addField<DAS_BIND_MANAGED_FIELD(tiled_resource)>("tiled_resource","tiled_resource");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1704:16
struct CGPUTextureViewDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureViewDescriptor> {
	CGPUTextureViewDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureViewDescriptor", ml, "CGPUTextureViewDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(texture)>("texture","texture");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(usages)>("usages","usages");
		addField<DAS_BIND_MANAGED_FIELD(aspects)>("aspects","aspects");
		addField<DAS_BIND_MANAGED_FIELD(dims)>("dims","dims");
		addField<DAS_BIND_MANAGED_FIELD(base_array_layer)>("base_array_layer","base_array_layer");
		addField<DAS_BIND_MANAGED_FIELD(array_layer_count)>("array_layer_count","array_layer_count");
		addField<DAS_BIND_MANAGED_FIELD(base_mip_level)>("base_mip_level","base_mip_level");
		addField<DAS_BIND_MANAGED_FIELD(mip_level_count)>("mip_level_count","mip_level_count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1719:16
struct CGPUQueryPoolDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueryPoolDescriptor> {
	CGPUQueryPoolDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueryPoolDescriptor", ml, "CGPUQueryPoolDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
		addField<DAS_BIND_MANAGED_FIELD(query_count)>("query_count","query_count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1726:16
struct CGPUTextureView_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureView> {
	CGPUTextureView_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureView", ml, "CGPUTextureView") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(info)>("info","info");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1733:16
struct CGPUSampler_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSampler> {
	CGPUSampler_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSampler", ml, "CGPUSampler") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1739:16
struct CGPUSwapChain_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSwapChain> {
	CGPUSwapChain_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSwapChain", ml, "CGPUSwapChain") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(back_buffer_count)>("back_buffer_count","back_buffer_count");
		addField<DAS_BIND_MANAGED_FIELD(p_back_buffers)>("p_back_buffers","p_back_buffers");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1747:16
struct CGPUSurface_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSurface> {
	CGPUSurface_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSurface", ml, "CGPUSurface") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(instance)>("instance","instance");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1753:16
struct CGPUBufferRange_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferRange> {
	CGPUBufferRange_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferRange", ml, "CGPUBufferRange") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(offset)>("offset","offset");
		addField<DAS_BIND_MANAGED_FIELD(size)>("size","size");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1760:16
struct CGPUMemoryPoolDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUMemoryPoolDescriptor> {
	CGPUMemoryPoolDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUMemoryPoolDescriptor", ml, "CGPUMemoryPoolDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
		addField<DAS_BIND_MANAGED_FIELD(memory_usage)>("memory_usage","memory_usage");
		addField<DAS_BIND_MANAGED_FIELD(block_size)>("block_size","block_size");
		addField<DAS_BIND_MANAGED_FIELD(min_block_count)>("min_block_count","min_block_count");
		addField<DAS_BIND_MANAGED_FIELD(max_block_count)>("max_block_count","max_block_count");
		addField<DAS_BIND_MANAGED_FIELD(min_alloc_alignment)>("min_alloc_alignment","min_alloc_alignment");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1771:16
struct CGPUMemoryPool_GeneratedAnnotation : ManagedStructureAnnotation<CGPUMemoryPool> {
	CGPUMemoryPool_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUMemoryPool", ml, "CGPUMemoryPool") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(device)>("device","device");
		addField<DAS_BIND_MANAGED_FIELD(type)>("_type","type");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1778:16
struct CGPUQueueSubmitDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueueSubmitDescriptor> {
	CGPUQueueSubmitDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueueSubmitDescriptor", ml, "CGPUQueueSubmitDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(cmd_count)>("cmd_count","cmd_count");
		addField<DAS_BIND_MANAGED_FIELD(p_cmds)>("p_cmds","p_cmds");
		addField<DAS_BIND_MANAGED_FIELD(signal_fence)>("signal_fence","signal_fence");
		addField<DAS_BIND_MANAGED_FIELD(wait_semaphore_count)>("wait_semaphore_count","wait_semaphore_count");
		addField<DAS_BIND_MANAGED_FIELD(p_wait_semaphores)>("p_wait_semaphores","p_wait_semaphores");
		addField<DAS_BIND_MANAGED_FIELD(signal_semaphore_count)>("signal_semaphore_count","signal_semaphore_count");
		addField<DAS_BIND_MANAGED_FIELD(p_signal_semaphores)>("p_signal_semaphores","p_signal_semaphores");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1790:16
struct CGPUQueuePresentDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueuePresentDescriptor> {
	CGPUQueuePresentDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueuePresentDescriptor", ml, "CGPUQueuePresentDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(swapchain)>("swapchain","swapchain");
		addField<DAS_BIND_MANAGED_FIELD(wait_semaphore_count)>("wait_semaphore_count","wait_semaphore_count");
		addField<DAS_BIND_MANAGED_FIELD(p_wait_semaphores)>("p_wait_semaphores","p_wait_semaphores");
		addField<DAS_BIND_MANAGED_FIELD(index)>("index","index");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1799:16
struct CGPUCoordinate_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCoordinate> {
	CGPUCoordinate_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCoordinate", ml, "CGPUCoordinate") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(x)>("x","x");
		addField<DAS_BIND_MANAGED_FIELD(y)>("y","y");
		addField<DAS_BIND_MANAGED_FIELD(z)>("z","z");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1807:16
struct CGPUCoordinateRegion_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCoordinateRegion> {
	CGPUCoordinateRegion_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCoordinateRegion", ml, "CGPUCoordinateRegion") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(start)>("start","start");
		addField<DAS_BIND_MANAGED_FIELD(end)>("end","end");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1814:16
struct CGPUTextureCoordinateRegion_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureCoordinateRegion> {
	CGPUTextureCoordinateRegion_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureCoordinateRegion", ml, "CGPUTextureCoordinateRegion") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(start)>("start","start");
		addField<DAS_BIND_MANAGED_FIELD(end)>("end","end");
		addField<DAS_BIND_MANAGED_FIELD(mip_level)>("mip_level","mip_level");
		addField<DAS_BIND_MANAGED_FIELD(layer)>("layer","layer");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1823:16
struct CGPUTiledTextureRegions_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTiledTextureRegions> {
	CGPUTiledTextureRegions_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTiledTextureRegions", ml, "CGPUTiledTextureRegions") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(texture)>("texture","texture");
		addField<DAS_BIND_MANAGED_FIELD(region_count)>("region_count","region_count");
		addField<DAS_BIND_MANAGED_FIELD(p_regions)>("p_regions","p_regions");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1831:16
struct CGPUTiledTexturePackedMip_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTiledTexturePackedMip> {
	CGPUTiledTexturePackedMip_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTiledTexturePackedMip", ml, "CGPUTiledTexturePackedMip") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(texture)>("texture","texture");
		addField<DAS_BIND_MANAGED_FIELD(layer)>("layer","layer");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1838:16
struct CGPUTiledTexturePackedMips_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTiledTexturePackedMips> {
	CGPUTiledTexturePackedMips_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTiledTexturePackedMips", ml, "CGPUTiledTexturePackedMips") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(packed_mip_count)>("packed_mip_count","packed_mip_count");
		addField<DAS_BIND_MANAGED_FIELD(p_packed_mips)>("p_packed_mips","p_packed_mips");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1845:16
struct CGPUColorAttachment_GeneratedAnnotation : ManagedStructureAnnotation<CGPUColorAttachment> {
	CGPUColorAttachment_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUColorAttachment", ml, "CGPUColorAttachment") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(load_action)>("load_action","load_action");
		addField<DAS_BIND_MANAGED_FIELD(store_action)>("store_action","store_action");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1853:16
struct CGPUDepthStencilAttachment_GeneratedAnnotation : ManagedStructureAnnotation<CGPUDepthStencilAttachment> {
	CGPUDepthStencilAttachment_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUDepthStencilAttachment", ml, "CGPUDepthStencilAttachment") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(depth_load_action)>("depth_load_action","depth_load_action");
		addField<DAS_BIND_MANAGED_FIELD(depth_store_action)>("depth_store_action","depth_store_action");
		addField<DAS_BIND_MANAGED_FIELD(stencil_load_action)>("stencil_load_action","stencil_load_action");
		addField<DAS_BIND_MANAGED_FIELD(stencil_store_action)>("stencil_store_action","stencil_store_action");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1863:16
struct CGPURenderPassDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPURenderPassDescriptor> {
	CGPURenderPassDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPURenderPassDescriptor", ml, "CGPURenderPassDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(sample_count)>("sample_count","sample_count");
		addField<DAS_BIND_MANAGED_FIELD(color_attachments)>("color_attachments","color_attachments");
		addField<DAS_BIND_MANAGED_FIELD(depth_stencil)>("depth_stencil","depth_stencil");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1871:16
struct CGPUFramebufferDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUFramebufferDescriptor> {
	CGPUFramebufferDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUFramebufferDescriptor", ml, "CGPUFramebufferDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(renderpass)>("renderpass","renderpass");
		addField<DAS_BIND_MANAGED_FIELD(attachment_count)>("attachment_count","attachment_count");
		addField<DAS_BIND_MANAGED_FIELD(p_attachments)>("p_attachments","p_attachments");
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
		addField<DAS_BIND_MANAGED_FIELD(layers)>("layers","layers");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1882:16
struct CGPUPipelineReflection_GeneratedAnnotation : ManagedStructureAnnotation<CGPUPipelineReflection> {
	CGPUPipelineReflection_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUPipelineReflection", ml, "CGPUPipelineReflection") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(stages)>("stages","stages");
		addField<DAS_BIND_MANAGED_FIELD(shader_resources_count)>("shader_resources_count","shader_resources_count");
		addField<DAS_BIND_MANAGED_FIELD(p_shader_resources)>("p_shader_resources","p_shader_resources");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1890:16
struct CGPUClearValue_GeneratedAnnotation : ManagedStructureAnnotation<CGPUClearValue> {
	CGPUClearValue_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUClearValue", ml, "CGPUClearValue") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(color)>("color","color");
		addField<DAS_BIND_MANAGED_FIELD(depth)>("depth","depth");
		addField<DAS_BIND_MANAGED_FIELD(stencil)>("stencil","stencil");
		addField<DAS_BIND_MANAGED_FIELD(is_color)>("is_color","is_color");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1899:16
struct CGPUQueryDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUQueryDescriptor> {
	CGPUQueryDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUQueryDescriptor", ml, "CGPUQueryDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(index)>("index","index");
		addField<DAS_BIND_MANAGED_FIELD(stage)>("stage","stage");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1906:16
struct CGPUAcquireNextDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUAcquireNextDescriptor> {
	CGPUAcquireNextDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUAcquireNextDescriptor", ml, "CGPUAcquireNextDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(signal_semaphore)>("signal_semaphore","signal_semaphore");
		addField<DAS_BIND_MANAGED_FIELD(fence)>("fence","fence");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1913:16
struct CGPUTextureSubresource_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureSubresource> {
	CGPUTextureSubresource_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureSubresource", ml, "CGPUTextureSubresource") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(aspects)>("aspects","aspects");
		addField<DAS_BIND_MANAGED_FIELD(mip_level)>("mip_level","mip_level");
		addField<DAS_BIND_MANAGED_FIELD(base_array_layer)>("base_array_layer","base_array_layer");
		addField<DAS_BIND_MANAGED_FIELD(layer_count)>("layer_count","layer_count");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1922:16
struct CGPUBufferToBufferTransfer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferToBufferTransfer> {
	CGPUBufferToBufferTransfer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferToBufferTransfer", ml, "CGPUBufferToBufferTransfer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(dst)>("dst","dst");
		addField<DAS_BIND_MANAGED_FIELD(dst_offset)>("dst_offset","dst_offset");
		addField<DAS_BIND_MANAGED_FIELD(src)>("src","src");
		addField<DAS_BIND_MANAGED_FIELD(src_offset)>("src_offset","src_offset");
		addField<DAS_BIND_MANAGED_FIELD(size)>("size","size");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1932:16
struct CGPUBufferToTilesTransfer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferToTilesTransfer> {
	CGPUBufferToTilesTransfer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferToTilesTransfer", ml, "CGPUBufferToTilesTransfer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(src)>("src","src");
		addField<DAS_BIND_MANAGED_FIELD(src_offset)>("src_offset","src_offset");
		addField<DAS_BIND_MANAGED_FIELD(dst)>("dst","dst");
		addField<DAS_BIND_MANAGED_FIELD(region)>("region","region");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1941:16
struct CGPUTextureToTextureTransfer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureToTextureTransfer> {
	CGPUTextureToTextureTransfer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureToTextureTransfer", ml, "CGPUTextureToTextureTransfer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(src)>("src","src");
		addField<DAS_BIND_MANAGED_FIELD(src_subresource)>("src_subresource","src_subresource");
		addField<DAS_BIND_MANAGED_FIELD(dst)>("dst","dst");
		addField<DAS_BIND_MANAGED_FIELD(dst_subresource)>("dst_subresource","dst_subresource");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1950:16
struct CGPUBufferToTextureTransfer_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferToTextureTransfer> {
	CGPUBufferToTextureTransfer_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferToTextureTransfer", ml, "CGPUBufferToTextureTransfer") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(dst)>("dst","dst");
		addField<DAS_BIND_MANAGED_FIELD(dst_subresource)>("dst_subresource","dst_subresource");
		addField<DAS_BIND_MANAGED_FIELD(src)>("src","src");
		addField<DAS_BIND_MANAGED_FIELD(src_offset)>("src_offset","src_offset");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1959:16
struct CGPUBufferBarrier_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferBarrier> {
	CGPUBufferBarrier_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferBarrier", ml, "CGPUBufferBarrier") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(buffer)>("buffer","buffer");
		addField<DAS_BIND_MANAGED_FIELD(src_state)>("src_state","src_state");
		addField<DAS_BIND_MANAGED_FIELD(dst_state)>("dst_state","dst_state");
		addField<DAS_BIND_MANAGED_FIELD(queue_acquire)>("queue_acquire","queue_acquire");
		addField<DAS_BIND_MANAGED_FIELD(queue_release)>("queue_release","queue_release");
		addField<DAS_BIND_MANAGED_FIELD(queue_type)>("queue_type","queue_type");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1970:16
struct CGPUTextureBarrier_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureBarrier> {
	CGPUTextureBarrier_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureBarrier", ml, "CGPUTextureBarrier") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(texture)>("texture","texture");
		addField<DAS_BIND_MANAGED_FIELD(src_state)>("src_state","src_state");
		addField<DAS_BIND_MANAGED_FIELD(dst_state)>("dst_state","dst_state");
		addField<DAS_BIND_MANAGED_FIELD(queue_acquire)>("queue_acquire","queue_acquire");
		addField<DAS_BIND_MANAGED_FIELD(queue_release)>("queue_release","queue_release");
		addField<DAS_BIND_MANAGED_FIELD(queue_type)>("queue_type","queue_type");
		addField<DAS_BIND_MANAGED_FIELD(subresource_barrier)>("subresource_barrier","subresource_barrier");
		addField<DAS_BIND_MANAGED_FIELD(mip_level)>("mip_level","mip_level");
		addField<DAS_BIND_MANAGED_FIELD(array_layer)>("array_layer","array_layer");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1984:16
struct CGPUResourceBarrierDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUResourceBarrierDescriptor> {
	CGPUResourceBarrierDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUResourceBarrierDescriptor", ml, "CGPUResourceBarrierDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(buffer_barrier_count)>("buffer_barrier_count","buffer_barrier_count");
		addField<DAS_BIND_MANAGED_FIELD(p_buffer_barriers)>("p_buffer_barriers","p_buffer_barriers");
		addField<DAS_BIND_MANAGED_FIELD(texture_barrier_count)>("texture_barrier_count","texture_barrier_count");
		addField<DAS_BIND_MANAGED_FIELD(p_texture_barriers)>("p_texture_barriers","p_texture_barriers");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1993:16
struct CGPUCommandPoolDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCommandPoolDescriptor> {
	CGPUCommandPoolDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCommandPoolDescriptor", ml, "CGPUCommandPoolDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:1999:16
struct CGPUCommandBufferDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCommandBufferDescriptor> {
	CGPUCommandBufferDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCommandBufferDescriptor", ml, "CGPUCommandBufferDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(is_secondary)>("is_secondary","is_secondary");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2005:16
struct CGPUSwapChainDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSwapChainDescriptor> {
	CGPUSwapChainDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSwapChainDescriptor", ml, "CGPUSwapChainDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(present_queue_count)>("present_queue_count","present_queue_count");
		addField<DAS_BIND_MANAGED_FIELD(p_present_queues)>("p_present_queues","p_present_queues");
		addField<DAS_BIND_MANAGED_FIELD(surface)>("surface","surface");
		addField<DAS_BIND_MANAGED_FIELD(image_count)>("image_count","image_count");
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
		addField<DAS_BIND_MANAGED_FIELD(enable_vsync)>("enable_vsync","enable_vsync");
		addField<DAS_BIND_MANAGED_FIELD(use_flip_swap_effect)>("use_flip_swap_effect","use_flip_swap_effect");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(old_swap_chain)>("old_swap_chain","old_swap_chain");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2020:16
struct CGPUComputePassDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUComputePassDescriptor> {
	CGPUComputePassDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUComputePassDescriptor", ml, "CGPUComputePassDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2026:16
struct CGPUCompiledShaderDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUCompiledShaderDescriptor> {
	CGPUCompiledShaderDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUCompiledShaderDescriptor", ml, "CGPUCompiledShaderDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(entry)>("entry","entry");
		addField<DAS_BIND_MANAGED_FIELD(shader_code)>("shader_code","shader_code");
		addField<DAS_BIND_MANAGED_FIELD(code_size)>("code_size","code_size");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2034:16
struct CGPUShaderLibraryDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUShaderLibraryDescriptor> {
	CGPUShaderLibraryDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUShaderLibraryDescriptor", ml, "CGPUShaderLibraryDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(code_size)>("code_size","code_size");
		addField<DAS_BIND_MANAGED_FIELD(p_codes)>("p_codes","p_codes");
		addField<DAS_BIND_MANAGED_FIELD(stage)>("stage","stage");
		addField<DAS_BIND_MANAGED_FIELD(reflection_only)>("reflection_only","reflection_only");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2044:16
struct CGPUBufferDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBufferDescriptor> {
	CGPUBufferDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBufferDescriptor", ml, "CGPUBufferDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(size)>("size","size");
		addField<DAS_BIND_MANAGED_FIELD(count_buffer)>("count_buffer","count_buffer");
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(descriptors)>("descriptors","descriptors");
		addField<DAS_BIND_MANAGED_FIELD(memory_usage)>("memory_usage","memory_usage");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(flags)>("flags","flags");
		addField<DAS_BIND_MANAGED_FIELD(first_element)>("first_element","first_element");
		addField<DAS_BIND_MANAGED_FIELD(element_count)>("element_count","element_count");
		addField<DAS_BIND_MANAGED_FIELD(element_stride)>("element_stride","element_stride");
		addField<DAS_BIND_MANAGED_FIELD(owner_queue)>("owner_queue","owner_queue");
		addField<DAS_BIND_MANAGED_FIELD(start_state)>("start_state","start_state");
		addField<DAS_BIND_MANAGED_FIELD(prefer_on_device)>("prefer_on_device","prefer_on_device");
		addField<DAS_BIND_MANAGED_FIELD(prefer_on_host)>("prefer_on_host","prefer_on_host");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2063:16
struct CGPUTextureDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureDescriptor> {
	CGPUTextureDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureDescriptor", ml, "CGPUTextureDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(native_handle)>("native_handle","native_handle");
		addField<DAS_BIND_MANAGED_FIELD(flags)>("flags","flags");
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
		addField<DAS_BIND_MANAGED_FIELD(depth)>("depth","depth");
		addField<DAS_BIND_MANAGED_FIELD(array_size)>("array_size","array_size");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(mip_levels)>("mip_levels","mip_levels");
		addField<DAS_BIND_MANAGED_FIELD(sample_count)>("sample_count","sample_count");
		addField<DAS_BIND_MANAGED_FIELD(sample_quality)>("sample_quality","sample_quality");
		addField<DAS_BIND_MANAGED_FIELD(owner_queue)>("owner_queue","owner_queue");
		addField<DAS_BIND_MANAGED_FIELD(start_state)>("start_state","start_state");
		addField<DAS_BIND_MANAGED_FIELD(descriptors)>("descriptors","descriptors");
		addField<DAS_BIND_MANAGED_FIELD(is_restrict_dedicated)>("is_restrict_dedicated","is_restrict_dedicated");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2083:16
struct CGPUExportTextureDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUExportTextureDescriptor> {
	CGPUExportTextureDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUExportTextureDescriptor", ml, "CGPUExportTextureDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(texture)>("texture","texture");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2089:16
struct CGPUImportTextureDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUImportTextureDescriptor> {
	CGPUImportTextureDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUImportTextureDescriptor", ml, "CGPUImportTextureDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(backend)>("backend","backend");
		addField<DAS_BIND_MANAGED_FIELD(shared_handle)>("shared_handle","shared_handle");
		addField<DAS_BIND_MANAGED_FIELD(width)>("width","width");
		addField<DAS_BIND_MANAGED_FIELD(height)>("height","height");
		addField<DAS_BIND_MANAGED_FIELD(depth)>("depth","depth");
		addField<DAS_BIND_MANAGED_FIELD(size_in_bytes)>("size_in_bytes","size_in_bytes");
		addField<DAS_BIND_MANAGED_FIELD(format)>("format","format");
		addField<DAS_BIND_MANAGED_FIELD(mip_levels)>("mip_levels","mip_levels");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2102:16
struct CGPUTextureAliasingBindDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUTextureAliasingBindDescriptor> {
	CGPUTextureAliasingBindDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUTextureAliasingBindDescriptor", ml, "CGPUTextureAliasingBindDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(aliased)>("aliased","aliased");
		addField<DAS_BIND_MANAGED_FIELD(aliasing)>("aliasing","aliasing");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2109:16
struct CGPUSamplerDescriptor_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSamplerDescriptor> {
	CGPUSamplerDescriptor_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSamplerDescriptor", ml, "CGPUSamplerDescriptor") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(min_filter)>("min_filter","min_filter");
		addField<DAS_BIND_MANAGED_FIELD(mag_filter)>("mag_filter","mag_filter");
		addField<DAS_BIND_MANAGED_FIELD(mipmap_mode)>("mipmap_mode","mipmap_mode");
		addField<DAS_BIND_MANAGED_FIELD(address_u)>("address_u","address_u");
		addField<DAS_BIND_MANAGED_FIELD(address_v)>("address_v","address_v");
		addField<DAS_BIND_MANAGED_FIELD(address_w)>("address_w","address_w");
		addField<DAS_BIND_MANAGED_FIELD(mip_lod_bias)>("mip_lod_bias","mip_lod_bias");
		addField<DAS_BIND_MANAGED_FIELD(max_anisotropy)>("max_anisotropy","max_anisotropy");
		addField<DAS_BIND_MANAGED_FIELD(compare_func)>("compare_func","compare_func");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2123:16
struct CGPUBeginRenderPassInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUBeginRenderPassInfo> {
	CGPUBeginRenderPassInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUBeginRenderPassInfo", ml, "CGPUBeginRenderPassInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(render_pass)>("render_pass","render_pass");
		addField<DAS_BIND_MANAGED_FIELD(framebuffer)>("framebuffer","framebuffer");
		addField<DAS_BIND_MANAGED_FIELD(clear_value_count)>("clear_value_count","clear_value_count");
		addField<DAS_BIND_MANAGED_FIELD(p_clear_values)>("p_clear_values","p_clear_values");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2132:16
struct CGPUEventInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUEventInfo> {
	CGPUEventInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUEventInfo", ml, "CGPUEventInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(color)>("color","color");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2139:16
struct CGPUMarkerInfo_GeneratedAnnotation : ManagedStructureAnnotation<CGPUMarkerInfo> {
	CGPUMarkerInfo_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUMarkerInfo", ml, "CGPUMarkerInfo") {
	}
	void init () {
		addField<DAS_BIND_MANAGED_FIELD(name)>("name","name");
		addField<DAS_BIND_MANAGED_FIELD(color)>("color","color");
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2149:16
struct CGPUProcTable_GeneratedAnnotation : ManagedStructureAnnotation<CGPUProcTable> {
	CGPUProcTable_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUProcTable", ml, "CGPUProcTable") {
	}
	void init () {
	}
};

// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2290:16
struct CGPUSurfacesProcTable_GeneratedAnnotation : ManagedStructureAnnotation<CGPUSurfacesProcTable> {
	CGPUSurfacesProcTable_GeneratedAnnotation(ModuleLibrary & ml) : ManagedStructureAnnotation ("CGPUSurfacesProcTable", ml, "CGPUSurfacesProcTable") {
	}
	void init () {
	}
};

