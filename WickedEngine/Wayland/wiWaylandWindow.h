#pragma once
#include "wayland-client-core.h"
#include "wayland-client-protocol.h"
#include "xdg-decoration-unstable-v1.h"
#include "xdg-shell.h"
#include <functional>

#define VK_NO_PROTOTYPES
#include "Utility/vulkan/vulkan.h"
// #include "Utility/volk.h"

extern "C" {
	struct xdg_wm_base;
	struct zxdg_decoration_manager_v1;
}

namespace wi::wayland {

class Backend;

class Window {
private:
	// reference variables
	Backend* backend = nullptr;

	// OWN variables
	wl_surface *WLsurface = nullptr;
	xdg_surface *XDGsurface = nullptr;
	xdg_toplevel *toplevel = nullptr;
	zxdg_toplevel_decoration_v1* toplevel_decoration = nullptr;

	wl_callback* frame_callback = nullptr;
	wl_surface_listener WLsurface_listener;
	wl_callback_listener frame_listener;
	xdg_surface_listener XDGsurface_listener;
	xdg_toplevel_listener toplevel_listener;

	int32_t current_width = 800;
	int32_t current_height = 600;
	int32_t desired_width = 0;
	int32_t desired_height = 0;

public:
	std::function<void(Window* window)> on_redraw_callback;
	std::function<void(Window* window)> on_close;
	std::function<void(Window* window)> on_focus_on;
	std::function<void(Window* window)> on_focus_off;

public:
	~Window() { Deinit(); }
	void Deinit();

	Window() = default;
	bool Init(Backend* backend, const char* title = nullptr);
	void SetTitle(const char* title);
	void GetWindowCurrentSize(int32_t* width, int32_t* height);
	bool HasPendingResize() const;
	bool AcceptDesiredDimensions();
	void SetFullscreen(bool fullscreen);

	VkResult CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* VKSurface);

	// callbacks
	void _on_enter_monitor(wl_output *output);
	void _on_leave_monitor(wl_output *output);
	void _on_redraw(wl_callback *wl_callback, uint32_t time);
	void _on_xdg_surface_ack_configure(uint32_t serial);
	void _on_toplevel_close();
	void _on_toplevel_configure(int32_t width, int32_t height, wl_array *states);
	void _on_toplevel_configure_bounds(int32_t width, int32_t height);

friend class Backend;
};

}
