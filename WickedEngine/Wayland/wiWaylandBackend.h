#pragma once
#include "wiWaylandWindow.h"
#include <wayland/wayland-client.h>
#include <map>
#include <string>
#include <iostream>

#include "xdg-decoration-unstable-v1.h"
#include "xdg-shell.h"

extern "C" {
struct xdg_wm_base;
struct zxdg_decoration_manager_v1;
}

namespace wi::wayland {

class Window;

class Backend {
private:
	wl_display* display = nullptr;
	wl_registry* registry = nullptr;
	wl_registry_listener registry_listener;
	// interface name -> id, version
	std::map<std::string, std::pair<uint32_t, uint32_t>> registry_map;

	wl_compositor* compositor = nullptr;
	xdg_wm_base* window_manager = nullptr;
	xdg_wm_base_listener window_manager_listener;
	zxdg_decoration_manager_v1 *decoration_manager = nullptr;

public:
	Window window;

public:
	Backend() = default;
	bool Init();
	~Backend() { DeInit(); }
	void DeInit();

	bool CreateWindow(const char* title);

	int dispatch();
	int roundtrip();

	// callbacks
	void registry_event_add(uint32_t id, const char* interface, uint32_t version);
	void registry_event_remove(uint32_t id);
	void window_manager_pong(uint32_t serial);

private:
	bool init_registry();
	bool registry_has(const char* interface_name) const;

	template<typename T>
	T* bind(const char* interface_name, const struct wl_interface *interface, uint32_t version = -1)
	{
		std::pair<uint32_t, uint32_t> reg = registry_map.at(interface_name);
		if (version < 0) {
			version = reg.second;
		} else {
			version = 1;
		}
		void* r = wl_registry_bind(registry, reg.first, interface, version);
		if (r == nullptr) {
			std::clog<< "ERROR loading interface \"" << interface_name << '"' << std::endl;
		}
		return static_cast<T*>(r);
	}
	bool bind_compositor();
	bool bind_xdg_wm_base();
	bool bind_decoration_manager();

	bool init_window_manager();

friend class Window;
};

}
