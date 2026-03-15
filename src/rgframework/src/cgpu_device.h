#pragma once

#include "framework.h"
#include <SDL3/SDL.h>
#include "imgui.h"
#include "renderdoc_helper.h"
#include <queue>
#include "renderer.h"
#include <taskflow/taskflow.hpp>
#include "imgui_threaded_rendering.h"

struct oval_transfer_data_to_texture
{
	HGEGraphics::Texture* texture;
	uint8_t* data;
	uint64_t size;
	uint32_t mipmap;
	uint32_t slice;
	bool transfer_full;
	bool generate_mipmap;
	uint8_t generate_mipmap_from;
};

struct oval_transfer_data_to_buffer
{
	HGEGraphics::Buffer* buffer;
	uint8_t* data;
	uint64_t size;
};

struct oval_graphics_transfer_queue
{
	oval_graphics_transfer_queue(std::pmr::memory_resource* memory_resource)
		: textures(memory_resource), buffers(memory_resource), memory_resource(memory_resource)
	{
	}

	std::pmr::monotonic_buffer_resource memory_resource;
	std::pmr::vector<oval_transfer_data_to_texture> textures;
	std::pmr::vector<oval_transfer_data_to_buffer> buffers;
};

struct FrameData
{
	CGPUFenceId inflightFence;
	HGEGraphics::ExecutorContext execContext;

	FrameData(CGPUDeviceId device, CGPUQueueId gfx_queue, bool profile, std::pmr::memory_resource* memory_resource)
		: execContext(device, gfx_queue, profile, memory_resource)
	{
		inflightFence = cgpu_device_create_fence(device);
	}

	void newFrame()
	{
		execContext.newFrame();
	}

	void free()
	{
		execContext.destroy();

		cgpu_device_free_fence(inflightFence->device, inflightFence);
		inflightFence = CGPU_NULLPTR;
	}
};

struct FrameInfo
{
	uint32_t current_frame_index{ 0 };
	size_t currentPacketFrame{ 0 };

	void reset()
	{
		current_frame_index = -1;
		currentPacketFrame = -1;
	}
};

enum class WaitLoadResourceType
{
	Texture,
	Mesh,
};

struct WaitLoadResource
{
	WaitLoadResourceType type;
	const char* path;
	size_t path_size;
	union {
		struct {
			HGEGraphics::Texture* texture;
			bool mipmap;
		} textureResource;
		struct {
			HGEGraphics::Mesh* mesh;
		} meshResource;
	};
};

struct TexturedVertex
{
	HMM_Vec3 position;
	HMM_Vec3 normal;
	HMM_Vec2 texCoord;
};

struct SwapChain
{
	CGPUSwapChainId handle{ CGPU_NULLPTR };
	std::vector<HGEGraphics::Backbuffer> backbuffer;
	std::vector<CGPUSemaphoreId> swapchain_prepared_semaphores;
	std::vector<CGPUSemaphoreId> render_finished_semaphores;

	static std::unique_ptr<SwapChain> create(CGPUDeviceId device, const CGPUSwapChainDescriptor& swap_chain_descriptor);
	static std::unique_ptr<SwapChain> resize(std::unique_ptr<SwapChain> old_swap_chain, CGPUDeviceId device, const CGPUSwapChainDescriptor& swap_chain_descriptor);

	~SwapChain();

private:
	SwapChain(const SwapChain&) = delete;
	SwapChain& operator=(const SwapChain&) = delete;
	SwapChain(const SwapChain&&) = delete;
	SwapChain& operator=(const SwapChain&&) = delete;

	SwapChain() {}
};

struct oval_window_impl_t : oval_window_t {
	SDL_Window* window;
	SDL_WindowID windowId;
	CGPUSurfaceId surface;
	std::unique_ptr<SwapChain> swapchain;
	uint32_t current_swapchain_index;
	HGEGraphics::Backbuffer* current_back_buffer;
	CGPUSemaphoreId current_prepared_semaphore;
	CGPUSemaphoreId current_finish_semaphore;
	bool needResize;
	ImGuiViewport* imgui_viewport;
	ImDrawDataSnapshot snapshot;
	ImDrawData* imgui_draw_data = nullptr;
	HGEGraphics::Mesh* imgui_mesh = nullptr;

	void RequestResize()
	{
		needResize = true;
	}

	bool AcquireNextImage(uint32_t frame_index)
	{
		current_back_buffer = nullptr;
		current_prepared_semaphore = CGPU_NULLPTR;
		current_finish_semaphore = CGPU_NULLPTR;

		if (!swapchain)
			return false;

		current_prepared_semaphore = swapchain->swapchain_prepared_semaphores[frame_index];

		CGPUAcquireNextDescriptor acquire_desc = {
			.signal_semaphore = current_prepared_semaphore,
		};

		auto res = cgpu_swap_chain_acquire_next_image(swapchain->handle, &acquire_desc, &current_swapchain_index);
		if (current_swapchain_index < swapchain->handle->back_buffer_count)
		{
			current_finish_semaphore = swapchain->render_finished_semaphores[current_swapchain_index];
			current_back_buffer = &swapchain->backbuffer[current_swapchain_index];
			return true;
		}
		else
		{
			return false;
		}
	}

	void Present(CGPUQueueId present_queue)
	{
		CGPUQueuePresentDescriptor present_desc = {
			.swapchain = swapchain->handle,
			.wait_semaphore_count = 1,
			.p_wait_semaphores = &current_finish_semaphore,
			.index = (uint8_t)current_swapchain_index,
		};
		cgpu_queue_present(present_queue, &present_desc);
	}

	void FetchImguiDrawData()
	{
		ImDrawData* drawData = imgui_viewport->DrawData;
		if (drawData)
			snapshot.SnapUsingSwap(drawData, ImGui::GetTime());
	}
};

typedef struct oval_cgpu_device_t {
	oval_cgpu_device_t(const oval_device_t& super, std::pmr::memory_resource* memory_resource)
		: super(super), windows(memory_resource), memory_resource(memory_resource), transfer_queue(memory_resource), allocator(memory_resource), wait_load_resources(memory_resource)
	{
	}

	oval_device_t super;
	oval_window_t* mainwindow;
	std::pmr::vector<oval_window_t*> windows;
	std::pmr::memory_resource* memory_resource;
	std::pmr::polymorphic_allocator<std::byte> allocator;
	CGPUInstanceId instance;
	CGPUDeviceId device;
	CGPUQueueId gfx_queue;
	CGPUQueueId present_queue;

	std::vector<FrameData> frameDatas;
	FrameInfo info;

	HGEGraphics::Shader* blit_shader = nullptr;
	CGPUSamplerId blit_linear_sampler = CGPU_NULLPTR;

	HGEGraphics::Texture* imgui_font_texture = nullptr;
	HGEGraphics::Shader* imgui_shader = nullptr;
	CGPUSamplerId imgui_font_sampler = CGPU_NULLPTR;

	bool rdc_capture = false;
	RENDERDOC_API_1_0_0* rdc = nullptr;

	std::pmr::vector<oval_graphics_transfer_queue*> transfer_queue;
	std::queue<WaitLoadResource, std::pmr::deque<WaitLoadResource>> wait_load_resources;
	oval_graphics_transfer_queue* cur_transfer_queue = nullptr;

	HGEGraphics::Texture* default_texture;

	tf::Executor taskExecutor{ (size_t)std::max((int)std::thread::hardware_concurrency() - 2, 1) };

	std::pmr::vector<std::unique_ptr<HGEGraphics::Mesh>> meshes;
	std::pmr::vector<std::unique_ptr<HGEGraphics::Shader>> shaders;
	std::pmr::vector<std::unique_ptr<HGEGraphics::ComputeShader>> computeShaders;
	std::pmr::vector<CGPUSamplerId> samplers;
	std::pmr::vector<std::unique_ptr<HGEGraphics::Texture>> textures;
	std::pmr::vector<std::unique_ptr<HGEGraphics::Material>> materials;
	
	entt::registry registry;
} oval_cgpu_device_t;

void oval_process_load_queue(oval_cgpu_device_t* device);
void oval_graphics_transfer_queue_execute_all(oval_cgpu_device_t* device, HGEGraphics::rendergraph_t& rg);
void oval_graphics_transfer_queue_release_all(oval_cgpu_device_t* device);
uint64_t load_mesh(oval_cgpu_device_t* device, oval_graphics_transfer_queue_t queue, HGEGraphics::Mesh* mesh, const char* filepath);
uint64_t load_texture(oval_cgpu_device_t* device, oval_graphics_transfer_queue_t queue, HGEGraphics::Texture* texture, const char* filepath, bool mipmap);
std::vector<uint8_t> readfile(const char* filename);
