#include "wiSDLInput.h"

#ifdef SDL3
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>

#include "wiUnorderedMap.h"
#include "wiBacklog.h"

namespace wi::input::sdlinput
{
    wi::input::KeyboardState keyboard;
    wi::input::MouseState mouse;

    struct Internal_ControllerState
    {
        Sint32 portID;
        SDL_JoystickID internalID;
        SDL_Gamepad* controller;
        Uint16 rumble_l, rumble_r = 0;
        wi::input::ControllerState state;
    };
    wi::vector<Internal_ControllerState> controllers;
    wi::unordered_map<SDL_JoystickID, size_t> controller_mapped;

    wi::vector<SDL_Event> events;

    int to_wicked(const SDL_Scancode &key, const SDL_Keycode &keyk);
    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed);
    void controller_map_rebuild();

    void AddController(Sint32 id);
    void RemoveController(Sint32 id);
    void RemoveController();

    void Initialize() {
        if(!SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt")){
            wi::backlog::post("[SDL Input] No controller config loaded, add gamecontrollerdb.txt file next to the executable or download it from https://github.com/gabomdq/SDL_GameControllerDB");
        }
    }

    void ProcessEvent(const SDL_Event &event){
        events.push_back(event);
    }

    void Update()
    {
        mouse.delta_wheel = 0; // Do not accumulate mouse wheel motion delta
        mouse.delta_position = XMFLOAT2(0, 0); // Do not accumulate mouse position delta

        for(auto& event : events){
            switch(event.type){
                // Keyboard events
                case SDL_EVENT_KEY_DOWN:             // Key pressed
                {
                    int converted = to_wicked(event.key.scancode, event.key.key);
                    if (converted >= 0) {
                        keyboard.buttons[converted] = true;
                    }
                    break;
                }
                case SDL_EVENT_KEY_UP:               // Key released
                {
                    int converted = to_wicked(event.key.scancode, event.key.key);
                    if (converted >= 0) {
                        keyboard.buttons[converted] = false;
                    }
                }
                case SDL_EVENT_TEXT_EDITING:         // Keyboard text editing (composition)
                case SDL_EVENT_TEXT_INPUT:           // Keyboard text input
                case SDL_EVENT_KEYMAP_CHANGED:       // Keymap changed due to a system event such as an
                    //     input language or keyboard layout change.
                    break;


                    // mouse events
                case SDL_EVENT_MOUSE_MOTION:          // Mouse moved
                    mouse.position.x = event.motion.x;
                    mouse.position.y = event.motion.y;
                    mouse.delta_position.x += event.motion.xrel;
                    mouse.delta_position.y += event.motion.yrel;
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:      // Mouse button pressed
                    switch(event.button.button){
                        case SDL_BUTTON_LEFT:
                            mouse.left_button_press = true;
                            break;
                        case SDL_BUTTON_RIGHT:
                            mouse.right_button_press = true;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            mouse.middle_button_press = true;
                            break;
                    }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP:        // Mouse button released
                    switch(event.button.button){
                        case SDL_BUTTON_LEFT:
                            mouse.left_button_press = false;
                            break;
                        case SDL_BUTTON_RIGHT:
                            mouse.right_button_press = false;
                            break;
                        case SDL_BUTTON_MIDDLE:
                            mouse.middle_button_press = false;
                            break;
                    }
                    break;
                case SDL_EVENT_MOUSE_WHEEL:           // Mouse wheel motion
                {
                    float delta = static_cast<float>(event.wheel.y);
                    if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
                        delta *= -1;
                    }
                    mouse.delta_wheel += delta;
                    break;
                }


                    // Joystick events
                case SDL_EVENT_JOYSTICK_AXIS_MOTION:          // Joystick axis motion
                case SDL_EVENT_JOYSTICK_BALL_MOTION:          // Joystick trackball motion
                case SDL_EVENT_JOYSTICK_HAT_MOTION:           // Joystick hat position change
                case SDL_EVENT_JOYSTICK_BUTTON_DOWN:          // Joystick button pressed
                case SDL_EVENT_JOYSTICK_BUTTON_UP:            // Joystick button released
                case SDL_EVENT_JOYSTICK_ADDED:         // A new joystick has been inserted into the system
                case SDL_EVENT_JOYSTICK_REMOVED:       // An opened joystick has been removed
                    break;


                    // Game controller events
                case SDL_EVENT_GAMEPAD_AXIS_MOTION:          // Game controller axis motion
                {
                    auto controller_get = controller_mapped.find(event.gaxis.which);
                    if(controller_get != controller_mapped.end()){
                        float raw = event.gaxis.value / 32767.0f;
                        const float deadzone = 0.2;
                        float deadzoned = (raw < -deadzone || raw > deadzone) ? raw : 0;
                        switch(event.gaxis.axis){
                            case SDL_GAMEPAD_AXIS_LEFTX:
                                controllers[controller_get->second].state.thumbstick_L.x = deadzoned;
                                break;
                            case SDL_GAMEPAD_AXIS_LEFTY:
                                controllers[controller_get->second].state.thumbstick_L.y = -deadzoned;
                                break;
                            case SDL_GAMEPAD_AXIS_RIGHTX:
                                controllers[controller_get->second].state.thumbstick_R.x = deadzoned;
                                break;
                            case SDL_GAMEPAD_AXIS_RIGHTY:
                                controllers[controller_get->second].state.thumbstick_R.y = deadzoned;
                                break;
                            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                                controllers[controller_get->second].state.trigger_L = (deadzoned > 0.f) ? deadzoned : 0;
                                break;
                            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                                controllers[controller_get->second].state.trigger_R = (deadzoned > 0.f) ? deadzoned : 0;
                                break;
                        }
                    }
                    break;
                }
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN:          // Game controller button pressed
                {
                    auto find = controller_mapped.find(event.gbutton.which);
                    if(find != controller_mapped.end()){
                        controller_to_wicked(&controllers[find->second].state.buttons, event.gbutton.button, true);
                    }
                    break;
                }
                case SDL_EVENT_GAMEPAD_BUTTON_UP:            // Game controller button released
                {
                    auto find = controller_mapped.find(event.gbutton.which);
                    if(find != controller_mapped.end()){
                        controller_to_wicked(&controllers[find->second].state.buttons, event.gbutton.button, false);
                    }
                    break;
                }
                case SDL_EVENT_GAMEPAD_ADDED:         // A new Game controller has been inserted into the system
                {
                    auto& controller = controllers.emplace_back();
                    controller.controller = SDL_OpenGamepad(event.cdevice.which);
                    if(controller.controller){
                        controller.portID = event.cdevice.which;
                        controller.internalID = SDL_GetJoystickID(SDL_GetGamepadJoystick(controller.controller));
                    }
                    controller_map_rebuild();
                    break;
                }
                case SDL_EVENT_GAMEPAD_REMOVED:       // An opened Game controller has been removed
                {
                    auto find = controller_mapped.find(event.cdevice.which);
                    if(find != controller_mapped.end()){
                        SDL_CloseGamepad(controllers[find->second].controller);
                        controllers[find->second] = std::move(controllers.back());
                        controllers.pop_back();
                    }
                    controller_map_rebuild();
                    break;
                }
                case SDL_EVENT_GAMEPAD_REMAPPED:      // The controller mapping was updated
                    break;


                    // Touch events
                case SDL_EVENT_FINGER_DOWN:
                case SDL_EVENT_FINGER_UP:
                case SDL_EVENT_FINGER_MOTION:
                    wi::backlog::post("finger!");
                    break;

                default:
                    break;
            }
            // Clone all events for use outside the internal code, e.g. main_SDL3.cpp can benefit from this
            //external_events.push_back(event);
        }

        //Update rumble every call
        for(auto& controller : controllers){
            SDL_RumbleGamepad(
                controller.controller,
                controller.rumble_l,
                controller.rumble_r,
                60); //Buffer at 60ms
        }

        //Flush away stored events
        events.clear();
    }

    int to_wicked(const SDL_Scancode &key, const SDL_Keycode &keyk) {

        // Scancode Conversion Segment

        if(key >= 4 && key <= 29){ // A to Z
            return (key - 4) + CHARACTER_RANGE_START;
        }
        if(key >= 30 && key <= 39){ // 0 to 9
            return (key - 30) + DIGIT_RANGE_START;
        }
        if(key >= 58 && key <= 69){ // F1 to F12
            return (key - 58) + KEYBOARD_BUTTON_F1;
        }
        if(key >= 79 && key <= 82){ // Keyboard directional buttons
            return (82 - key) + KEYBOARD_BUTTON_UP;
        }
        switch(key){ // Individual scancode key conversion
            case SDL_SCANCODE_SPACE:
                return wi::input::KEYBOARD_BUTTON_SPACE;
            case SDL_SCANCODE_LSHIFT:
                return wi::input::KEYBOARD_BUTTON_LSHIFT;
            case SDL_SCANCODE_RSHIFT:
                return wi::input::KEYBOARD_BUTTON_RSHIFT;
            case SDL_SCANCODE_RETURN:
                return wi::input::KEYBOARD_BUTTON_ENTER;
            case SDL_SCANCODE_ESCAPE:
                return wi::input::KEYBOARD_BUTTON_ESCAPE;
            case SDL_SCANCODE_HOME:
                return wi::input::KEYBOARD_BUTTON_HOME;
            case SDL_SCANCODE_RCTRL:
                return wi::input::KEYBOARD_BUTTON_RCONTROL;
            case SDL_SCANCODE_LCTRL:
                return wi::input::KEYBOARD_BUTTON_LCONTROL;
            case SDL_SCANCODE_DELETE:
                return wi::input::KEYBOARD_BUTTON_DELETE;
            case SDL_SCANCODE_BACKSPACE:
                return wi::input::KEYBOARD_BUTTON_BACKSPACE;
            case SDL_SCANCODE_PAGEDOWN:
                return wi::input::KEYBOARD_BUTTON_PAGEDOWN;
            case SDL_SCANCODE_PAGEUP:
                return wi::input::KEYBOARD_BUTTON_PAGEUP;
			case SDL_SCANCODE_KP_0:
				return wi::input::KEYBOARD_BUTTON_NUMPAD0;
			case SDL_SCANCODE_KP_1:
				return wi::input::KEYBOARD_BUTTON_NUMPAD1;
			case SDL_SCANCODE_KP_2:
				return wi::input::KEYBOARD_BUTTON_NUMPAD2;
			case SDL_SCANCODE_KP_3:
				return wi::input::KEYBOARD_BUTTON_NUMPAD3;
			case SDL_SCANCODE_KP_4:
				return wi::input::KEYBOARD_BUTTON_NUMPAD4;
			case SDL_SCANCODE_KP_5:
				return wi::input::KEYBOARD_BUTTON_NUMPAD5;
			case SDL_SCANCODE_KP_6:
				return wi::input::KEYBOARD_BUTTON_NUMPAD6;
			case SDL_SCANCODE_KP_7:
				return wi::input::KEYBOARD_BUTTON_NUMPAD7;
			case SDL_SCANCODE_KP_8:
				return wi::input::KEYBOARD_BUTTON_NUMPAD8;
			case SDL_SCANCODE_KP_9:
				return wi::input::KEYBOARD_BUTTON_NUMPAD9;
			case SDL_SCANCODE_KP_MULTIPLY:
				return wi::input::KEYBOARD_BUTTON_MULTIPLY;
			case SDL_SCANCODE_KP_PLUS:
				return wi::input::KEYBOARD_BUTTON_ADD;
			case SDL_SCANCODE_SEPARATOR:
				return wi::input::KEYBOARD_BUTTON_SEPARATOR;
			case SDL_SCANCODE_KP_MINUS:
				return wi::input::KEYBOARD_BUTTON_SUBTRACT;
			case SDL_SCANCODE_KP_DECIMAL:
				return wi::input::KEYBOARD_BUTTON_DECIMAL;
			case SDL_SCANCODE_KP_DIVIDE:
				return wi::input::KEYBOARD_BUTTON_DIVIDE;
			case SDL_SCANCODE_INSERT:
				return wi::input::KEYBOARD_BUTTON_INSERT;
			case SDL_SCANCODE_TAB:
				return wi::input::KEYBOARD_BUTTON_TAB;
			case SDL_SCANCODE_GRAVE:
    			return wi::input::KEYBOARD_BUTTON_TILDE;
        }


        // Keycode Conversion Segment

        if(keyk >= 91 && keyk <= 126){
            return keyk;
        }

        return -1;

    }

    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed){
        uint32_t btnenum;
        switch(button){
            case SDL_GAMEPAD_BUTTON_DPAD_UP: btnenum = wi::input::GAMEPAD_BUTTON_UP; break;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT: btnenum = wi::input::GAMEPAD_BUTTON_LEFT; break;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN: btnenum = wi::input::GAMEPAD_BUTTON_DOWN; break;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: btnenum = wi::input::GAMEPAD_BUTTON_RIGHT; break;
            case SDL_GAMEPAD_BUTTON_WEST: btnenum = wi::input::GAMEPAD_BUTTON_1; break;
            case SDL_GAMEPAD_BUTTON_SOUTH: btnenum = wi::input::GAMEPAD_BUTTON_2; break;
            case SDL_GAMEPAD_BUTTON_EAST: btnenum = wi::input::GAMEPAD_BUTTON_3; break;
            case SDL_GAMEPAD_BUTTON_NORTH: btnenum = wi::input::GAMEPAD_BUTTON_4; break;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_5; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_6; break;
            case SDL_GAMEPAD_BUTTON_LEFT_STICK: btnenum = wi::input::GAMEPAD_BUTTON_7; break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK: btnenum = wi::input::GAMEPAD_BUTTON_8; break;
            case SDL_GAMEPAD_BUTTON_BACK: btnenum = wi::input::GAMEPAD_BUTTON_9; break;
            case SDL_GAMEPAD_BUTTON_START: btnenum = wi::input::GAMEPAD_BUTTON_10; break;
			default: assert(0); return;
        }
        btnenum = 1 << (btnenum - wi::input::GAMEPAD_RANGE_START - 1);
        if(pressed){
            *current |= btnenum;
        }else{
            *current &= ~btnenum;
        }
    }

    // Rebuild controller mappings for fast array access
    void controller_map_rebuild(){
        controller_mapped.clear();
        for(int index = 0; index < controllers.size(); ++index){
            controller_mapped.insert({controllers[index].internalID, index});
        }
    }

    void GetKeyboardState(wi::input::KeyboardState* state) {
        *state = keyboard;
    }
    void GetMouseState(wi::input::MouseState* state) {
        *state = mouse;
    }

    int GetMaxControllerCount() { return controllers.size(); }
    bool GetControllerState(wi::input::ControllerState* state, int index) {
        if(index < controllers.size()){
            if (state != nullptr)
			{
				*state = controllers[index].state;
			}
			return true;
        }
        return false;
    }
    void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {
        if(index < controllers.size()){
            SDL_SetGamepadLED(
                controllers[index].controller,
                data.led_color.getR(),
                data.led_color.getG(),
                data.led_color.getB());
            controllers[index].rumble_l = (Uint16)floor(data.vibration_left * 0xFFFF);
            controllers[index].rumble_r = (Uint16)floor(data.vibration_right * 0xFFFF);
        }
    }
}
#else
namespace wi::input::sdlinput
{
    void Initialize() {}
    void Update() {}
    void GetKeyboardState(wi::input::KeyboardState* state) {}
    void GetMouseState(wi::input::MouseState* state) {}
    int GetMaxControllerCount() { return 0; }
    bool GetControllerState(wi::input::ControllerState* state, int index) { return false; }
    void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {}
}
#endif // _WIN32
