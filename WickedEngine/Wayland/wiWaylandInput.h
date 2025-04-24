#pragma once
#include <cstdint>

#include "wiInput.h"

typedef int Event;

namespace wi::input::wayland {
    struct Event {
        enum EventType {
            None = 0,
            Keyboard,
            Mouse,
        } type;
        struct KeyboardEvent {
            uint32_t time, key, state;
        } keyboard_event;
    };

    void Initialize();
    void Deinit();

    void SetKeyMap(uint32_t format, int32_t fd, uint32_t size);
    void ProcessKeyboardEvent(uint32_t time, uint32_t key, uint32_t state);

    void ProcessPointerButton(uint32_t time, uint32_t uint32, uint32_t state);
    void ProcessPointerMotion(uint32_t time, double x, double y);

    void Update();

    void GetKeyboardState(wi::input::KeyboardState* state);

    void GetMouseState(wi::input::MouseState* state);
}
