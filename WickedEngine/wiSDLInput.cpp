#include "wiSDLInput.h"

#ifdef SDL2

#include "CommonInclude.h"
#include "wiBacklog.h"
#include "wiInput.h"
#include "wiVector.h"

#include <SDL2/SDL.h>
#include <SDL_events.h>
#include <SDL_gamecontroller.h>
#include <SDL_joystick.h>
#include <SDL_stdinc.h>
#include <iostream>

namespace wi::input::sdlinput
{
    wi::input::KeyboardState keyboard;
    wi::input::MouseState mouse;

    struct Internal_ControllerState
    {
        Sint32 portID;
        SDL_JoystickID internalID;
        SDL_GameController* controller;
        wi::input::ControllerState state;
    };
    wi::vector<Internal_ControllerState> controllers;
    int numSticks = 0;

    int to_wicked(const SDL_Scancode &key);
    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed);

    void AddController(Sint32 id);
    void RemoveController(Sint32 id);
    void RemoveController();

    void Initialize() {
        if(!SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt")){
            wi::backlog::post("[SDL Input] No controller config loaded, add gamecontrollerdb.txt file next to the executable or download it from https://github.com/gabomdq/SDL_GameControllerDB");
        }
        
        // Attempt to add joysticks at initialization
        numSticks = SDL_NumJoysticks();
        if(numSticks > 0){
            for(int i = 0; i<numSticks; i++){
                AddController(i);
            }
        }
    }

    void Update()
    {
        auto saved_x = mouse.position.x;
        auto saved_y = mouse.position.y;

        // update keyboard and mouse to the latest value (in case of other input systems I suppose)
//        keyboard = wi::input::KeyboardState();
//        mouse = wi::input::MouseState();
//        for (auto& internal_controller : controllers)
//        {
//            internal_controller.state = wi::input::ControllerState();
//        }
        // mouse.position.x = saved_x;
        // mouse.position.y = saved_y;
        mouse.delta_wheel = 0;

        //Attempt to add-remove controller by checking joystick numbers, works reliably
        if(SDL_NumJoysticks() > numSticks){
            numSticks = SDL_NumJoysticks();
            for(int i = 0; i<numSticks; i++){
                if(controllers.empty()){
                    AddController(i);
                }
                if(i < controllers.size()){
                    if(!controllers[i].controller){
                        AddController(i);
                    }
                }
            }
        }else if(SDL_NumJoysticks() < numSticks){
            numSticks = SDL_NumJoysticks();
            RemoveController();
        }

        wi::vector<SDL_Event> events(1000);

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
                        int converted = to_wicked(event.key.keysym.scancode);
                        if (converted >= 0) {
                            keyboard.buttons[converted] = true;
                        }
                        break;
                    }
                    case SDL_KEYUP:               // Key released
                    {
                        int converted = to_wicked(event.key.keysym.scancode);
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
        mouse.delta_position.x = x - saved_x;
        mouse.delta_position.y = y - saved_y;
        mouse.left_button_press = mouse_buttons_state & SDL_BUTTON_LMASK;
        mouse.right_button_press = mouse_buttons_state & SDL_BUTTON_RMASK;
        mouse.middle_button_press = mouse_buttons_state & SDL_BUTTON_MMASK;

        //Get controller axis and button values here instead
        for(auto& controller : controllers){
            if(controller.controller){
                //Get controller buttons
                for(int btn=SDL_CONTROLLER_BUTTON_INVALID; btn<SDL_CONTROLLER_BUTTON_MAX; btn++){
                    controller_to_wicked(&controller.state.buttons,
                        btn,
                        (bool)SDL_GameControllerGetButton(controller.controller, (SDL_GameControllerButton)btn));
                }

                //Get controller axes
                float rawLx = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
                float rawLy = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;
                float rawRx = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;
                float rawRy = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f;
                float rawLT = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f;
                float rawRT = SDL_GameControllerGetAxis(controller.controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f;

                controller.state.thumbstick_L.x = (rawLx < -0.01 || rawLx > 0.01) ? rawLx : 0;
                controller.state.thumbstick_L.y = (rawLy < -0.01 || rawLy > 0.01) ? rawLy : 0;
                controller.state.thumbstick_R.x = (rawRx < -0.01 || rawRx > 0.01) ? rawRx : 0;
                controller.state.thumbstick_R.y = (rawRy < -0.01 || rawRy > 0.01) ? rawRy : 0;
                controller.state.trigger_L = (rawLT > 0.01) ? rawLT : 0;
                controller.state.trigger_R = (rawRT > 0.01) ? rawRT : 0;
            }
        }
    }

    void GetKeyboardState(wi::input::KeyboardState* state) {
        *state = keyboard;
    }
    void GetMouseState(wi::input::MouseState* state) {
        *state = mouse;
    }
    int GetMaxControllerCount() { return numSticks; }
    bool GetControllerState(wi::input::ControllerState* state, int index) { 
        if(index < controllers.size()){
            if(controllers[index].controller){
                if(state){
                    *state = controllers[index].state;
                }
                return true;
            }
        }
        return false; 
    }
    void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {}


    int to_wicked(const SDL_Scancode &key) {
        if (key < arraysize(keyboard.buttons))
        {
            return key;
        }
        return -1;
    }

    void controller_to_wicked(uint32_t *current, Uint8 button, bool pressed){
        uint32_t btnenum;
        switch(button){
            case SDL_CONTROLLER_BUTTON_DPAD_UP: btnenum = wi::input::GAMEPAD_BUTTON_UP; break;
            case SDL_CONTROLLER_BUTTON_DPAD_LEFT: btnenum = wi::input::GAMEPAD_BUTTON_LEFT; break;
            case SDL_CONTROLLER_BUTTON_DPAD_DOWN: btnenum = wi::input::GAMEPAD_BUTTON_DOWN; break;
            case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: btnenum = wi::input::GAMEPAD_BUTTON_RIGHT; break;
            case SDL_CONTROLLER_BUTTON_X: btnenum = wi::input::GAMEPAD_BUTTON_1; break;
            case SDL_CONTROLLER_BUTTON_A: btnenum = wi::input::GAMEPAD_BUTTON_2; break;
            case SDL_CONTROLLER_BUTTON_B: btnenum = wi::input::GAMEPAD_BUTTON_3; break;
            case SDL_CONTROLLER_BUTTON_Y: btnenum = wi::input::GAMEPAD_BUTTON_4; break;
            case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_5; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: btnenum = wi::input::GAMEPAD_BUTTON_6; break;
            case SDL_CONTROLLER_BUTTON_LEFTSTICK: btnenum = wi::input::GAMEPAD_BUTTON_7; break;
            case SDL_CONTROLLER_BUTTON_RIGHTSTICK: btnenum = wi::input::GAMEPAD_BUTTON_8; break;
            case SDL_CONTROLLER_BUTTON_BACK: btnenum = wi::input::GAMEPAD_BUTTON_9; break;
            case SDL_CONTROLLER_BUTTON_START: btnenum = wi::input::GAMEPAD_BUTTON_10; break;
        }
        btnenum = 1 << (btnenum - wi::input::GAMEPAD_RANGE_START - 1);
        if(pressed){
            *current |= btnenum;
        }else{
            *current &= ~btnenum;
        }
    }

    void AddController(Sint32 id){
        if(SDL_IsGameController(id)){
            bool opened = false;
            for(auto& controller : controllers){
                if(!controller.controller){
                    controller.controller = SDL_GameControllerOpen(id);
                    if(controller.controller){
                        controller.portID = id;
                        controller.internalID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.controller));
                        opened = true;
                    }
                }
            }
            if(!opened){
                auto& controller = controllers.emplace_back();
                controller.controller = SDL_GameControllerOpen(id);
                if(controller.controller){
                    controller.portID = id;
                    controller.internalID = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.controller));
                    opened = true;
                }
            }
        }
    }

    //Attempt to remove controller by checking if controller is attached or not
    void RemoveController(){
        for(auto& controller : controllers){
            if(!SDL_JoystickGetAttached(SDL_GameControllerGetJoystick(controller.controller))){
                SDL_GameControllerClose(controller.controller);
                controller.controller = nullptr;
            }
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
