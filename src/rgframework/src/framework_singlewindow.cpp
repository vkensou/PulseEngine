#include "framework.h"

#include <SDL3/SDL.h>
#include "cgpu/api.h"
#include "rendergraph.h"
#include "rendergraph_compiler.h"
#include "rendergraph_executor.h"
#include "imgui_impl_sdl3.h"
#include <string.h>
#include "cgpu_device.h"
#include "mimalloc.h"
#include <thread>
#include <chrono>
#ifdef __ANDROID__
#include "jni.h"
#endif
#include <SDL3/SDL_main.h>

void oval_log(void* user_data, ECGPULogSeverity severity, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	SDL_LogPriority priority = SDL_LOG_PRIORITY_VERBOSE;
	if (severity == CGPU_LOG_SEVERITY_TRACE)
		priority = SDL_LOG_PRIORITY_VERBOSE;
	else if (severity == CGPU_LOG_SEVERITY_DEBUG)
		priority = SDL_LOG_PRIORITY_DEBUG;
	else if (severity == CGPU_LOG_SEVERITY_INFO)
		priority = SDL_LOG_PRIORITY_INFO;
	else if (severity == CGPU_LOG_SEVERITY_WARNING)
		priority = SDL_LOG_PRIORITY_WARN;
	else if (severity == CGPU_LOG_SEVERITY_ERROR)
		priority = SDL_LOG_PRIORITY_ERROR;
	else if (severity == CGPU_LOG_SEVERITY_FATAL)
		priority = SDL_LOG_PRIORITY_CRITICAL;
	SDL_LogMessageV(SDL_LOG_CATEGORY_RENDER, priority, fmt, args);
	va_end(args);
}

void* oval_malloc(void* user_data, size_t size, const void* pool)
{
	return mi_malloc(size);
}

void* oval_realloc(void* user_data, void* ptr, size_t size, const void* pool)
{
	return mi_realloc(ptr, size);
}

void* oval_calloc(void* user_data, size_t count, size_t size, const void* pool)
{
	return mi_calloc(count, size);
}

void oval_free(void* user_data, void* ptr, const void* pool)
{
	mi_free(ptr);
}

void* oval_malloc_aligned(void* user_data, size_t size, size_t alignment, const void* pool)
{
	return mi_malloc_aligned(size, alignment);
}

void* oval_realloc_aligned(void* user_data, void* ptr, size_t size, size_t alignment, const void* pool)
{
	return mi_realloc_aligned(ptr, size, alignment);
}

void* oval_calloc_aligned(void* user_data, size_t count, size_t size, size_t alignment, const void* pool)
{
	return mi_calloc_aligned(count, size, alignment);
}

void oval_free_aligned(void* user_data, void* ptr, const void* pool)
{
	mi_free_aligned(ptr, 1);
}

oval_device_t* oval_create_device(const oval_device_descriptor* device_descriptor)
{
	SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
	
	if (!SDL_Init(0))
		return nullptr;

	oval_device_descriptor descriptor = *device_descriptor;
	if (descriptor.fixed_update_time_step <= 0)
		descriptor.fixed_update_time_step = 1.0 / 30.0;

	oval_device_t super = { .descriptor = descriptor };
	super.width = device_descriptor->width;
	super.height = device_descriptor->height;

	auto memory_resource = new std::pmr::unsynchronized_pool_resource();
	auto device_cgpu = new oval_cgpu_device_t(super, memory_resource);

	if (device_descriptor->enable_capture)
	{
		auto renderdoc_path = locate_renderdoc();
		if (load_renderdoc(renderdoc_path))
			device_cgpu->rdc = GetRenderDocApi();
	}

	bool validation = device_descriptor->enable_gpu_validation;
#ifdef __ANDROID__
	validation = false;
#endif
	CGPUInstanceDescriptor instance_desc = {
		.backend = CGPU_BACKEND_VULKAN,
		.enable_debug_layer = validation,
		.enable_gpu_based_validation = validation,
		.enable_set_name = validation,
		.logger = {
			.log_callback = oval_log
		},
		.allocator = {
			.malloc_fn = oval_malloc,
			.realloc_fn = oval_realloc,
			.calloc_fn = oval_calloc,
			.free_fn = oval_free,
			.malloc_aligned_fn = oval_malloc_aligned,
			.realloc_aligned_fn = oval_realloc_aligned,
			.calloc_aligned_fn = oval_calloc_aligned,
			.free_aligned_fn = oval_free_aligned,
			.user_data = nullptr,
		},
	};

	device_cgpu->instance = cgpu_create_instance(&instance_desc);

	uint32_t adapters_count = 0;
	cgpu_instance_enum_adapters(device_cgpu->instance, &adapters_count, CGPU_NULLPTR);
	CGPUAdapterId* adapters = (CGPUAdapterId*)malloc(sizeof(CGPUAdapterId) * (adapters_count));
	cgpu_instance_enum_adapters(device_cgpu->instance, &adapters_count, adapters);
	auto adapter = adapters[0];

	// Create device
	CGPUQueueGroupDescriptor G = {
		.queue_type = CGPU_QUEUE_TYPE_GRAPHICS,
		.queue_count = 1
	};
	CGPUDeviceDescriptor device_desc = {
		.queue_group_count = 1,
		.p_queue_groups = &G,
	};
	device_cgpu->device = cgpu_adapter_create_device(adapter, &device_desc);
	device_cgpu->gfx_queue = cgpu_device_get_queue(device_cgpu->device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
	device_cgpu->present_queue = device_cgpu->gfx_queue;
	free(adapters);

	{
		const uint64_t width = 4;
		const uint64_t height = 4;
		const uint64_t count = width * height;

		CGPUTextureDescriptor default_texture_desc =
		{
			.name = "default_texture",
			.width = width,
			.height = height,
			.depth = 1,
			.array_size = 1,
			.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM,
			.mip_levels = 1,
			.descriptors = CGPU_RESOURCE_TYPE_TEXTURE,
		};
		uint32_t colors[count];
		std::fill(colors, colors + count, 0xffff00ff);
		device_cgpu->default_texture = oval_create_texture_from_buffer(&device_cgpu->super, default_texture_desc, colors, sizeof(colors));
	}

	for (uint32_t i = 0; i < 3; ++i)
	{
		device_cgpu->frameDatas.emplace_back(device_cgpu->device, device_cgpu->gfx_queue, device_cgpu->super.descriptor.enable_profile, device_cgpu->memory_resource);
		device_cgpu->frameDatas[i].execContext.default_texture = device_cgpu->default_texture->view;
	}

	IMGUI_CHECKVERSION();

	device_cgpu->imgui_font = IM_NEW(ImFontAtlas)();

	oval_window_descriptor window_descriptor = {
		.width = device_descriptor->width,
		.height = device_descriptor->height,
		.resizable = true,
		.use_imgui = true,
		.own_imgui = true,
		.on_imgui = device_descriptor->on_imgui,
	};
	device_cgpu->super.mainwindow_entity = oval_create_window_entity(&device_cgpu->super, &window_descriptor);
	{
		auto& registry = device_cgpu->registry;
		auto& window = registry.get<WindowComponent>(device_cgpu->super.mainwindow_entity);
		device_cgpu->mainwindow = window.handle;
	}

	CGPUBlendAttachmentState blit_blend_attachments = {
		.enable = false,
		.src_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_factor = CGPU_BLEND_FACTOR_ZERO,
		.src_alpha_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_alpha_factor = CGPU_BLEND_FACTOR_ZERO,
		.blend_op = CGPU_BLEND_OP_ADD,
		.blend_alpha_op = CGPU_BLEND_OP_ADD,
		.color_mask = CGPU_COLOR_MASK_RGBA,
	};
	CGPUBlendStateDescriptor blit_blend_desc = {
		.attachment_count = 1,
		.p_attachments = &blit_blend_attachments,
		.alpha_to_coverage = false,
		.independent_blend = false,
	};
	CGPUDepthStateDescriptor depth_desc = {
		.depth_test = false,
		.depth_write = false,
		.stencil_test = false,
	};
	CGPURasterizerStateDescriptor rasterizer_state = {
		.cull_mode = CGPU_CULL_MODE_NONE,
	};
	uint8_t blit_vert_spv[] = {
		#include "blit.vs.spv.h"
	};
	uint8_t blit_frag_spv[] = {
		#include "blit.ps.spv.h"
	};
	device_cgpu->blit_shader = oval_create_shader(&device_cgpu->super, blit_vert_spv, sizeof(blit_vert_spv), blit_frag_spv, sizeof(blit_frag_spv), blit_blend_desc, depth_desc, rasterizer_state);

	CGPUSamplerDescriptor blit_linear_sampler_desc = {
		.min_filter = CGPU_FILTER_TYPE_LINEAR,
		.mag_filter = CGPU_FILTER_TYPE_LINEAR,
		.mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
		.address_u = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
		.address_v = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
		.address_w = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
		.mip_lod_bias = 0,
		.max_anisotropy = 1,
	};
	device_cgpu->blit_linear_sampler = oval_create_sampler(&device_cgpu->super, &blit_linear_sampler_desc);

	CGPUBlendAttachmentState imgui_blend_attachments = {
		.enable = true,
		.src_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
		.dst_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.src_alpha_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
		.dst_alpha_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.blend_op = CGPU_BLEND_OP_ADD,
		.blend_alpha_op = CGPU_BLEND_OP_ADD,
		.color_mask = CGPU_COLOR_MASK_RGBA,
	};
	CGPUBlendStateDescriptor imgui_blend_desc = {
		.attachment_count = 1,
		.p_attachments = &imgui_blend_attachments,
		.alpha_to_coverage = false,
		.independent_blend = false,
	};
	uint8_t imgui_vert_spv[] = {
		#include "imgui.vs.spv.h"
	};
	uint8_t imgui_frag_spv[] = {
		#include "imgui.ps.spv.h"
	};
	device_cgpu->imgui_shader = oval_create_shader(&device_cgpu->super, imgui_vert_spv, sizeof(imgui_vert_spv), imgui_frag_spv, sizeof(imgui_frag_spv), imgui_blend_desc, depth_desc, rasterizer_state);

	{
		unsigned char* fontPixels;
		int fontTexWidth, fontTexHeight;
		device_cgpu->imgui_font->GetTexDataAsRGBA32(&fontPixels, &fontTexWidth, &fontTexHeight);

		uint64_t width = fontTexWidth;
		uint64_t height = fontTexHeight;
		uint64_t count = width * height;

		CGPUTextureDescriptor imgui_font_texture_desc =
		{
			.name = "ImGui Default Font Texture",
			.width = width,
			.height = height,
			.depth = 1,
			.array_size = 1,
			.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM,
			.mip_levels = 1,
			.descriptors = CGPU_RESOURCE_TYPE_TEXTURE,
		};
		device_cgpu->imgui_font_texture = oval_create_texture_from_buffer(&device_cgpu->super, imgui_font_texture_desc, fontPixels, count * 4);
	}

	CGPUSamplerDescriptor imgui_font_sampler_desc = {
		.min_filter = CGPU_FILTER_TYPE_LINEAR,
		.mag_filter = CGPU_FILTER_TYPE_LINEAR,
		.mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
		.address_u = CGPU_ADDRESS_MODE_REPEAT,
		.address_v = CGPU_ADDRESS_MODE_REPEAT,
		.address_w = CGPU_ADDRESS_MODE_REPEAT,
		.mip_lod_bias = 0,
		.max_anisotropy = 1,
	};
	device_cgpu->imgui_font_sampler = oval_create_sampler(&device_cgpu->super, &imgui_font_sampler_desc);

	return (oval_device_t*)device_cgpu;
}

HGEGraphics::Mesh* setupImGuiResourcesMesh(oval_cgpu_device_t* device, HGEGraphics::rendergraph_t& rg, ImDrawData* drawData, HGEGraphics::Mesh* imgui_mesh)
{
	using namespace HGEGraphics;

	if (drawData && drawData->TotalVtxCount > 0)
	{
		size_t vertex_size = drawData->TotalVtxCount * sizeof(ImDrawVert);
		size_t index_size = drawData->TotalIdxCount * sizeof(ImDrawIdx);

		struct PassData
		{
			ImDrawData* drawData;
		};
		PassData* update_vertex_passdata;
		auto imgui_vertex_buffer = declare_dynamic_vertex_buffer(imgui_mesh, &rg, drawData->TotalVtxCount);
		rendergraph_add_uploadbufferpass(&rg, "upload imgui vertex data", imgui_vertex_buffer, [](UploadEncoder* encoder, void* passdata)
			{
				PassData* resolved_passdata = (PassData*)passdata;
				ImDrawData* drawData = resolved_passdata->drawData;
				uint32_t offset = 0;
				for (int n = 0; n < drawData->CmdListsCount; n++)
				{
					const ImDrawList* cmd_list = drawData->CmdLists[n];
					upload(encoder, offset, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data);

					offset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
				}
			}, sizeof(PassData), (void**)&update_vertex_passdata);
		update_vertex_passdata->drawData = drawData;

		PassData* update_index_passdata;
		auto imgui_index_buffer = declare_dynamic_index_buffer(imgui_mesh, &rg, drawData->TotalIdxCount);
		rendergraph_add_uploadbufferpass(&rg, "upload imgui index data", imgui_index_buffer, [](UploadEncoder* encoder, void* passdata)
			{
				PassData* resolved_passdata = (PassData*)passdata;
				ImDrawData* drawData = resolved_passdata->drawData;
				uint32_t offset = 0;
				for (int n = 0; n < drawData->CmdListsCount; n++)
				{
					const ImDrawList* cmd_list = drawData->CmdLists[n];
					upload(encoder, offset, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), cmd_list->IdxBuffer.Data);

					offset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
				}
			}, sizeof(PassData), (void**)&update_index_passdata);
		update_index_passdata->drawData = drawData;
	}
	return imgui_mesh;
}

void setupImGuiResources(oval_cgpu_device_t* device, HGEGraphics::rendergraph_t& rg, const std::pmr::vector<oval_window_impl_t*>& prepared_windows)
{
	for (auto window : prepared_windows)
	{
		window->imgui_draw_data = window->snapshot.DrawData.TotalVtxCount > 0 ? &window->snapshot.DrawData : nullptr;
		auto imgui_mesh = setupImGuiResourcesMesh(device, rg, window->imgui_draw_data, window->imgui_mesh);
	}
}

void renderImgui(oval_cgpu_device_t* device, HGEGraphics::rendergraph_t& rg, HGEGraphics::texture_handle_t rg_back_buffer, oval_window_impl_t* window)
{
	using namespace HGEGraphics;

	if (rendergraph_texture_handle_valid(rg_back_buffer) && window->imgui_draw_data)
	{
		auto passBuilder = rendergraph_add_renderpass(&rg, "Imgui Pass");
		uint32_t color = 0xffffffff;
		renderpass_add_color_attachment(&passBuilder, rg_back_buffer, ECGPULoadAction::CGPU_LOAD_ACTION_LOAD, color, ECGPUStoreAction::CGPU_STORE_ACTION_STORE);
		renderpass_use_buffer(&passBuilder, window->imgui_mesh->vertex_buffer->dynamic_handle);
		renderpass_use_buffer(&passBuilder, window->imgui_mesh->index_buffer->dynamic_handle);

		struct ImguiPassPassData
		{
			oval_cgpu_device_t* device;
			oval_window_impl_t* window;
		};
		ImguiPassPassData* passdata = nullptr;
		renderpass_set_executable(&passBuilder, [](RenderPassEncoder* encoder, void* passdata)
			{
				ImguiPassPassData* resolved_passdata = (ImguiPassPassData*)passdata;
				oval_cgpu_device_t* device = resolved_passdata->device;
				oval_window_impl_t* window = resolved_passdata->window;

				auto drawData = window->imgui_draw_data;
				int fb_width = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
				int fb_height = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
				if (fb_width <= 0 || fb_height <= 0)
					return;

				set_viewport(encoder,
					0.0f, 0.0f,
					(float)fb_width, (float)fb_height,
					0.f, 1.f);

				float scale[2];
				scale[0] = 2.0f / drawData->DisplaySize.x;
				scale[1] = -2.0f / drawData->DisplaySize.y;
				float translate[2];
				translate[0] = -1.0f - drawData->DisplayPos.x * scale[0];
				translate[1] = +1.0f - drawData->DisplayPos.y * scale[1];
				struct ConstantData
				{
					float scale[2];
					float translate[2];
				} data;
				data = {
					.scale = { scale[0], scale[1] },
					.translate = { translate[0], translate[1] },
				};
				push_constants(encoder, device->imgui_shader, "pc", &data);

				set_global_texture(encoder, device->imgui_font_texture, 0, 0);
				set_global_sampler(encoder, device->imgui_font_sampler, 0, 1);

				ImVec2 clip_off = drawData->DisplayPos;         // (0,0) unless using multi-viewports
				ImVec2 clip_scale = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				int global_vtx_offset = 0;
				int global_idx_offset = 0;
				for (size_t i = 0; i < drawData->CmdListsCount; ++i)
				{
					const auto cmdList = drawData->CmdLists[i];
					for (size_t j = 0; j < cmdList->CmdBuffer.size(); ++j)
					{
						const auto cmdBuffer = &cmdList->CmdBuffer[j];

						ImVec2 clip_min((cmdBuffer->ClipRect.x - clip_off.x)* clip_scale.x, (cmdBuffer->ClipRect.y - clip_off.y)* clip_scale.y);
						ImVec2 clip_max((cmdBuffer->ClipRect.z - clip_off.x)* clip_scale.x, (cmdBuffer->ClipRect.w - clip_off.y)* clip_scale.y);

						// Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
						if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
						if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
						if (clip_max.x > fb_width) { clip_max.x = (float)fb_width; }
						if (clip_max.y > fb_height) { clip_max.y = (float)fb_height; }
						if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
							continue;

						// Apply scissor/clipping rectangle
						set_scissor(encoder, (clip_min.x), (clip_min.y), (clip_max.x - clip_min.x), (clip_max.y - clip_min.y));

						draw_submesh(encoder, device->imgui_shader, window->imgui_mesh, cmdBuffer->ElemCount, cmdBuffer->IdxOffset + global_idx_offset, 0, cmdBuffer->VtxOffset + global_vtx_offset);
					}
					global_idx_offset += cmdList->IdxBuffer.Size;
					global_vtx_offset += cmdList->VtxBuffer.Size;
				}
			}, sizeof(int*), (void**)&passdata);
		passdata->device = device;
		passdata->window = window;
	}
}

void renderImgui(oval_cgpu_device_t* device, HGEGraphics::rendergraph_t& rg, const std::pmr::vector<oval_window_impl_t*>& prepared_windows)
{
	for (auto window : prepared_windows)
	{
		auto back_buffer = oval_get_backbuffer_for_window(&device->super, window, rg);
		renderImgui(device, rg, back_buffer, window);
	}
}

HGEGraphics::texture_handle_t oval_get_backbuffer_for_window(struct oval_device_t* device, oval_window_t* window, HGEGraphics::rendergraph_t& rg)
{
	using namespace HGEGraphics;
	auto D = (oval_cgpu_device_t*)device;
	auto& registry = D->registry;

	auto window_impl = (oval_window_impl_t*)window;
	if (window_impl->swapchain == nullptr)
	{
		return {};
	}

	auto back_buffer = window_impl->current_back_buffer;
	if (back_buffer == nullptr)
		return {};
	return rendergraph_import_backbuffer(&rg, back_buffer);
}

void render(oval_cgpu_device_t* device, const oval_submit_context& submit_context, const std::pmr::vector<oval_window_impl_t*>& prepared_windows)
{
	using namespace HGEGraphics;

	std::pmr::unsynchronized_pool_resource rg_pool(device->memory_resource);
	rendergraph_t rg{ 1, 1, 1, device->blit_shader, device->blit_linear_sampler, &rg_pool };

	oval_graphics_transfer_queue_execute_all(device, rg);

	setupImGuiResources(device, rg, prepared_windows);

	if (device->super.descriptor.on_submit)
		device->super.descriptor.on_submit(&device->super, submit_context, rg);
	renderImgui(device, rg, prepared_windows);

	for (auto window : prepared_windows)
	{
		auto back_buffer = oval_get_backbuffer_for_window(&device->super, window, rg);
		rendergraph_present(&rg, back_buffer);
	}

	auto compiled = Compiler::Compile(rg, &rg_pool);
	Executor::Execute(compiled, device->frameDatas[device->info.current_frame_index].execContext);

	for (auto imported : rg.imported_textures)
	{
		imported->dynamic_handle = {};
	}
	for (auto imported : rg.imported_buffers)
	{
		imported->dynamic_handle = {};
	}

	oval_graphics_transfer_queue_release_all(device);
}

void release_swapchain_related_resources(oval_cgpu_device_t* D, CGPUSwapChainId swapchain)
{
	for (size_t i = 0; i < D->frameDatas.size(); ++i)
	{
		for (size_t j = 0; j < swapchain->back_buffer_count; ++j)
		{
			auto image = swapchain->p_back_buffers[j];
			D->frameDatas[i].execContext.textureViewPool.destroyRelativeTexture(image);
		}
	}
}

bool on_resize(oval_cgpu_device_t* D, oval_window_impl_t* window)
{
	if (SDL_GetWindowFlags(window->window) & SDL_WINDOW_MINIMIZED)
		return false;

	int w, h;
	SDL_GetWindowSize(window->window, &w, &h);

	if (w == 0 || h == 0)
		return false;

	ECGPUTextureFormat swapchainFormat = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB;
	CGPUSwapChainDescriptor descriptor = {
		.present_queue_count = 1,
		.p_present_queues = &D->present_queue,
		.surface = window->surface,
		.image_count = 3,
		.width = (uint32_t)w,
		.height = (uint32_t)h,
		.enable_vsync = true,
		.format = swapchainFormat,
	};

	if (window->swapchain != nullptr)
	{
		release_swapchain_related_resources(D, window->swapchain->handle);
	}

	auto new_swapchain = SwapChain::resize(std::move(window->swapchain), D->device, descriptor);
	if (!new_swapchain)
	{
		return false;
	}
	window->swapchain = std::move(new_swapchain);
	window->needResize = false;
	return true;
}

void oval_runloop(oval_device_t* device)
{
	auto D = (oval_cgpu_device_t*)device;

	SDL_Event e;
	bool quit = false;

	double time_since_startup = 0;
	double lag;
	if (D->super.descriptor.update_frequecy_mode == UPDATE_FREQUENCY_MODE_FIXED)
		lag = D->super.descriptor.fixed_update_time_step;
	else if (D->super.descriptor.update_frequecy_mode == UPDATE_FREQUENCY_MODE_VARIABLE && D->super.descriptor.render_frequecy_mode == RENDER_FREQUENCY_MODE_LIMITED)
		lag = 1.0 / D->super.descriptor.target_fps;
	else
		lag = 1.0 / 60;
	int countFrame = 0;
	int lastFPS = 0;
	std::vector<tf::Taskflow> update_flows;
	std::pmr::vector<oval_window_impl_t*> need_resize_windows(D->memory_resource);
	std::pmr::vector<oval_window_impl_t*> prepared_windows(D->memory_resource);
	std::pmr::vector<CGPUSemaphoreId> finish_semaphores(D->memory_resource);
	std::pmr::vector<CGPUSemaphoreId> wait_semaphores(D->memory_resource);

	auto lastFrameSampleTime = std::chrono::high_resolution_clock::now();
	auto lastCountFPSTime = lastFrameSampleTime;
	while (quit == false)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_EVENT_QUIT)
				quit = true;
			else
			{
				auto props = SDL_GetWindowProperties(SDL_GetWindowFromEvent(&e));
				auto window = (oval_window_impl_t*)SDL_GetPointerProperty(props, "sdl.window.userdata", nullptr);
				if (window)
				{
					ImGui::SetCurrentContext(window->imgui_context);
					ImGui_ImplSDL3_ProcessEvent(&e);
					if ((e.type & 0x200) != 0)
					{
						if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
						{
							auto mainwindow = (oval_window_impl_t*)D->mainwindow;
							if (window == mainwindow)
								quit = true;
						}
						else if (e.type == SDL_EVENT_WINDOW_RESIZED || e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED)
						{
							window->RequestResize();
						}
					}
				}
			}
		}

		need_resize_windows.clear();
		for (auto window : D->windows)
		{
			auto win_impl = (oval_window_impl_t*)window;
			if (win_impl->needResize || win_impl->swapchain == nullptr)
				need_resize_windows.push_back(win_impl);
		}

		if (!need_resize_windows.empty())
		{
			cgpu_queue_wait_idle(D->gfx_queue);
			for (auto window : need_resize_windows)
				on_resize(D, window);
		}

		D->info.current_frame_index = (D->info.current_frame_index + 1) % D->frameDatas.size();
		D->info.currentPacketFrame = (D->info.currentPacketFrame + 1) % 2;
		auto& cur_frame_data = D->frameDatas[D->info.current_frame_index];
		cgpu_wait_fences(1, &cur_frame_data.inflightFence);
		cur_frame_data.newFrame();

		prepared_windows.clear();
		for (auto window : D->windows)
		{
			auto win_impl = (oval_window_impl_t*)window;
			if (!win_impl->swapchain)
				continue;

			if (win_impl->AcquireNextImage(D->info.current_frame_index))
				prepared_windows.push_back(win_impl);
			else
				win_impl->RequestResize();
		}

		if (prepared_windows.empty())
			continue;

		finish_semaphores.clear();
		wait_semaphores.clear();
		for (auto window : prepared_windows)
		{
			wait_semaphores.push_back(window->current_prepared_semaphore);
			finish_semaphores.push_back(window->current_finish_semaphore);
			window->FetchImguiDrawData();
		}

		ImFontAtlasUpdateNewFrame(D->imgui_font, ImGui::GetFrameCount(), false);
		for (auto window : D->windows)
		{
			auto oval_window = (oval_window_impl_t*)window;
			if (oval_window->imgui_owned_context)
			{
				ImGui::SetCurrentContext(oval_window->imgui_context);
				ImGui_ImplSDL3_NewFrame();
				ImGui::NewFrame();
			}
		}

		bool rdc_capturing = false;
		if (D->rdc && D->rdc_capture)
		{
			D->rdc->StartFrameCapture(nullptr, nullptr);
			rdc_capturing = true;
		}

		tf::Taskflow flow;
		tf::Task last_update_flow = flow.placeholder();

		double fixed_update_time_step = D->super.descriptor.update_frequecy_mode == UPDATE_FREQUENCY_MODE_FIXED ? D->super.descriptor.fixed_update_time_step : lag;
		while (lag > 0 && lag >= fixed_update_time_step)
		{
			time_since_startup += fixed_update_time_step;
			oval_update_context update_context
			{
				.delta_time = (float)fixed_update_time_step,
				.time_since_startup = (float)time_since_startup,
				.delta_time_double = fixed_update_time_step,
				.time_since_startup_double = time_since_startup,
				.fps = lastFPS,
			};

			if (D->super.descriptor.on_update)
			{
				update_flows.push_back(D->super.descriptor.on_update(&D->super, update_context));
				auto update_task = flow.composed_of(update_flows.back()).name("update");
				update_task.succeed(last_update_flow);
				last_update_flow = update_task;
			}
			lag -= fixed_update_time_step;
		}

		double interpolation_time = D->super.descriptor.render_need_interpolate ? lag : 0;

		oval_render_context render_context
		{
			.delta_time = (float)fixed_update_time_step,
			.time_since_startup = (float)time_since_startup,
			.render_interpolation_time = (float)interpolation_time,
			.delta_time_double = fixed_update_time_step,
			.time_since_startup_double = time_since_startup,
			.render_interpolation_time_double = interpolation_time,
			.currentRenderPacketFrame = D->info.currentPacketFrame,
			.fps = lastFPS,
		};

		auto renderTask = flow.emplace([D, render_context]()
			{
				if (D->super.descriptor.on_render)
					D->super.descriptor.on_render(&D->super, render_context);
			}).name("render");

		auto imguiTask = flow.emplace([D, render_context]
			{
				if (D->super.descriptor.on_imgui)
					D->super.descriptor.on_imgui(&D->super, render_context);

				for (auto window : D->windows)
				{
					auto oval_window = (oval_window_impl_t*)window;
					if (oval_window->imgui_owned_context)
					{
						ImGui::SetCurrentContext(oval_window->imgui_context);
						ImGui::EndFrame();
						ImGui::Render();
					}
				}
			}).name("imgui");

		oval_submit_context submit_context
		{
			.submitRenderPacketFrame = (D->info.currentPacketFrame + 1) % 2,
		};
		auto submitTask = flow.emplace([D, &cur_frame_data, submit_context, &prepared_windows, &finish_semaphores, &wait_semaphores]
			{
				if (D->cur_transfer_queue)
					oval_graphics_transfer_queue_submit(&D->super, D->cur_transfer_queue);
				D->cur_transfer_queue = nullptr;
				oval_process_load_queue(D);

				render(D, submit_context, prepared_windows);

				CGPUQueueSubmitDescriptor submit_desc = {
					.cmd_count = (uint32_t)cur_frame_data.execContext.allocated_cmds.size(),
					.p_cmds = cur_frame_data.execContext.allocated_cmds.data(),
					.signal_fence = cur_frame_data.inflightFence,
					.wait_semaphore_count = (uint32_t)wait_semaphores.size(),
					.p_wait_semaphores = wait_semaphores.data(),
					.signal_semaphore_count = (uint32_t)finish_semaphores.size(),
					.p_signal_semaphores = finish_semaphores.data(),
				};

				cgpu_queue_submit(D->gfx_queue, &submit_desc);

				for (auto window : prepared_windows)
					window->Present(D->present_queue);
			}).name("submit");

		renderTask.succeed(last_update_flow);
		imguiTask.succeed(renderTask);
		D->taskExecutor.run(flow).wait();
		update_flows.clear();

		if (rdc_capturing)
		{
			D->rdc->EndFrameCapture(nullptr, nullptr);
			D->rdc_capture = false;
		}
		if (D->rdc && D->rdc_capture)
		{
			if (!D->rdc->IsRemoteAccessConnected())
			{
				D->rdc->LaunchReplayUI(1, "");
			}
		}

		long sleep_duration = 0;
		if (D->super.descriptor.render_frequecy_mode == RENDER_FREQUENCY_MODE_LIMITED)
		{
			long long render_max_delta_time = 1000000 / D->super.descriptor.target_fps;
			auto waiting = lastFrameSampleTime + std::chrono::microseconds(render_max_delta_time);
			while (std::chrono::high_resolution_clock::now() < waiting)
				std::this_thread::sleep_for(std::chrono::microseconds(0));
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::microseconds(0));
		}

		auto fpsSampleTime = std::chrono::high_resolution_clock::now();
		auto elapsedFPSTime = std::chrono::duration_cast<std::chrono::duration<double>>(fpsSampleTime - lastCountFPSTime).count();
		if (elapsedFPSTime < 1)
			countFrame++;
		else
		{
			lastFPS = countFrame;
			countFrame = 0;
			lastCountFPSTime = fpsSampleTime;
		}

		auto end_of_time = std::chrono::high_resolution_clock::now();
		auto elapsedTime = std::chrono::duration_cast<std::chrono::duration<double>>(end_of_time - lastFrameSampleTime).count();
		lag += elapsedTime;
		lastFrameSampleTime = end_of_time;
	}

	cgpu_queue_wait_idle(D->gfx_queue);

	for (int i = 0; i < 3; ++i)
	{
		D->frameDatas[i].execContext.pre_destroy();
	}
}

void oval_free_device(oval_device_t* device)
{
	auto D = (oval_cgpu_device_t*)device;

	D->default_texture = nullptr;
	D->imgui_shader = nullptr;
	D->imgui_font_texture = nullptr;
	D->imgui_font_sampler = CGPU_NULLPTR;
	D->blit_shader = nullptr;
	D->blit_linear_sampler = nullptr;

	for (auto window : D->windows)
	{
		auto window_impl = (oval_window_impl_t*)window;
		oval_free_window(device, window);
	}
	D->windows.clear();
	D->mainwindow = CGPU_NULLPTR;
	D->super.mainwindow_entity = entt::entity{};

	IM_DELETE(D->imgui_font);
	D->imgui_font = nullptr;

	D->info.reset();

	for (int i = 0; i < D->frameDatas.size(); ++i)
	{
		D->frameDatas[i].free();
	}

	D->materials.clear();
	D->meshes.clear();
	D->shaders.clear();
	D->computeShaders.clear();
	D->textures.clear();
	for (auto sampler : D->samplers)
		cgpu_device_free_sampler(D->device, sampler);
	D->samplers.clear();

	cgpu_device_free_queue(D->device, D->gfx_queue);
	D->gfx_queue = CGPU_NULLPTR;
	D->present_queue = CGPU_NULLPTR;
	cgpu_adapter_free_device(D->device->adapter, D->device);
	D->device = CGPU_NULLPTR;
	cgpu_free_instance(D->instance);
	D->instance = CGPU_NULLPTR;

	auto memory = D->memory_resource;


	SDL_Quit();

	delete D;

	delete memory;
}

void oval_render_debug_capture(oval_device_t* device)
{
	auto D = (oval_cgpu_device_t*)device;
	D->rdc_capture = true;
}

void oval_query_render_profile(oval_device_t* device, uint32_t* length, const char*** names, const float** durations)
{
	auto D = (oval_cgpu_device_t*)device;
	auto& cur_frame_data = D->frameDatas[D->info.current_frame_index];
	if (cur_frame_data.execContext.profiler)
	{
		cur_frame_data.execContext.profiler->Query(*length, *names, *durations);
	}
	else
	{
		*length = 0;
		*names = nullptr;
		*durations = nullptr;
	}
}

entt::registry* oval_get_registry(oval_device_t* device)
{
	auto D = (oval_cgpu_device_t*)device;
	return &D->registry;
}
