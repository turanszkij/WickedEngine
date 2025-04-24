#include "wiWaylandBackend.h"
#include "wiWaylandUtils.h"
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <iostream>

#include "wiWaylandInput.h"
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

extern "C" void _static_on_keyboard_keymap(void* data, wl_keyboard* keyboard,
										   uint32_t format,
										   int32_t fd, uint32_t size)
{
	Backend *backend = static_cast<Backend*>(data);
	wi::input::wayland::SetKeyMap(format, fd, size);
}

extern "C" void _static_on_keyboard_focuson(void* data, wl_keyboard* keyboard,
											uint32_t serial,
											wl_surface* surface,
											wl_array* keys)
{
	Backend *backend = static_cast<Backend*>(data);
	backend->keyboard_focuson(serial, surface, keys);
}

extern "C" void _static_on_keyboard_focusoff(void* data, wl_keyboard* keyboard,
											 uint32_t serial,
											 wl_surface* surface)
{
	Backend *backend = static_cast<Backend*>(data);
	backend->keyboard_focusoff(serial, surface);
}

extern "C" void _static_on_keyboard_key(void* data, wl_keyboard* keyboard,
										uint32_t serial,
										uint32_t time,
										uint32_t key,
										uint32_t state)
{
	Backend *backend = static_cast<Backend*>(data);
	// fprintf(stderr, "Key is %d state is %d\n", key, state);
	// backend->keyboard_key(serial, time, key, state);
	wi::input::wayland::ProcessKeyboardEvent(time, key, state);
}

extern "C" void _static_on_keyboard_modifiers(void* data, wl_keyboard* keyboard,
											  uint32_t serial,
											  uint32_t mods_depressed,
											  uint32_t mods_latched,
											  uint32_t mods_locked,
											  uint32_t group)
{
	Backend *backend = static_cast<Backend*>(data);
	// backend->keyboard_focusoff(serial, surface);
	fprintf(stderr, "Modifiers depressed %d, latched %d, locked %d, group %d\n",
			mods_depressed, mods_latched, mods_locked, group);
}

extern "C" void _static_on_keyboard_repeat_info(void* data, wl_keyboard* keyboard,
												int32_t rate,
												int32_t delay)
{
	Backend *backend = static_cast<Backend*>(data);
	// backend->keyboard_focusoff(serial, surface);
}

extern "C" void _static_on_pointer_enter(void *data,
                                         struct wl_pointer *wl_pointer,
                                         uint32_t serial,
                                         struct wl_surface *surface,
                                         wl_fixed_t surface_x,
                                         wl_fixed_t surface_y)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_leave(void *data,
                                         struct wl_pointer *wl_pointer,
                                         uint32_t serial,
                                         struct wl_surface *surface)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_motion(void *data,
                                          struct wl_pointer *wl_pointer,
                                          uint32_t time,
                                          wl_fixed_t surface_x,
                                          wl_fixed_t surface_y)
{
	double x = wl_fixed_to_double(surface_x), y = wl_fixed_to_double(surface_y);
	Backend *backend = static_cast<Backend*>(data);
	fprintf(stderr, "Mouse movement %f,%f\n", x, y);
	wi::input::wayland::ProcessPointerMotion(time, x, y);
}

extern "C" void _static_on_pointer_button(void *data,
                                          struct wl_pointer *wl_pointer,
                                          uint32_t serial,
                                          uint32_t time,
                                          uint32_t button,
                                          uint32_t state)
{
	Backend *backend = static_cast<Backend*>(data);
	wi::input::wayland::ProcessPointerButton(time, button, state);
	fprintf(stderr, "Mouse key is %d state is %d\n", button, state);
}

extern "C" void _static_on_pointer_axis(void *data,
                                        struct wl_pointer *wl_pointer,
                                        uint32_t time,
                                        uint32_t axis,
                                        wl_fixed_t value)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_frame(void *data, struct wl_pointer *wl_pointer)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_axis_source(void *data,
                                               struct wl_pointer *wl_pointer,
                                               uint32_t axis_source)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_axis_stop(void *data,
                                             struct wl_pointer *wl_pointer,
                                             uint32_t time,
                                             uint32_t axis)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_axis_discrete(void *data,
                                                 struct wl_pointer *wl_pointer,
                                                 uint32_t axis,
                                                 int32_t discrete)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_axis_value120(void *data,
                                                 struct wl_pointer *wl_pointer,
                                                 uint32_t axis,
                                                 int32_t value120)
{
	Backend *backend = static_cast<Backend*>(data);
}

extern "C" void _static_on_pointer_axis_relative_direction(void *data,
                                                           struct wl_pointer *wl_pointer,
                                                           uint32_t axis,
                                                           uint32_t direction)
{
	Backend *backend = static_cast<Backend*>(data);
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
	//optional, being "unstable" API (GNOME does not implement this)
	if (!bind_decoration_manager())		{std::cerr<<"bind decoration_manager failed"<<std::endl;}

	// mouse and keyboard focus and inputs
	if (!bind_wl_seat())				{std::cerr<<"bind wl seat failed"<<std::endl; return false;}
	keyboard = wl_seat_get_keyboard(seat);
	if (keyboard == nullptr)			{std::cerr<<"wl_keyboard not found"<<std::endl; return false;}
	pointer = wl_seat_get_pointer(seat);
	if (pointer == nullptr)				{std::cerr<<"wl_pointer not found"<<std::endl; return false;}
	keyboard_listener.keymap = &_static_on_keyboard_keymap;
	keyboard_listener.enter = &_static_on_keyboard_focuson;
	keyboard_listener.leave = &_static_on_keyboard_focusoff;
	keyboard_listener.key = &_static_on_keyboard_key;
	keyboard_listener.modifiers = &_static_on_keyboard_modifiers;
	keyboard_listener.repeat_info = &_static_on_keyboard_repeat_info;
	wl_keyboard_add_listener(keyboard, &keyboard_listener, this);

	//TODO add pointer image every time the pointer enters! see https://wayland.freedesktop.org/docs/html/ch04.html#sect-Protocol-Input
	pointer_listener.enter = &_static_on_pointer_enter;
	pointer_listener.leave = _static_on_pointer_leave;
	pointer_listener.motion = _static_on_pointer_motion;
	pointer_listener.button = _static_on_pointer_button;
	pointer_listener.axis = _static_on_pointer_axis;
	pointer_listener.frame = _static_on_pointer_frame;
	pointer_listener.axis_source = _static_on_pointer_axis_source;
	pointer_listener.axis_stop = _static_on_pointer_axis_stop;
	pointer_listener.axis_discrete = _static_on_pointer_axis_discrete;
	pointer_listener.axis_value120 = _static_on_pointer_axis_value120;
	pointer_listener.axis_relative_direction = _static_on_pointer_axis_relative_direction;
	wl_pointer_add_listener(pointer, &pointer_listener, this);

	wi::input::wayland::Initialize();
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

bool Backend::bind_wl_seat()
{
	seat = this->bind<wl_seat>("wl_seat", &wl_seat_interface);
	return seat != nullptr;
}

bool Backend::init_window_manager()
{
	window_manager_listener.ping = &::window_manager_pong;
	xdg_wm_base_add_listener(window_manager, &window_manager_listener, this);
	return true;
}

void Backend::DeInit()
{
	wi::input::wayland::Deinit();
	safe_delete(pointer, wl_pointer_release);
	safe_delete(keyboard, wl_keyboard_release);
	safe_delete(seat, wl_seat_destroy);
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

void Backend::keyboard_focuson(uint32_t serial, wl_surface* surface, wl_array* keys)
{
	if (surface != window.WLsurface)
		return;
	if (window.on_focus_on) window.on_focus_on(&window);
}

void Backend::keyboard_focusoff(uint32_t serial, wl_surface* surface)
{
	if (surface != window.WLsurface)
		return;
	if (window.on_focus_off) window.on_focus_off(&window);
}
