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
	oval_window->width = w;
	oval_window->height = h;
	oval_window->on_imgui = window_descriptor->on_imgui;

	auto D = (oval_cgpu_device_t*)device;

	auto window_props = SDL_GetWindowProperties(window);
	void* native_view = nullptr;
#ifdef _WIN32
	auto hwnd = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
	native_view = hwnd;
#elif defined(__ANDROID__)
	auto handle = SDL_GetPointerProperty(window_props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
	native_view = handle;
#endif

	oval_window->surface = cgpu_instance_create_surface_from_native_view(D->instance, native_view);

	ECGPUTextureFormat swapchainFormat = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB;
	CGPUSwapChainDescriptor swap_chain_descriptor = {
		.present_queue_count = 1,
		.p_present_queues = &D->present_queue,
		.surface = oval_window->surface,
		.image_count = 3,
		.width = (uint32_t)w,
		.height = (uint32_t)h,
		.enable_vsync = true,
		.format = swapchainFormat,
	};
	oval_window->swapchain = cgpu_device_create_swap_chain(D->device, &swap_chain_descriptor);
	oval_window->backbuffer.resize(oval_window->swapchain->back_buffer_count);
	oval_window->swapchain_prepared_semaphores.resize(oval_window->swapchain->back_buffer_count);
	oval_window->render_finished_semaphores.resize(oval_window->swapchain->back_buffer_count);
	for (uint32_t i = 0; i < oval_window->swapchain->back_buffer_count; i++)
	{
		HGEGraphics::init_backbuffer(&oval_window->backbuffer[i], oval_window->swapchain, i);
		oval_window->swapchain_prepared_semaphores[i] = cgpu_device_create_semaphore(D->device);
		oval_window->render_finished_semaphores[i] = cgpu_device_create_semaphore(D->device);
	}

	D->windows.push_back(oval_window);
	return oval_window;
}

void oval_free_window(oval_device_t* device, oval_window_t* window)
{
	auto oval_window = (oval_window_impl_t*)window;
	auto D = (oval_cgpu_device_t*)device;
	auto iter = std::find(D->windows.begin(), D->windows.end(), oval_window);
	if (iter != D->windows.end())
	{
		for (uint32_t i = 0; i < oval_window->swapchain_prepared_semaphores.size(); ++i)
		{
			cgpu_device_free_semaphore(D->device, oval_window->swapchain_prepared_semaphores[i]);
			oval_window->swapchain_prepared_semaphores[i] = CGPU_NULLPTR;
		}
		oval_window->swapchain_prepared_semaphores.clear();

		for (uint32_t i = 0; i < oval_window->render_finished_semaphores.size(); ++i)
		{
			cgpu_device_free_semaphore(D->device, oval_window->render_finished_semaphores[i]);
			oval_window->render_finished_semaphores[i] = CGPU_NULLPTR;
		}
		oval_window->render_finished_semaphores.clear();

		oval_window->backbuffer.clear();

		cgpu_device_free_swap_chain(D->device, oval_window->swapchain);
		oval_window->swapchain = CGPU_NULLPTR;
		cgpu_instance_free_surface(D->instance, oval_window->surface);
		oval_window->surface = CGPU_NULLPTR;

		SDL_DestroyWindow(oval_window->window);

		delete oval_window;
		D->windows.erase(iter);
	}
}
