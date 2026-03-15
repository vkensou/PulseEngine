#include "cgpu_device.h"
#include <algorithm>

oval_window_t* oval_create_window(oval_device_t* device, const oval_window_descriptor* window_descriptor)
{
	SDL_PropertiesID props = SDL_CreateProperties();
	SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, window_descriptor->title);
	SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
	SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, window_descriptor->width);
	SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, window_descriptor->height);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, window_descriptor->resizable);
	SDL_SetBooleanProperty(props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);

	SDL_Window* window = SDL_CreateWindowWithProperties(props);

	SDL_DestroyProperties(props);

	if (window == nullptr)
		return nullptr;

	int w, h;
	SDL_GetWindowSize(window, &w, &h);

	auto oval_window = new oval_window_impl_t();
	oval_window->window = window;
	oval_window->windowId = SDL_GetWindowID(window);
	oval_window->width = w;
	oval_window->height = h;
	oval_window->on_imgui = window_descriptor->on_imgui;
	oval_window->needResize = false;
	oval_window->current_swapchain_index = 0;
	oval_window->current_finish_semaphore = CGPU_NULLPTR;

	auto D = (oval_cgpu_device_t*)device;

	auto window_props = SDL_GetWindowProperties(window);
	SDL_SetPointerProperty(window_props, "sdl.window.userdata", oval_window);
	void* native_view = nullptr;
#ifdef _WIN32
	auto hwnd = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	native_view = hwnd;
#elif defined(__ANDROID__)
	auto handle = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
	native_view = handle;
#endif

	oval_window->surface = cgpu_instance_create_surface_from_native_view(D->instance, native_view);

	D->windows.push_back(oval_window);
	return oval_window;
}

void oval_free_window(oval_cgpu_device_t* D, oval_window_impl_t* oval_window)
{
	auto iter = std::find(D->windows.begin(), D->windows.end(), oval_window);
	if (iter != D->windows.end())
	{
		oval_window->swapchain.reset();
		cgpu_instance_free_surface(D->instance, oval_window->surface);
		oval_window->surface = CGPU_NULLPTR;

		SDL_DestroyWindow(oval_window->window);

		delete oval_window;
		D->windows.erase(iter);
	}
}

void oval_free_window(oval_device_t* device, oval_window_t* window)
{
	auto oval_window = (oval_window_impl_t*)window;
	auto D = (oval_cgpu_device_t*)device;
	oval_free_window(D, oval_window);
}

entt::entity oval_create_window_entity(oval_device_t* device, const oval_window_descriptor* window_descriptor)
{
	auto D = (oval_cgpu_device_t*)device;
	auto& registry = D->registry;

	auto window_handle = oval_create_window(device, window_descriptor);

	auto window_entity = registry.create();
	registry.emplace<WindowComponent>(window_entity, window_handle);

	return window_entity;
}

void oval_free_window_entity(oval_device_t* device, entt::entity window_entity)
{
	auto D = (oval_cgpu_device_t*)device;
	auto& registry = D->registry;

	auto window = registry.try_get<WindowComponent>(window_entity);
	if (window != nullptr)
	{
		auto oval_window = (oval_window_impl_t*)window->handle;

		oval_free_window(D, oval_window);

		registry.destroy(window_entity);
	}
}

void oval_get_window_size(oval_window_t* window, int* width, int* height)
{
	auto oval_window = (oval_window_impl_t*)window;
	int w, h;
	SDL_GetWindowSize(oval_window->window, &w, &h);
	*width = w;
	*height = h;
}

std::unique_ptr<SwapChain> SwapChain::create(CGPUDeviceId device, const CGPUSwapChainDescriptor& swap_chain_descriptor)
{
	return resize(nullptr, device, swap_chain_descriptor);
}

SwapChain::~SwapChain()
{
	for (auto semaphore : swapchain_prepared_semaphores)
	{
		cgpu_device_free_semaphore(semaphore->device, semaphore);
	}
	swapchain_prepared_semaphores.clear();

	for (auto semaphore : render_finished_semaphores)
	{
		cgpu_device_free_semaphore(semaphore->device, semaphore);
	}
	render_finished_semaphores.clear();

	backbuffer.clear();

	if (handle != CGPU_NULLPTR)
		cgpu_device_free_swap_chain(handle->device, handle);
}

std::unique_ptr<SwapChain> SwapChain::resize(std::unique_ptr<SwapChain> old_swap_chain, CGPUDeviceId device, const CGPUSwapChainDescriptor& swap_chain_descriptor)
{
	auto swapchain = std::move(old_swap_chain);
	CGPUSwapChainId old_handle = CGPU_NULLPTR;
	if (!swapchain)
	{
		swapchain = std::unique_ptr<SwapChain>(new SwapChain());
	}
	else
	{
		old_handle = swapchain->handle;
		swapchain->handle = CGPU_NULLPTR;
		swapchain->backbuffer.clear();
	}

	CGPUSwapChainDescriptor descriptor = swap_chain_descriptor;
	descriptor.old_swap_chain = old_handle != CGPU_NULLPTR ? old_handle : CGPU_NULLPTR;
	auto handle = cgpu_device_create_swap_chain(device, &descriptor);

	if (old_handle != CGPU_NULLPTR)
	{
		cgpu_device_free_swap_chain(old_handle->device, old_handle);
	}

	if (handle == CGPU_NULLPTR)
		return nullptr;
	swapchain->handle = handle;

	swapchain->backbuffer.resize(handle->back_buffer_count);
	for (uint32_t i = 0; i < handle->back_buffer_count; i++)
	{
		HGEGraphics::init_backbuffer(&swapchain->backbuffer[i], handle, i);
	}

	for (uint32_t i = handle->back_buffer_count; i < swapchain->swapchain_prepared_semaphores.size(); ++i)
	{
		cgpu_device_free_semaphore(device, swapchain->swapchain_prepared_semaphores.back());
		swapchain->swapchain_prepared_semaphores.pop_back();
	}
	for (uint32_t i = swapchain->swapchain_prepared_semaphores.size(); i < handle->back_buffer_count; ++i)
	{
		auto semaphore = cgpu_device_create_semaphore(device);
		if (semaphore == CGPU_NULLPTR)
			return nullptr;
		swapchain->swapchain_prepared_semaphores.push_back(semaphore);
	}
	assert(swapchain->swapchain_prepared_semaphores.size() == handle->back_buffer_count);
	for (uint32_t i = handle->back_buffer_count; i < swapchain->render_finished_semaphores.size(); ++i)
	{
		cgpu_device_free_semaphore(device, swapchain->render_finished_semaphores.back());
		swapchain->render_finished_semaphores.pop_back();
	}
	for (uint32_t i = swapchain->render_finished_semaphores.size(); i < handle->back_buffer_count; ++i)
	{
		auto semaphore = cgpu_device_create_semaphore(device);
		if (semaphore == CGPU_NULLPTR)
			return nullptr;
		swapchain->render_finished_semaphores.push_back(semaphore);
	}
	assert(swapchain->render_finished_semaphores.size() == handle->back_buffer_count);

	return swapchain;
}