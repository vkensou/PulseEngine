// this file is generated via Daslang automatic binder
// all user modifications will be lost after this file is re-generated

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"
#include "dasCGPU.h"
#include "need_dasCGPU.h"
namespace das {
#include "dasCGPU.func.aot.decl.inc"
void Module_dasCGPU::initFunctions_1() {
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2302:25
	makeExtern< const CGPUInstance * (*)(const CGPUInstanceDescriptor *) , cgpu_create_instance , SimNode_ExtFuncCall >(lib,"cgpu_create_instance","cgpu_create_instance")
		->args({"desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2303:15
	makeExtern< void (*)(const CGPUInstance *) , cgpu_free_instance , SimNode_ExtFuncCall >(lib,"cgpu_free_instance","cgpu_free_instance")
		->args({"instance"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2304:23
	makeExtern< ECGPUBackend (*)(const CGPUInstance *const) , cgpu_instance_get_backend , SimNode_ExtFuncCall >(lib,"cgpu_instance_get_backend","cgpu_instance_get_backend")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2305:15
	makeExtern< void (*)(const CGPUInstance *const,CGPUInstanceFeatures *) , cgpu_instance_query_instance_features , SimNode_ExtFuncCall >(lib,"cgpu_instance_query_instance_features","cgpu_instance_query_instance_features")
		->args({"_this","features"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2306:15
	makeExtern< void (*)(const CGPUInstance *const,unsigned int *,const CGPUAdapter **) , cgpu_instance_enum_adapters , SimNode_ExtFuncCall >(lib,"cgpu_instance_enum_adapters","cgpu_instance_enum_adapters")
		->args({"_this","p_adapters_count","p_adapters"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2307:24
	makeExtern< const CGPUSurface * (*)(const CGPUInstance *,void *) , cgpu_instance_create_surface_from_native_view , SimNode_ExtFuncCall >(lib,"cgpu_instance_create_surface_from_native_view","cgpu_instance_create_surface_from_native_view")
		->args({"_this","view"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2308:24
	makeExtern< const CGPUSurface * (*)(const CGPUInstance *,HWND__ *) , cgpu_instance_create_surface_from_hwnd , SimNode_ExtFuncCall >(lib,"cgpu_instance_create_surface_from_hwnd","cgpu_instance_create_surface_from_hwnd")
		->args({"_this","window"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2309:24
	// makeExtern< const CGPUSurface * (*)(const CGPUInstance *,ANativeWindow *) , cgpu_instance_create_surface_from_native_window , SimNode_ExtFuncCall >(lib,"cgpu_instance_create_surface_from_native_window","cgpu_instance_create_surface_from_native_window")
	// 	->args({"_this","window"})
	// 	->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2310:15
	makeExtern< void (*)(const CGPUInstance *,const CGPUSurface *) , cgpu_instance_free_surface , SimNode_ExtFuncCall >(lib,"cgpu_instance_free_surface","cgpu_instance_free_surface")
		->args({"_this","surface"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2311:35
	makeExtern< const CGPUAdapterDetail * (*)(const CGPUAdapter *const) , cgpu_adapter_query_adapter_detail , SimNode_ExtFuncCall >(lib,"cgpu_adapter_query_adapter_detail","cgpu_adapter_query_adapter_detail")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2312:19
	makeExtern< unsigned int (*)(const CGPUAdapter *const,ECGPUQueueType) , cgpu_adapter_query_queue_count , SimNode_ExtFuncCall >(lib,"cgpu_adapter_query_queue_count","cgpu_adapter_query_queue_count")
		->args({"_this","type"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2313:23
	makeExtern< const CGPUDevice * (*)(const CGPUAdapter *,const CGPUDeviceDescriptor *) , cgpu_adapter_create_device , SimNode_ExtFuncCall >(lib,"cgpu_adapter_create_device","cgpu_adapter_create_device")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2314:15
	makeExtern< void (*)(const CGPUAdapter *,const CGPUDevice *) , cgpu_adapter_free_device , SimNode_ExtFuncCall >(lib,"cgpu_adapter_free_device","cgpu_adapter_free_device")
		->args({"_this","device"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2315:15
	makeExtern< void (*)(const CGPUDevice *,unsigned long long *,unsigned long long *) , cgpu_device_query_video_memory_info , SimNode_ExtFuncCall >(lib,"cgpu_device_query_video_memory_info","cgpu_device_query_video_memory_info")
		->args({"_this","total","used"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2316:15
	makeExtern< void (*)(const CGPUDevice *,unsigned long long *,unsigned long long *) , cgpu_device_query_shared_memory_info , SimNode_ExtFuncCall >(lib,"cgpu_device_query_shared_memory_info","cgpu_device_query_shared_memory_info")
		->args({"_this","total","used"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2317:22
	makeExtern< const CGPUFence * (*)(const CGPUDevice *) , cgpu_device_create_fence , SimNode_ExtFuncCall >(lib,"cgpu_device_create_fence","cgpu_device_create_fence")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2318:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUFence *) , cgpu_device_free_fence , SimNode_ExtFuncCall >(lib,"cgpu_device_free_fence","cgpu_device_free_fence")
		->args({"_this","fence"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2319:26
	makeExtern< const CGPUSemaphore * (*)(const CGPUDevice *) , cgpu_device_create_semaphore , SimNode_ExtFuncCall >(lib,"cgpu_device_create_semaphore","cgpu_device_create_semaphore")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2320:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUSemaphore *) , cgpu_device_free_semaphore , SimNode_ExtFuncCall >(lib,"cgpu_device_free_semaphore","cgpu_device_free_semaphore")
		->args({"_this","semaphore"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2321:34
	makeExtern< const CGPURootSignaturePool * (*)(const CGPUDevice *,const CGPURootSignaturePoolDescriptor *) , cgpu_device_create_root_signature_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_create_root_signature_pool","cgpu_device_create_root_signature_pool")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2322:15
	makeExtern< void (*)(const CGPUDevice *,const CGPURootSignaturePool *) , cgpu_device_free_root_signature_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_free_root_signature_pool","cgpu_device_free_root_signature_pool")
		->args({"_this","pool"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2323:30
	makeExtern< const CGPURootSignature * (*)(const CGPUDevice *,const CGPURootSignatureDescriptor *) , cgpu_device_create_root_signature , SimNode_ExtFuncCall >(lib,"cgpu_device_create_root_signature","cgpu_device_create_root_signature")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2324:15
	makeExtern< void (*)(const CGPUDevice *,const CGPURootSignature *) , cgpu_device_free_root_signature , SimNode_ExtFuncCall >(lib,"cgpu_device_free_root_signature","cgpu_device_free_root_signature")
		->args({"_this","signature"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2325:30
	makeExtern< const CGPUDescriptorSet * (*)(const CGPUDevice *,const CGPUDescriptorSetDescriptor *) , cgpu_device_create_descriptor_set , SimNode_ExtFuncCall >(lib,"cgpu_device_create_descriptor_set","cgpu_device_create_descriptor_set")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2326:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUDescriptorSet *) , cgpu_device_free_descriptor_set , SimNode_ExtFuncCall >(lib,"cgpu_device_free_descriptor_set","cgpu_device_free_descriptor_set")
		->args({"_this","set"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2327:32
	makeExtern< const CGPUComputePipeline * (*)(const CGPUDevice *,const CGPUComputePipelineDescriptor *) , cgpu_device_create_compute_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_device_create_compute_pipeline","cgpu_device_create_compute_pipeline")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2328:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUComputePipeline *) , cgpu_device_free_compute_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_device_free_compute_pipeline","cgpu_device_free_compute_pipeline")
		->args({"_this","pipeline"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2329:31
	makeExtern< const CGPURenderPipeline * (*)(const CGPUDevice *,const CGPURenderPipelineDescriptor *) , cgpu_device_create_render_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_device_create_render_pipeline","cgpu_device_create_render_pipeline")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2330:15
	makeExtern< void (*)(const CGPUDevice *,const CGPURenderPipeline *) , cgpu_device_free_render_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_device_free_render_pipeline","cgpu_device_free_render_pipeline")
		->args({"_this","pipeline"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2331:26
	makeExtern< const CGPUQueryPool * (*)(const CGPUDevice *,const CGPUQueryPoolDescriptor *) , cgpu_device_create_query_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_create_query_pool","cgpu_device_create_query_pool")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2332:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUQueryPool *) , cgpu_device_free_query_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_free_query_pool","cgpu_device_free_query_pool")
		->args({"_this","pool"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2333:27
	// makeExtern< const CGPUMemoryPool * (*)(const CGPUDevice *,const CGPUMemoryPoolDescriptor *) , cgpu_device_create_memory_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_create_memory_pool","cgpu_device_create_memory_pool")
	// 	->args({"_this","desc"})
	// 	->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2334:15
	// makeExtern< void (*)(const CGPUDevice *,const CGPUMemoryPool *) , cgpu_device_free_memory_pool , SimNode_ExtFuncCall >(lib,"cgpu_device_free_memory_pool","cgpu_device_free_memory_pool")
	// 	->args({"_this","pool"})
	// 	->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2335:22
	makeExtern< const CGPUQueue * (*)(const CGPUDevice *,ECGPUQueueType,unsigned int) , cgpu_device_get_queue , SimNode_ExtFuncCall >(lib,"cgpu_device_get_queue","cgpu_device_get_queue")
		->args({"_this","type","index"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2336:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUQueue *) , cgpu_device_free_queue , SimNode_ExtFuncCall >(lib,"cgpu_device_free_queue","cgpu_device_free_queue")
		->args({"_this","queue"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2337:27
	makeExtern< const CGPURenderPass * (*)(const CGPUDevice *,const CGPURenderPassDescriptor *) , cgpu_device_create_render_pass , SimNode_ExtFuncCall >(lib,"cgpu_device_create_render_pass","cgpu_device_create_render_pass")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2338:28
	makeExtern< const CGPUFramebuffer * (*)(const CGPUDevice *,const CGPUFramebufferDescriptor *) , cgpu_device_create_framebuffer , SimNode_ExtFuncCall >(lib,"cgpu_device_create_framebuffer","cgpu_device_create_framebuffer")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2339:15
	makeExtern< void (*)(const CGPUDevice *,const CGPURenderPass *) , cgpu_device_free_render_pass , SimNode_ExtFuncCall >(lib,"cgpu_device_free_render_pass","cgpu_device_free_render_pass")
		->args({"_this","render_pass"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2340:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUFramebuffer *) , cgpu_device_free_framebuffer , SimNode_ExtFuncCall >(lib,"cgpu_device_free_framebuffer","cgpu_device_free_framebuffer")
		->args({"_this","framebuffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2341:30
	makeExtern< const CGPUShaderLibrary * (*)(const CGPUDevice *,const CGPUShaderLibraryDescriptor *) , cgpu_device_create_shader_library , SimNode_ExtFuncCall >(lib,"cgpu_device_create_shader_library","cgpu_device_create_shader_library")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2342:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUShaderLibrary *) , cgpu_device_free_shader_library , SimNode_ExtFuncCall >(lib,"cgpu_device_free_shader_library","cgpu_device_free_shader_library")
		->args({"_this","library"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2343:23
	makeExtern< const CGPUBuffer * (*)(const CGPUDevice *,const CGPUBufferDescriptor *) , cgpu_device_create_buffer , SimNode_ExtFuncCall >(lib,"cgpu_device_create_buffer","cgpu_device_create_buffer")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2344:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUBuffer *) , cgpu_device_free_buffer , SimNode_ExtFuncCall >(lib,"cgpu_device_free_buffer","cgpu_device_free_buffer")
		->args({"_this","buffer"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2345:24
	makeExtern< const CGPUSampler * (*)(const CGPUDevice *,const CGPUSamplerDescriptor *) , cgpu_device_create_sampler , SimNode_ExtFuncCall >(lib,"cgpu_device_create_sampler","cgpu_device_create_sampler")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2346:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUSampler *) , cgpu_device_free_sampler , SimNode_ExtFuncCall >(lib,"cgpu_device_free_sampler","cgpu_device_free_sampler")
		->args({"_this","sampler"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2347:24
	makeExtern< const CGPUTexture * (*)(const CGPUDevice *,const CGPUTextureDescriptor *) , cgpu_device_create_texture , SimNode_ExtFuncCall >(lib,"cgpu_device_create_texture","cgpu_device_create_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2348:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUTexture *) , cgpu_device_free_texture , SimNode_ExtFuncCall >(lib,"cgpu_device_free_texture","cgpu_device_free_texture")
		->args({"_this","texture"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2349:28
	makeExtern< const CGPUTextureView * (*)(const CGPUDevice *,const CGPUTextureViewDescriptor *) , cgpu_device_create_texture_view , SimNode_ExtFuncCall >(lib,"cgpu_device_create_texture_view","cgpu_device_create_texture_view")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2350:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUTextureView *) , cgpu_device_free_texture_view , SimNode_ExtFuncCall >(lib,"cgpu_device_free_texture_view","cgpu_device_free_texture_view")
		->args({"_this","render_target"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2351:15
	makeExtern< bool (*)(const CGPUDevice *,const CGPUTextureAliasingBindDescriptor *) , cgpu_device_try_bind_aliasing_texture , SimNode_ExtFuncCall >(lib,"cgpu_device_try_bind_aliasing_texture","cgpu_device_try_bind_aliasing_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2352:19
	makeExtern< uint64_t (*)(const CGPUDevice *,const CGPUExportTextureDescriptor *) , cgpu_device_export_shared_texture_handle , SimNode_ExtFuncCall >(lib,"cgpu_device_export_shared_texture_handle","cgpu_device_export_shared_texture_handle")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2353:24
	makeExtern< const CGPUTexture * (*)(const CGPUDevice *,const CGPUImportTextureDescriptor *) , cgpu_device_import_shared_texture_handle , SimNode_ExtFuncCall >(lib,"cgpu_device_import_shared_texture_handle","cgpu_device_import_shared_texture_handle")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2354:26
	makeExtern< const CGPUSwapChain * (*)(const CGPUDevice *,const CGPUSwapChainDescriptor *) , cgpu_device_create_swap_chain , SimNode_ExtFuncCall >(lib,"cgpu_device_create_swap_chain","cgpu_device_create_swap_chain")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2355:15
	makeExtern< void (*)(const CGPUDevice *,const CGPUSwapChain *) , cgpu_device_free_swap_chain , SimNode_ExtFuncCall >(lib,"cgpu_device_free_swap_chain","cgpu_device_free_swap_chain")
		->args({"_this","swapchain"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2356:15
	makeExtern< void (*)(unsigned int,const CGPUFence *const *) , cgpu_wait_fences , SimNode_ExtFuncCall >(lib,"cgpu_wait_fences","cgpu_wait_fences")
		->args({"fence_count","p_fences"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2357:15
	makeExtern< void (*)(unsigned int,const CGPUFence *const *) , cgpu_reset_fences , SimNode_ExtFuncCall >(lib,"cgpu_reset_fences","cgpu_reset_fences")
		->args({"fence_count","p_fences"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2358:27
	makeExtern< ECGPUFenceStatus (*)(const CGPUFence *) , cgpu_fence_query_status , SimNode_ExtFuncCall >(lib,"cgpu_fence_query_status","cgpu_fence_query_status")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2359:27
	makeExtern< ECGPUSubmitError (*)(const CGPUQueue *,const CGPUQueueSubmitDescriptor *) , cgpu_queue_submit , SimNode_ExtFuncCall >(lib,"cgpu_queue_submit","cgpu_queue_submit")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2360:28
	makeExtern< ECGPUPresentError (*)(const CGPUQueue *,const CGPUQueuePresentDescriptor *) , cgpu_queue_present , SimNode_ExtFuncCall >(lib,"cgpu_queue_present","cgpu_queue_present")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2361:15
	makeExtern< void (*)(const CGPUQueue *) , cgpu_queue_wait_idle , SimNode_ExtFuncCall >(lib,"cgpu_queue_wait_idle","cgpu_queue_wait_idle")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2362:16
	makeExtern< float (*)(const CGPUQueue *) , cgpu_queue_get_timestamp_period_ns , SimNode_ExtFuncCall >(lib,"cgpu_queue_get_timestamp_period_ns","cgpu_queue_get_timestamp_period_ns")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2363:15
	makeExtern< void (*)(const CGPUQueue *,const CGPUTiledTextureRegions *) , cgpu_queue_map_tiled_texture , SimNode_ExtFuncCall >(lib,"cgpu_queue_map_tiled_texture","cgpu_queue_map_tiled_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2364:15
	makeExtern< void (*)(const CGPUQueue *,const CGPUTiledTextureRegions *) , cgpu_queue_unmap_tiled_texture , SimNode_ExtFuncCall >(lib,"cgpu_queue_unmap_tiled_texture","cgpu_queue_unmap_tiled_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2365:15
	makeExtern< void (*)(const CGPUQueue *,const CGPUTiledTexturePackedMips *) , cgpu_queue_map_packed_mips , SimNode_ExtFuncCall >(lib,"cgpu_queue_map_packed_mips","cgpu_queue_map_packed_mips")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2366:15
	makeExtern< void (*)(const CGPUQueue *,const CGPUTiledTexturePackedMips *) , cgpu_queue_unmap_packed_mips , SimNode_ExtFuncCall >(lib,"cgpu_queue_unmap_packed_mips","cgpu_queue_unmap_packed_mips")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2367:28
	makeExtern< const CGPUCommandPool * (*)(const CGPUQueue *,const CGPUCommandPoolDescriptor *) , cgpu_queue_create_command_pool , SimNode_ExtFuncCall >(lib,"cgpu_queue_create_command_pool","cgpu_queue_create_command_pool")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2368:15
	makeExtern< void (*)(const CGPUQueue *,const CGPUCommandPool *) , cgpu_queue_free_command_pool , SimNode_ExtFuncCall >(lib,"cgpu_queue_free_command_pool","cgpu_queue_free_command_pool")
		->args({"_this","pool"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2369:15
	makeExtern< void (*)(const CGPUDescriptorSet *,unsigned int,const CGPUDescriptorData *) , cgpu_descriptor_set_update , SimNode_ExtFuncCall >(lib,"cgpu_descriptor_set_update","cgpu_descriptor_set_update")
		->args({"_this","data_count","p_datas"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2370:30
	makeExtern< const CGPUCommandBuffer * (*)(const CGPUCommandPool *,const CGPUCommandBufferDescriptor *) , cgpu_command_pool_create_command_buffer , SimNode_ExtFuncCall >(lib,"cgpu_command_pool_create_command_buffer","cgpu_command_pool_create_command_buffer")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2371:15
	makeExtern< void (*)(const CGPUCommandPool *) , cgpu_command_pool_reset , SimNode_ExtFuncCall >(lib,"cgpu_command_pool_reset","cgpu_command_pool_reset")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2372:15
	makeExtern< void (*)(const CGPUCommandPool *,const CGPUCommandBuffer *) , cgpu_command_pool_free_command_buffer , SimNode_ExtFuncCall >(lib,"cgpu_command_pool_free_command_buffer","cgpu_command_pool_free_command_buffer")
		->args({"_this","cmd"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2373:15
	makeExtern< void (*)(const CGPUBuffer *,const CGPUBufferRange *) , cgpu_buffer_map , SimNode_ExtFuncCall >(lib,"cgpu_buffer_map","cgpu_buffer_map")
		->args({"_this","range"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2374:15
	makeExtern< void (*)(const CGPUBuffer *) , cgpu_buffer_unmap , SimNode_ExtFuncCall >(lib,"cgpu_buffer_unmap","cgpu_buffer_unmap")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2375:37
	makeExtern< ECGPUAcquireNextImageError (*)(const CGPUSwapChain *,const CGPUAcquireNextDescriptor *,unsigned int *) , cgpu_swap_chain_acquire_next_image , SimNode_ExtFuncCall >(lib,"cgpu_swap_chain_acquire_next_image","cgpu_swap_chain_acquire_next_image")
		->args({"_this","desc","p_image_index"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2376:15
	makeExtern< void (*)(const CGPUCommandBuffer *) , cgpu_command_buffer_begin , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_begin","cgpu_command_buffer_begin")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2377:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUBufferToBufferTransfer *) , cgpu_command_buffer_transfer_buffer_to_buffer , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_transfer_buffer_to_buffer","cgpu_command_buffer_transfer_buffer_to_buffer")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2378:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUTextureToTextureTransfer *) , cgpu_command_buffer_transfer_texture_to_texture , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_transfer_texture_to_texture","cgpu_command_buffer_transfer_texture_to_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2379:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUBufferToTextureTransfer *) , cgpu_command_buffer_transfer_buffer_to_texture , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_transfer_buffer_to_texture","cgpu_command_buffer_transfer_buffer_to_texture")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2380:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUBufferToTilesTransfer *) , cgpu_command_buffer_transfer_buffer_to_tiles , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_transfer_buffer_to_tiles","cgpu_command_buffer_transfer_buffer_to_tiles")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2381:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUResourceBarrierDescriptor *) , cgpu_command_buffer_resource_barrier , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_resource_barrier","cgpu_command_buffer_resource_barrier")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2382:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUQueryPool *,const CGPUQueryDescriptor *) , cgpu_command_buffer_begin_query , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_begin_query","cgpu_command_buffer_begin_query")
		->args({"_this","pool","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2383:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUQueryPool *,const CGPUQueryDescriptor *) , cgpu_command_buffer_end_query , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_end_query","cgpu_command_buffer_end_query")
		->args({"_this","pool","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2384:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUQueryPool *,unsigned int,unsigned int) , cgpu_command_buffer_reset_query_pool , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_reset_query_pool","cgpu_command_buffer_reset_query_pool")
		->args({"_this","pool","start_query","query_count"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2385:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUQueryPool *,const CGPUBuffer *,unsigned int,unsigned int) , cgpu_command_buffer_resolve_query , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_resolve_query","cgpu_command_buffer_resolve_query")
		->args({"_this","pool","readback","start_query","query_count"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2386:15
	makeExtern< void (*)(const CGPUCommandBuffer *) , cgpu_command_buffer_end , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_end","cgpu_command_buffer_end")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2387:35
	makeExtern< const CGPUComputePassEncoder * (*)(const CGPUCommandBuffer *,const CGPUComputePassDescriptor *) , cgpu_command_buffer_begin_compute_pass , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_begin_compute_pass","cgpu_command_buffer_begin_compute_pass")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2388:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUComputePassEncoder *) , cgpu_command_buffer_end_compute_pass , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_end_compute_pass","cgpu_command_buffer_end_compute_pass")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2389:34
	makeExtern< const CGPURenderPassEncoder * (*)(const CGPUCommandBuffer *,const CGPUBeginRenderPassInfo *) , cgpu_command_buffer_begin_render_pass , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_begin_render_pass","cgpu_command_buffer_begin_render_pass")
		->args({"_this","begin_info"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2390:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPURenderPassEncoder *) , cgpu_command_buffer_end_render_pass , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_end_render_pass","cgpu_command_buffer_end_render_pass")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2391:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUEventInfo *) , cgpu_command_buffer_begin_event , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_begin_event","cgpu_command_buffer_begin_event")
		->args({"_this","event"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2392:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUMarkerInfo *) , cgpu_command_buffer_set_marker , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_set_marker","cgpu_command_buffer_set_marker")
		->args({"_this","marker"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2393:15
	makeExtern< void (*)(const CGPUCommandBuffer *) , cgpu_command_buffer_end_event , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_end_event","cgpu_command_buffer_end_event")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2394:28
	makeExtern< const CGPUStateBuffer * (*)(const CGPUCommandBuffer *,const CGPUStateBufferDescriptor *) , cgpu_command_buffer_create_state_buffer , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_create_state_buffer","cgpu_command_buffer_create_state_buffer")
		->args({"_this","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2395:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUStateBuffer *) , cgpu_command_buffer_free_state_buffer , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_free_state_buffer","cgpu_command_buffer_free_state_buffer")
		->args({"_this","stream"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2396:23
	makeExtern< const CGPUBinder * (*)(const CGPUCommandBuffer *) , cgpu_command_buffer_create_binder , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_create_binder","cgpu_command_buffer_create_binder")
		->args({"_this"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2397:15
	makeExtern< void (*)(const CGPUCommandBuffer *,const CGPUBinder *) , cgpu_command_buffer_free_binder , SimNode_ExtFuncCall >(lib,"cgpu_command_buffer_free_binder","cgpu_command_buffer_free_binder")
		->args({"_this","binder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2398:15
	makeExtern< void (*)(const CGPUComputePassEncoder *,const CGPUDescriptorSet *) , cgpu_compute_pass_encoder_bind_descriptor_set , SimNode_ExtFuncCall >(lib,"cgpu_compute_pass_encoder_bind_descriptor_set","cgpu_compute_pass_encoder_bind_descriptor_set")
		->args({"_this","set"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2399:15
	makeExtern< void (*)(const CGPUComputePassEncoder *,const CGPUComputePipeline *) , cgpu_compute_pass_encoder_bind_compute_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_compute_pass_encoder_bind_compute_pipeline","cgpu_compute_pass_encoder_bind_compute_pipeline")
		->args({"_this","pipeline"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2400:15
	makeExtern< void (*)(const CGPUComputePassEncoder *,unsigned int,unsigned int,unsigned int) , cgpu_compute_pass_encoder_dispatch , SimNode_ExtFuncCall >(lib,"cgpu_compute_pass_encoder_dispatch","cgpu_compute_pass_encoder_dispatch")
		->args({"_this","x","y","z"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2401:15
	makeExtern< void (*)(const CGPUComputePassEncoder *,const CGPURootSignature *,const char *,const void *) , cgpu_compute_pass_encoder_push_constants , SimNode_ExtFuncCall >(lib,"cgpu_compute_pass_encoder_push_constants","cgpu_compute_pass_encoder_push_constants")
		->args({"_this","rs","name","data"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2402:15
	makeExtern< void (*)(const CGPUComputePassEncoder *,const CGPUStateBuffer *) , cgpu_compute_pass_encoder_bind_state_buffer , SimNode_ExtFuncCall >(lib,"cgpu_compute_pass_encoder_bind_state_buffer","cgpu_compute_pass_encoder_bind_state_buffer")
		->args({"_this","stream"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2403:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,ECGPUShadingRate,ECGPUShadingRateCombiner,ECGPUShadingRateCombiner) , cgpu_render_pass_encoder_set_shading_rate , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_set_shading_rate","cgpu_render_pass_encoder_set_shading_rate")
		->args({"_this","shading_rate","post_rasterize_rate","final_rate"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2404:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,const CGPUDescriptorSet *) , cgpu_render_pass_encoder_bind_descriptor_set , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_bind_descriptor_set","cgpu_render_pass_encoder_bind_descriptor_set")
		->args({"_this","set"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2405:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,float,float,float,float,float,float) , cgpu_render_pass_encoder_set_viewport , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_set_viewport","cgpu_render_pass_encoder_set_viewport")
		->args({"_this","x","y","width","height","min_depth","max_depth"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2406:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,unsigned int,unsigned int,unsigned int) , cgpu_render_pass_encoder_set_scissor , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_set_scissor","cgpu_render_pass_encoder_set_scissor")
		->args({"_this","x","y","width","height"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2407:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,const CGPURenderPipeline *) , cgpu_render_pass_encoder_bind_render_pipeline , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_bind_render_pipeline","cgpu_render_pass_encoder_bind_render_pipeline")
		->args({"_this","pipeline"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2408:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,const CGPUBuffer *const *,const unsigned int *,const unsigned int *) , cgpu_render_pass_encoder_bind_vertex_buffers , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_bind_vertex_buffers","cgpu_render_pass_encoder_bind_vertex_buffers")
		->args({"_this","buffer_count","p_buffers","p_strides","p_offsets"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2409:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,const CGPUBuffer *,unsigned int,uint64_t) , cgpu_render_pass_encoder_bind_index_buffer , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_bind_index_buffer","cgpu_render_pass_encoder_bind_index_buffer")
		->args({"_this","buffer","index_stride","offset"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2410:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,const CGPURootSignature *,const char *,const void *) , cgpu_render_pass_encoder_push_constants , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_push_constants","cgpu_render_pass_encoder_push_constants")
		->args({"_this","rs","name","data"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2411:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,unsigned int) , cgpu_render_pass_encoder_draw , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_draw","cgpu_render_pass_encoder_draw")
		->args({"_this","vertex_count","first_vertex"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2412:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,unsigned int,unsigned int,unsigned int) , cgpu_render_pass_encoder_draw_instanced , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_draw_instanced","cgpu_render_pass_encoder_draw_instanced")
		->args({"_this","vertex_count","first_vertex","instance_count","first_instance"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2413:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,unsigned int,unsigned int) , cgpu_render_pass_encoder_draw_indexed , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_draw_indexed","cgpu_render_pass_encoder_draw_indexed")
		->args({"_this","index_count","first_index","first_vertex"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2414:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,unsigned int,unsigned int,unsigned int,unsigned int,unsigned int) , cgpu_render_pass_encoder_draw_indexed_instanced , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_draw_indexed_instanced","cgpu_render_pass_encoder_draw_indexed_instanced")
		->args({"_this","index_count","first_index","instance_count","first_instance","first_vertex"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2415:15
	makeExtern< void (*)(const CGPURenderPassEncoder *,const CGPUStateBuffer *) , cgpu_render_pass_encoder_bind_state_buffer , SimNode_ExtFuncCall >(lib,"cgpu_render_pass_encoder_bind_state_buffer","cgpu_render_pass_encoder_bind_state_buffer")
		->args({"_this","stream"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2416:29
	makeExtern< const CGPULinkedShader * (*)(const CGPURootSignature *,unsigned int,const CGPUCompiledShaderDescriptor *) , cgpu_root_signature_compile_and_link_shaders , SimNode_ExtFuncCall >(lib,"cgpu_root_signature_compile_and_link_shaders","cgpu_root_signature_compile_and_link_shaders")
		->args({"_this","count","desc"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2417:15
	makeExtern< void (*)(const CGPURootSignature *,unsigned int,const CGPUCompiledShaderDescriptor *,const CGPUCompiledShader **) , cgpu_root_signature_compile_shaders , SimNode_ExtFuncCall >(lib,"cgpu_root_signature_compile_shaders","cgpu_root_signature_compile_shaders")
		->args({"_this","count","desc","out_isas"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2418:15
	makeExtern< void (*)(const CGPURootSignature *,const CGPUCompiledShader *) , cgpu_root_signature_free_compiled_shader , SimNode_ExtFuncCall >(lib,"cgpu_root_signature_free_compiled_shader","cgpu_root_signature_free_compiled_shader")
		->args({"_this","shader"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2419:15
	makeExtern< void (*)(const CGPURootSignature *,const CGPULinkedShader *) , cgpu_root_signature_free_linked_shader , SimNode_ExtFuncCall >(lib,"cgpu_root_signature_free_linked_shader","cgpu_root_signature_free_linked_shader")
		->args({"_this","shader"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2420:35
	makeExtern< const CGPURasterStateEncoder * (*)(const CGPUStateBuffer *,const CGPURenderPassEncoder *) , cgpu_state_buffer_open_raster_state_encoder , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_open_raster_state_encoder","cgpu_state_buffer_open_raster_state_encoder")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2421:15
	makeExtern< void (*)(const CGPUStateBuffer *,const CGPURasterStateEncoder *) , cgpu_state_buffer_close_raster_state_encoder , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_close_raster_state_encoder","cgpu_state_buffer_close_raster_state_encoder")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2422:35
	makeExtern< const CGPUShaderStateEncoder * (*)(const CGPUStateBuffer *,const CGPURenderPassEncoder *) , cgpu_state_buffer_open_shader_state_encoder_r , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_open_shader_state_encoder_r","cgpu_state_buffer_open_shader_state_encoder_r")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2423:35
	makeExtern< const CGPUShaderStateEncoder * (*)(const CGPUStateBuffer *,const CGPUComputePassEncoder *) , cgpu_state_buffer_open_shader_state_encoder_c , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_open_shader_state_encoder_c","cgpu_state_buffer_open_shader_state_encoder_c")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2424:15
	makeExtern< void (*)(const CGPUStateBuffer *,const CGPUShaderStateEncoder *) , cgpu_state_buffer_close_shader_state_encoder , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_close_shader_state_encoder","cgpu_state_buffer_close_shader_state_encoder")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2425:33
	makeExtern< const CGPUUserStateEncoder * (*)(const CGPUStateBuffer *,const CGPURenderPassEncoder *) , cgpu_state_buffer_open_user_state_encoder , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_open_user_state_encoder","cgpu_state_buffer_open_user_state_encoder")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2426:15
	makeExtern< void (*)(const CGPUStateBuffer *,const CGPUUserStateEncoder *) , cgpu_state_buffer_close_user_state_encoder , SimNode_ExtFuncCall >(lib,"cgpu_state_buffer_close_user_state_encoder","cgpu_state_buffer_close_user_state_encoder")
		->args({"_this","encoder"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2427:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,float,float,float,float,float,float) , cgpu_raster_state_encoder_set_viewport , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_viewport","cgpu_raster_state_encoder_set_viewport")
		->args({"_this","x","y","width","height","min_depth","max_depth"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2428:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,unsigned int,unsigned int,unsigned int,unsigned int) , cgpu_raster_state_encoder_set_scissor , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_scissor","cgpu_raster_state_encoder_set_scissor")
		->args({"_this","x","y","width","height"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2429:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,unsigned int) , cgpu_raster_state_encoder_set_cull_mode , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_cull_mode","cgpu_raster_state_encoder_set_cull_mode")
		->args({"_this","cull_mode"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2430:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,ECGPUFrontFace) , cgpu_raster_state_encoder_set_front_face , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_front_face","cgpu_raster_state_encoder_set_front_face")
		->args({"_this","front_face"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2431:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,ECGPUPrimitiveTopology) , cgpu_raster_state_encoder_set_primitive_topology , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_primitive_topology","cgpu_raster_state_encoder_set_primitive_topology")
		->args({"_this","topology"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2432:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,bool) , cgpu_raster_state_encoder_set_depth_test_enabled , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_depth_test_enabled","cgpu_raster_state_encoder_set_depth_test_enabled")
		->args({"_this","enabled"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2433:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,bool) , cgpu_raster_state_encoder_set_depth_write_enabled , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_depth_write_enabled","cgpu_raster_state_encoder_set_depth_write_enabled")
		->args({"_this","enabled"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2434:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,ECGPUCompareOp) , cgpu_raster_state_encoder_set_depth_compare_op , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_depth_compare_op","cgpu_raster_state_encoder_set_depth_compare_op")
		->args({"_this","compare_op"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2435:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,bool) , cgpu_raster_state_encoder_set_stencil_test_enabled , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_stencil_test_enabled","cgpu_raster_state_encoder_set_stencil_test_enabled")
		->args({"_this","enabled"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2436:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,unsigned int,ECGPUStencilOp,ECGPUStencilOp,ECGPUStencilOp,ECGPUCompareOp) , cgpu_raster_state_encoder_set_stencil_compare_op , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_stencil_compare_op","cgpu_raster_state_encoder_set_stencil_compare_op")
		->args({"_this","faces","fail_op","pass_op","depth_fail_op","compare_op"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2437:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,ECGPUFillMode) , cgpu_raster_state_encoder_set_fill_mode , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_fill_mode","cgpu_raster_state_encoder_set_fill_mode")
		->args({"_this","fill_mode"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2438:15
	makeExtern< void (*)(const CGPURasterStateEncoder *,unsigned int) , cgpu_raster_state_encoder_set_sample_count , SimNode_ExtFuncCall >(lib,"cgpu_raster_state_encoder_set_sample_count","cgpu_raster_state_encoder_set_sample_count")
		->args({"_this","sample_count"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2439:15
	makeExtern< void (*)(const CGPUShaderStateEncoder *,unsigned int,const unsigned int *,const CGPUCompiledShader *const *) , cgpu_shader_state_encoder_bind_shaders , SimNode_ExtFuncCall >(lib,"cgpu_shader_state_encoder_bind_shaders","cgpu_shader_state_encoder_bind_shaders")
		->args({"_this","stage_count","p_stages","shaders"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2440:15
	makeExtern< void (*)(const CGPUShaderStateEncoder *,const CGPULinkedShader *) , cgpu_shader_state_encoder_bind_linked_shader , SimNode_ExtFuncCall >(lib,"cgpu_shader_state_encoder_bind_linked_shader","cgpu_shader_state_encoder_bind_linked_shader")
		->args({"_this","linked"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2441:15
	makeExtern< void (*)(const CGPUBinder *,const CGPUVertexLayout *) , cgpu_binder_bind_vertex_layout , SimNode_ExtFuncCall >(lib,"cgpu_binder_bind_vertex_layout","cgpu_binder_bind_vertex_layout")
		->args({"_this","layout"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2442:15
	makeExtern< void (*)(const CGPUBinder *,unsigned int,unsigned int,const CGPUBuffer *const *,const unsigned long long *,const unsigned long long *,const unsigned long long *) , cgpu_binder_bind_vertex_buffer , SimNode_ExtFuncCall >(lib,"cgpu_binder_bind_vertex_buffer","cgpu_binder_bind_vertex_buffer")
		->args({"_this","first_binding","binding_count","p_buffers","p_offsets","p_sizes","p_strides"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2444:30
	makeExtern< bool (*)(const ECGPUTextureFormat) , FormatUtil_IsDepthStencilFormat , SimNode_ExtFuncCall >(lib,"FormatUtil_IsDepthStencilFormat","FormatUtil_IsDepthStencilFormat")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2458:30
	makeExtern< bool (*)(const ECGPUTextureFormat) , FormatUtil_IsDepthOnlyFormat , SimNode_ExtFuncCall >(lib,"FormatUtil_IsDepthOnlyFormat","FormatUtil_IsDepthOnlyFormat")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2467:30
	makeExtern< bool (*)(const ECGPUTextureFormat) , FormatUtil_ContainDepth , SimNode_ExtFuncCall >(lib,"FormatUtil_ContainDepth","FormatUtil_ContainDepth")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2480:30
	makeExtern< bool (*)(const ECGPUTextureFormat) , FormatUtil_ContainStencil , SimNode_ExtFuncCall >(lib,"FormatUtil_ContainStencil","FormatUtil_ContainStencil")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2491:34
	makeExtern< unsigned int (*)(const ECGPUTextureFormat) , FormatUtil_BitSizeOfBlock , SimNode_ExtFuncCall >(lib,"FormatUtil_BitSizeOfBlock","FormatUtil_BitSizeOfBlock")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2634:34
	makeExtern< unsigned int (*)(const ECGPUTextureFormat) , FormatUtil_WidthOfBlock , SimNode_ExtFuncCall >(lib,"FormatUtil_WidthOfBlock","FormatUtil_WidthOfBlock")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2718:34
	makeExtern< unsigned int (*)(const ECGPUTextureFormat) , FormatUtil_HeightOfBlock , SimNode_ExtFuncCall >(lib,"FormatUtil_HeightOfBlock","FormatUtil_HeightOfBlock")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
// from G:/Users/Administrator/Downloads/DAS/learn/bind/cgpu/api.h:2802:34
	makeExtern< unsigned int (*)(const ECGPUVertexFormat) , VertexFormatUtil_BitSizeOfBlock , SimNode_ExtFuncCall >(lib,"VertexFormatUtil_BitSizeOfBlock","VertexFormatUtil_BitSizeOfBlock")
		->args({"arg"})
		->addToModule(*this, SideEffects::worstDefault);
}
}

