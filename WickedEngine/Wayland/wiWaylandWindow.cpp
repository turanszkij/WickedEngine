#include "wiWaylandWindow.h"
#include "wiWaylandBackend.h"
#include "wayland-client-protocol.h"
#include "wiWaylandUtils.h"
#include "xdg-decoration-unstable-v1.h"
#include "xdg-shell.h"
#include <iostream>

#define VK_NO_PROTOTYPES
#include "Utility/vulkan/vulkan.h"
#include "Utility/vulkan/vulkan_wayland.h"
#include "Utility/volk.h"

using namespace wi::wayland;

extern "C" void _static_on_redraw(void *data,
								  wl_callback *wl_callback,
								  uint32_t time)
{
	static_cast<Window*>(data)->on_redraw(wl_callback, time);
}

extern "C" void _static_on_enter(void *data,
								 wl_surface *wl_surface,
								 wl_output *output)
{
	static_cast<Window*>(data)->on_enter(output);
}

extern "C" void _static_on_leave(void *data,
								 wl_surface *wl_surface,
								 wl_output *output)
{
	static_cast<Window*>(data)->on_leave(output);
}

extern "C" void _static_xdg_surface_configure(void* data, xdg_surface* raw, uint32_t serial)
{
	static_cast<Window*>(data)->on_xdg_surface_ack_configure(serial);
}

extern "C" void _static_toplevel_close(void *data, xdg_toplevel *xdg_toplevel)
{
	static_cast<Window*>(data)->on_toplevel_close();
}

extern "C" void _static_toplevel_configure(void *data,
					  xdg_toplevel *xdg_toplevel,
					  int32_t width,
					  int32_t height,
					  wl_array *states)
{
	static_cast<Window*>(data)->on_toplevel_configure(width, height, states);
}

extern "C" void _static_toplevel_configure_bounds(void *data,
							 xdg_toplevel *xdg_toplevel,
							 int32_t width,
							 int32_t height)
{
	static_cast<Window*>(data)->on_toplevel_configure_bounds(width, height);
}

void Window::on_enter(wl_output *output)
{}

void Window::on_leave(wl_output *output)
{}

void Window::on_redraw(wl_callback *wl_callback, uint32_t time)
{
	if(frame_callback != nullptr) {
		wl_callback_destroy(frame_callback);
		frame_callback = nullptr;
	}
	if (on_redraw_callback)
		on_redraw_callback(this);
}

void Window::on_xdg_surface_ack_configure(uint32_t serial)
{
	xdg_surface_ack_configure(XDGsurface, serial);
}

void Window::on_toplevel_close()
{
	if (on_close) on_close(this);
}

void Window::on_toplevel_configure(int32_t width, int32_t height, wl_array* states)
{
	desired_width = width;
	desired_height = height;
}

void Window::on_toplevel_configure_bounds(int32_t width, int32_t height)
{}


bool Window::Init(Backend* backend, const char* title)
{
	this->backend = backend;

	// Surface
	WLsurface = wl_compositor_create_surface(backend->compositor);
	if (WLsurface == nullptr) {std::cerr << "Could not create surface from compositor" << std::endl; return false;}
	WLsurface_listener.enter = &_static_on_enter;
	WLsurface_listener.leave = &_static_on_leave;
	frame_listener.done = &_static_on_redraw;
	wl_surface_add_listener(WLsurface, &WLsurface_listener, this);

	// XDG Surface
	XDGsurface = xdg_wm_base_get_xdg_surface(backend->window_manager, WLsurface);
	if (XDGsurface == nullptr) {std::cerr << "Could not create XDG surface from WL surface" << std::endl; return false;}
	XDGsurface_listener.configure = &_static_xdg_surface_configure;
	xdg_surface_add_listener(XDGsurface, &XDGsurface_listener, this);

	// Top Level
	toplevel = xdg_surface_get_toplevel(XDGsurface);
	if (toplevel == nullptr) {std::cerr << "Could not create XDG surface from WL surface" << std::endl; return false;}
	toplevel_listener.close = &_static_toplevel_close;
	toplevel_listener.configure = &_static_toplevel_configure;
	toplevel_listener.configure_bounds = &_static_toplevel_configure_bounds;
	xdg_toplevel_add_listener(toplevel, &toplevel_listener, this);

	// DecorationManager
	if (backend->decoration_manager)
	{
		toplevel_decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(backend->decoration_manager, toplevel);
		if (toplevel_decoration != nullptr)
		{
			zxdg_toplevel_decoration_v1_set_mode(toplevel_decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
		}
	}

	SetTitle(title);
	return true;
}

void Window::Deinit()
{
	safe_delete(frame_callback, wl_callback_destroy);
	safe_delete(toplevel_decoration, zxdg_toplevel_decoration_v1_destroy);
	safe_delete(toplevel, xdg_toplevel_destroy);
	safe_delete(XDGsurface, xdg_surface_destroy);
	safe_delete(WLsurface, wl_surface_destroy);
}

void Window::SetTitle(const char* title)
{
	if (title == nullptr)
		return;
	if (toplevel == nullptr)
		return;
	xdg_toplevel_set_title(toplevel, title);
}

void Window::GetWindowCurrentSize(int32_t *width, int32_t *height)
{
	*width = current_width;
	*height = current_height;
}

bool Window::HasPendingResize() const
{
	return desired_height > 0 || desired_height > 0;
}

bool Window::AcceptDesiredDimensions()
{
	const bool has_pending_resize = HasPendingResize();
	if (desired_width > 0)
	{
		current_width = desired_width;
		desired_width = 0;
	}
	if (desired_height > 0)
	{
		current_height = desired_height;
		desired_height = 0;
	}
	return has_pending_resize;
}

void Window::SetFullscreen(bool fullscreen)
{
	if (fullscreen)
	{
		// wl_output output;
		// xdg_toplevel_set_fullscreen(toplevel, &output);
	}
	else
	{
		xdg_toplevel_unset_fullscreen(toplevel);
	}
}

VkResult Window::CreateVulkanSurface(VkInstance instance, VkSurfaceKHR* VKsurface)
{
	PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR =
	    (PFN_vkCreateWaylandSurfaceKHR)vkGetInstanceProcAddr(
	        instance,
	        "vkCreateWaylandSurfaceKHR");

	VkWaylandSurfaceCreateInfoKHR createInfo {};
	createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.display = backend->display;
	createInfo.surface = WLsurface;

	const VkAllocationCallbacks* allocator = nullptr;

	VkResult res = vkCreateWaylandSurfaceKHR(instance, &createInfo, allocator, VKsurface);

	backend->roundtrip();
	return res;
}
