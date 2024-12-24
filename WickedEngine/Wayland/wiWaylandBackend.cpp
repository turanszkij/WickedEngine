#include "wiWaylandBackend.h"
#include "wiWaylandUtils.h"
#include <wayland/wayland-client-core.h>
#include <wayland/wayland-client-protocol.h>
#include <wayland/wayland-client.h>
#include <iostream>

#include "xdg-shell.h"
#include "xdg-decoration-unstable-v1.h"


using namespace wi::wayland;

/// functions callbacks - START
extern "C" void registry_global(void *data, wl_registry *raw_registry, uint32_t id, const char* interface, uint32_t version)
{
	Backend *backend = static_cast<Backend*>(data);
	backend->registry_event_add(id, interface, version);
}

extern "C" void registry_global_remove(void *data, wl_registry *raw_registry, uint32_t id)
{
	Backend *backend = static_cast<Backend*>(data);
	backend->registry_event_remove(id);
}

extern "C" void window_manager_pong(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
{
	Backend *backend = static_cast<Backend*>(data);
	backend->window_manager_pong(serial);
}

/// functions callbacks - END

bool Backend::Init()
{
	const char* name = nullptr;
	display = wl_display_connect(name);
	if (display == nullptr)				{std::cerr<<"wl_display_connect failed"<<std::endl; return false;}
	registry = wl_display_get_registry(display);
	if (registry == nullptr)			{std::cerr<<"wl_display_get_registry failed"<<std::endl; return false;}
	if (!init_registry())				{std::cerr<<"init registry failed"<<std::endl; return false;}

	if (!bind_compositor())				{std::cerr<<"bind compositor failed"<<std::endl; return false;}
	if (!registry_has("xdg_wm_base"))	{std::cerr<<"XDG WM BASE not found"<<std::endl; return false;}
	if (!bind_xdg_wm_base())			{std::cerr<<"bind xdg_wm_base failed"<<std::endl; return false;}
	if (!init_window_manager())			{std::cerr<<"xdg_wm_base init failed"<<std::endl; return false;}
	if (!bind_decoration_manager())		{std::cerr<<"bind decoration_manager failed"<<std::endl;} //optional, being "unstable" API (GNOME does not implement this)

	return true;
}

bool Backend::init_registry()
{
	registry_listener.global = &::registry_global;
	registry_listener.global_remove = &::registry_global_remove;
	wl_registry_add_listener(registry, &this->registry_listener, this);

	dispatch();
	roundtrip();

	return true;
}

bool Backend::registry_has(const char* interface_name) const
{
	return registry_map.find(interface_name) != registry_map.end();
}

bool Backend::bind_compositor()
{
	compositor = bind<wl_compositor>("wl_compositor", &wl_compositor_interface);
	return compositor != nullptr;
}

bool Backend::bind_xdg_wm_base()
{
	window_manager = this->bind<xdg_wm_base>("xdg_wm_base", &xdg_wm_base_interface);
	return window_manager != nullptr;
}

bool Backend::bind_decoration_manager()
{
	decoration_manager = this->bind<zxdg_decoration_manager_v1>(
		"zxdg_decoration_manager_v1", &zxdg_decoration_manager_v1_interface);
	return decoration_manager != nullptr;
}

bool Backend::init_window_manager()
{
	window_manager_listener.ping = &::window_manager_pong;
	xdg_wm_base_add_listener(window_manager, &window_manager_listener, this);
	return true;
}

void Backend::DeInit()
{
	safe_delete(decoration_manager, zxdg_decoration_manager_v1_destroy);
	safe_delete(window_manager, xdg_wm_base_destroy);
	safe_delete(compositor, wl_compositor_destroy);
	// registry needs to be killed before, otherwhise the application will crash
	safe_delete(registry, wl_registry_destroy);
	safe_delete(display, wl_display_disconnect);
}

bool Backend::CreateWindow(const char* title)
{
	return window.Init(this, title);
}

int Backend::dispatch()
{
	return wl_display_dispatch(display);
}

int Backend::roundtrip()
{
	return wl_display_roundtrip(display);
}

void Backend::registry_event_add(uint32_t id, const char* interface, uint32_t version)
{
	std::cout << "Registry has " << interface << std::endl;
	registry_map[interface] = std::make_pair(id, version);
}

void Backend::registry_event_remove(uint32_t id)
{
	for (auto it = registry_map.begin(); it != registry_map.end(); ++it) {
		if (it->second.first == id) {
			registry_map.erase(it);
			return;
		}
	}
	std::clog << "Registry removal failed, not found: " << id << std::endl;
}

void Backend::window_manager_pong(uint32_t serial)
{
	xdg_wm_base_pong(window_manager, serial);
}
