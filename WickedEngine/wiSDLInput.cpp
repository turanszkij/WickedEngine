#include "CommonInclude.h"
#include "wiInput.h"
#include "wiSDLInput.h"

#ifdef SDL2

#include <SDL2/SDL.h>
#include <iostream>

namespace wiSDLInput
{
    wiInput::KeyboardState keyboard;
    wiInput::MouseState mouse;

    //TODO controllers
    //struct Internal_ControllerState
    //{
    //    HANDLE handle = NULL;
    //    bool is_xinput = false;
    //    std::wstring name;
    //    wiInput::ControllerState state;
    //};
    //std::vector<Internal_ControllerState> controllers;

    int to_wiInput(const SDL_Scancode &key);

    void Initialize() {}
    void Update()
    {
        auto saved_x = mouse.position.x;
        auto saved_y = mouse.position.y;

        // update keyboard and mouse to the latest value (in case of other input systems I suppose)
//        keyboard = wiInput::KeyboardState();
//        mouse = wiInput::MouseState();
//        for (auto& internal_controller : controllers)
//        {
//            internal_controller.state = wiInput::ControllerState();
//        }
        mouse.position.x = saved_x;
        mouse.position.y = saved_y;

        std::vector<SDL_Event> events(1000);

        // This removes the only the inputs events from the event queue, leaving audio and window events for other
        // section of the code
        SDL_PumpEvents();
        int ret = SDL_PeepEvents(events.data(), events.size(), SDL_GETEVENT, SDL_KEYDOWN, SDL_MULTIGESTURE);
        if (ret < 0) {
            std::cerr << "Error Peeping event: " << SDL_GetError() << std::endl;
            return;
        } else if (ret > 0) {
            events.resize(ret);

            for (const SDL_Event &event : events) {
                switch (event.type) {
                    // Keyboard events
                    case SDL_KEYDOWN:             // Key pressed
                    {
                        int converted = to_wiInput(event.key.keysym.scancode);
                        if (converted >= 0) {
                            keyboard.buttons[converted] = true;
                        }
                        break;
                    }
                    case SDL_KEYUP:               // Key released
                    {
                        int converted = to_wiInput(event.key.keysym.scancode);
                        if (converted >= 0) {
                            keyboard.buttons[converted] = false;
                        }
                        break;
                    }
                    case SDL_TEXTEDITING:         // Keyboard text editing (composition)
                    case SDL_TEXTINPUT:           // Keyboard text input
                    case SDL_KEYMAPCHANGED:       // Keymap changed due to a system event such as an
                        //     input language or keyboard layout change.
                        break;


                        // mouse events
                    case SDL_MOUSEMOTION:          // Mouse moved
                        mouse.position.x = event.motion.x;
                        mouse.position.y = event.motion.y;
                        mouse.delta_position.x += event.motion.xrel;
                        mouse.delta_position.y += event.motion.yrel;
                        // TODO you can update mouse buttons by reading event.motion.state
                        mouse.left_button_press = event.motion.state & SDL_BUTTON_LMASK;
                        mouse.right_button_press = event.motion.state & SDL_BUTTON_RMASK;
                        mouse.middle_button_press = event.motion.state & SDL_BUTTON_MMASK;
                        //mouse.x1_button_press = event.motion.state & SDL_BUTTON_X1MASK;
                        //mouse.x2_button_press = event.motion.state & SDL_BUTTON_X2MASK;

                        break;
                    case SDL_MOUSEBUTTONDOWN:      // Mouse button pressed
                    case SDL_MOUSEBUTTONUP:        // Mouse button released
                        // handled at the bottom of this function
                        break;
                    case SDL_MOUSEWHEEL:           // Mouse wheel motion
                    {
                        float delta = static_cast<float>(event.wheel.y);
                        if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                            delta *= -1;
                        }
                        mouse.delta_wheel += delta;
                        break;
                    }


                        // Joystick events
                    case SDL_JOYAXISMOTION:          // Joystick axis motion
                    case SDL_JOYBALLMOTION:          // Joystick trackball motion
                    case SDL_JOYHATMOTION:           // Joystick hat position change
                    case SDL_JOYBUTTONDOWN:          // Joystick button pressed
                    case SDL_JOYBUTTONUP:            // Joystick button released
                    case SDL_JOYDEVICEADDED:         // A new joystick has been inserted into the system
                    case SDL_JOYDEVICEREMOVED:       // An opened joystick has been removed
                        break;


                        // Game controller events
                    case SDL_CONTROLLERAXISMOTION:          // Game controller axis motion
                    case SDL_CONTROLLERBUTTONDOWN:          // Game controller button pressed
                    case SDL_CONTROLLERBUTTONUP:            // Game controller button released
                    case SDL_CONTROLLERDEVICEADDED:         // A new Game controller has been inserted into the system
                    case SDL_CONTROLLERDEVICEREMOVED:       // An opened Game controller has been removed
                    case SDL_CONTROLLERDEVICEREMAPPED:      // The controller mapping was updated
                        break;


                        // Touch events
                    case SDL_FINGERDOWN:
                    case SDL_FINGERUP:
                    case SDL_FINGERMOTION:
                        break;


                        // Gesture events
                    case SDL_DOLLARGESTURE:
                    case SDL_DOLLARRECORD:
                    case SDL_MULTIGESTURE:
                        break;
                }
            }
        }

        // asking directly some data instead of events
        int x,y;
        uint32_t mouse_buttons_state = SDL_GetMouseState(&x, &y);
        mouse.position.x = x;
        mouse.position.y = y;
        mouse.left_button_press = mouse_buttons_state & SDL_BUTTON_LMASK;
        mouse.right_button_press = mouse_buttons_state & SDL_BUTTON_RMASK;
        mouse.middle_button_press = mouse_buttons_state & SDL_BUTTON_MMASK;
    }

    void GetKeyboardState(wiInput::KeyboardState* state) {
        *state = keyboard;
    }
    void GetMouseState(wiInput::MouseState* state) {
        *state = mouse;
    }
    int GetMaxControllerCount() { return 0; }
    bool GetControllerState(wiInput::ControllerState* state, int index) { return false; }
    void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index) {}


    int to_wiInput(const SDL_Scancode &key) {
        if (key < arraysize(keyboard.buttons))
        {
            return key;
        }
        return -1;
    }
}

#else
namespace wiSDLInput
{
    void Initialize() {}
    void Update() {}
    void GetKeyboardState(wiInput::KeyboardState* state) {}
    void GetMouseState(wiInput::MouseState* state) {}
    int GetMaxControllerCount() { return 0; }
    bool GetControllerState(wiInput::ControllerState* state, int index) { return false; }
    void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index) {}
}
#endif // _WIN32