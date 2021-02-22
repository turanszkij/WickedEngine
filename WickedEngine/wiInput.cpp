#include "wiInput.h"
#include "wiPlatform.h"
#include "wiXInput.h"
#include "wiRawInput.h"
#include "wiSDLInput.h"
#include "wiHelper.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiColor.h"

#include <algorithm>
#include <map>
#include <atomic>
#include <thread>

#ifdef SDL2
#include <SDL2/SDL.h>
#endif // SDL2

#ifdef PLATFORM_UWP
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.Devices.Input.h>
#endif // PLATFORM_UWP

using namespace std;

namespace wiInput
{

#ifdef _WIN32
#ifndef PLATFORM_UWP
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
#else
#define KEY_DOWN(vk_code) ((int)winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().GetAsyncKeyState((winrt::Windows::System::VirtualKey)vk_code) < 0)
#define KEY_TOGGLE(vk_code) (((int)winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread().GetAsyncKeyState((winrt::Windows::System::VirtualKey)vk_code) & 1) != 0)
#endif //PLATFORM_UWP
#else
#define KEY_DOWN(vk_code) 0
#define KEY_TOGGLE(vk_code) 0
#endif // WIN32
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))

	KeyboardState keyboard;
	MouseState mouse;

	const MouseState& GetMouseState() { return mouse; }

	struct Input 
	{
		BUTTON button = BUTTON_NONE;
		int playerIndex = 0;

		bool operator<(const Input other) {
			return (button != other.button || playerIndex != other.playerIndex);
		}
		struct LessComparer {
			bool operator()(Input const& a, Input const& b) const {
				return (a.button < b.button || a.playerIndex < b.playerIndex);
			}
		};
	};
	std::map<Input, uint32_t, Input::LessComparer> inputs;
	std::vector<Touch> touches;

	struct Controller
	{
		enum DeviceType
		{
			DISCONNECTED,
			XINPUT,
			RAWINPUT,
		};
		DeviceType deviceType;
		int deviceIndex;
		ControllerState state;
	};
	std::vector<Controller> controllers;
	std::atomic_bool initialized{ false };

	void Initialize()
	{
		wiRawInput::Initialize();
		wiSDLInput::Initialize();

		wiBackLog::post("wiInput Initialized");
		initialized.store(true);
	}

	void Update()
	{
		if (!initialized.load())
		{
			return;
		}

		auto range = wiProfiler::BeginRangeCPU("Input");

		wiXInput::Update();
		wiRawInput::Update();
		wiSDLInput::Update();

		mouse.delta_wheel = 0;
		mouse.delta_position = XMFLOAT2(0, 0);

#ifdef _WIN32
		wiRawInput::GetMouseState(&mouse); // currently only the relative data can be used from this
		wiRawInput::GetKeyboardState(&keyboard); // it contains pressed buttons as "keyboard/typewriter" like, so no continuous presses

		// apparently checking the mouse here instead of Down() avoids missing the button presses (review!)
        mouse.left_button_press |= KEY_DOWN(VK_LBUTTON);
        mouse.middle_button_press |= KEY_DOWN(VK_MBUTTON);
        mouse.right_button_press |= KEY_DOWN(VK_RBUTTON);

#ifndef PLATFORM_UWP
		// Since raw input doesn't contain absolute mouse position, we get it with regular winapi:
		HWND hWnd = GetActiveWindow();
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(hWnd, &p);
		const float dpiscaling = (float)GetDpiForWindow(hWnd) / 96.0f;
		mouse.position.x = (float)p.x / dpiscaling;
		mouse.position.y = (float)p.y / dpiscaling;
#endif // PLATFORM_UWP

#elif SDL2
		wiSDLInput::GetMouseState(&mouse);
		wiSDLInput::GetKeyboardState(&keyboard);
		//TODO controllers
		//TODO touch
#endif

#ifdef PLATFORM_UWP
		static bool isRegisteredUWP = false;
		if (!isRegisteredUWP)
		{
			isRegisteredUWP = true;
			using namespace winrt::Windows::UI::Core;
			using namespace winrt::Windows::Devices::Input;

			auto window = CoreWindow::GetForCurrentThread();
			window.PointerPressed([](CoreWindow, PointerEventArgs args) {
				auto p = args.CurrentPoint();

				if (p.Properties().IsPrimary())
				{
					mouse.position = XMFLOAT2(p.Position().X, p.Position().Y);
					mouse.left_button_press = p.Properties().IsLeftButtonPressed();
					mouse.middle_button_press = p.Properties().IsMiddleButtonPressed();
					mouse.right_button_press = p.Properties().IsRightButtonPressed();
					mouse.pressure = p.Properties().Pressure();
				}

				Touch touch;
				touch.state = Touch::TOUCHSTATE_PRESSED;
				touch.pos = XMFLOAT2(p.Position().X, p.Position().Y);
				touches.push_back(touch);
			});
			window.PointerReleased([](CoreWindow, PointerEventArgs args) {
				auto p = args.CurrentPoint();

				if (p.Properties().IsPrimary())
				{
					mouse.left_button_press = p.Properties().IsLeftButtonPressed();
					mouse.middle_button_press = p.Properties().IsMiddleButtonPressed();
					mouse.right_button_press = p.Properties().IsRightButtonPressed();
					mouse.pressure = p.Properties().Pressure();
				}

				Touch touch;
				touch.state = Touch::TOUCHSTATE_RELEASED;
				touch.pos = XMFLOAT2(p.Position().X, p.Position().Y);
				touches.push_back(touch);
			});
			window.PointerMoved([](CoreWindow, PointerEventArgs args) {
				auto p = args.CurrentPoint();

				if (p.Properties().IsPrimary())
				{
					mouse.position = XMFLOAT2(p.Position().X, p.Position().Y);
					mouse.pressure = p.Properties().Pressure();
				}

				Touch touch;
				touch.state = Touch::TOUCHSTATE_MOVED;
				touch.pos = XMFLOAT2(p.Position().X, p.Position().Y);
				touches.push_back(touch);
			});
			window.PointerWheelChanged([](CoreWindow, PointerEventArgs args) {
				auto p = args.CurrentPoint();

				if (p.Properties().IsPrimary())
				{
					mouse.delta_wheel += (float)p.Properties().MouseWheelDelta() / WHEEL_DELTA;
				}

				Touch touch;
				touch.state = Touch::TOUCHSTATE_RELEASED;
				touch.pos = XMFLOAT2(p.Position().X, p.Position().Y);
				touches.push_back(touch);
			});
			MouseDevice::GetForCurrentView().MouseMoved([](MouseDevice, MouseEventArgs args) {
				mouse.delta_position.x += float(args.MouseDelta().X);
				mouse.delta_position.y += float(args.MouseDelta().Y);
			});
		}
#endif

		// Check if low-level XINPUT controller is not registered for playerindex slot and register:
		for (int i = 0; i < wiXInput::GetMaxControllerCount(); ++i)
		{
			if (wiXInput::GetControllerState(nullptr, i))
			{
				int slot = -1;
				for (int j = 0; j < (int)controllers.size(); ++j)
				{
					if (slot < 0 && controllers[j].deviceType == Controller::DISCONNECTED)
					{
						// take the first disconnected slot
						slot = j;
					}
					if (controllers[j].deviceType == Controller::XINPUT && controllers[j].deviceIndex == i)
					{
						// it is already registered to this slot
						slot = j;
						break;
					}
				}
				if (slot == -1)
				{
					// no disconnected slot was found, and it was not registered
					slot = (int)controllers.size();
					controllers.emplace_back();
				}
				controllers[slot].deviceType = Controller::XINPUT;
				controllers[slot].deviceIndex = i;
			}
		}

		// Check if low-level RAWINPUT controller is not registered for playerindex slot and register:
		for (int i = 0; i < wiRawInput::GetMaxControllerCount(); ++i)
		{
			if (wiRawInput::GetControllerState(nullptr, i))
			{
				int slot = -1;
				for (int j = 0; j < (int)controllers.size(); ++j)
				{
					if (slot < 0 && controllers[j].deviceType == Controller::DISCONNECTED)
					{
						// take the first disconnected slot
						slot = j;
					}
					if (controllers[j].deviceType == Controller::RAWINPUT && controllers[j].deviceIndex == i)
					{
						// it is already registered to this slot
						slot = j;
						break;
					}
				}
				if (slot == -1)
				{
					// no disconnected slot was found, and it was not registered
					slot = (int)controllers.size();
					controllers.emplace_back();
				}
				controllers[slot].deviceType = Controller::RAWINPUT;
				controllers[slot].deviceIndex = i;
			}
		}

		// Read low-level controllers:
		for (auto& controller : controllers)
		{
			bool connected = false;
			switch (controller.deviceType)
			{
			case Controller::XINPUT:
				connected = wiXInput::GetControllerState(&controller.state, controller.deviceIndex);
				break;
			case Controller::RAWINPUT:
				connected = wiRawInput::GetControllerState(&controller.state, controller.deviceIndex);
				break;
			}

			if (!connected)
			{
				controller.deviceType = Controller::DISCONNECTED;
			}
		}

		for (auto iter = inputs.begin(); iter != inputs.end();)
		{
			BUTTON button = iter->first.button;
			int playerIndex = iter->first.playerIndex;

			bool todelete = false;

			if (Down(button, playerIndex))
			{
				iter->second++;
			}
			else
			{
				todelete = true;
			}

			if (todelete)
			{
				inputs.erase(iter++);
			}
			else
			{
				++iter;
			}
		}

		touches.clear();

		wiProfiler::EndRange(range);
	}

	bool Down(BUTTON button, int playerindex)
	{
		if (!initialized.load())
		{
			return false;
		}

		if(button > GAMEPAD_RANGE_START)
		{
			if (playerindex < (int)controllers.size())
			{
				const ControllerState& state = controllers[playerindex].state;

				switch (button)
				{
				case GAMEPAD_BUTTON_UP: return state.buttons & (1 << (GAMEPAD_BUTTON_UP - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_LEFT: return state.buttons & (1 << (GAMEPAD_BUTTON_LEFT - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_DOWN: return state.buttons & (1 << (GAMEPAD_BUTTON_DOWN - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_RIGHT: return state.buttons & (1 << (GAMEPAD_BUTTON_RIGHT - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_1: return state.buttons & (1 << (GAMEPAD_BUTTON_1 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_2: return state.buttons & (1 << (GAMEPAD_BUTTON_2 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_3: return state.buttons & (1 << (GAMEPAD_BUTTON_3 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_4: return state.buttons & (1 << (GAMEPAD_BUTTON_4 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_5: return state.buttons & (1 << (GAMEPAD_BUTTON_5 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_6: return state.buttons & (1 << (GAMEPAD_BUTTON_6 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_7: return state.buttons & (1 << (GAMEPAD_BUTTON_7 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_8: return state.buttons & (1 << (GAMEPAD_BUTTON_8 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_9: return state.buttons & (1 << (GAMEPAD_BUTTON_9 - GAMEPAD_RANGE_START - 1));
				case GAMEPAD_BUTTON_10: return state.buttons & (1 << (GAMEPAD_BUTTON_10 - GAMEPAD_RANGE_START - 1));
				}

			}
		}
		else if (playerindex == 0) // keyboard or mouse
		{
			uint8_t keycode = (uint8_t)button;

			switch (button)
			{
			case wiInput::MOUSE_BUTTON_LEFT:
				if (mouse.left_button_press) 
					return true;
				return false;
				break;
			case wiInput::MOUSE_BUTTON_RIGHT:
				if (mouse.right_button_press) 
					return true;
				return false;
				break;
			case wiInput::MOUSE_BUTTON_MIDDLE:
				if (mouse.middle_button_press) 
					return true;
				return false;
				break;
#ifdef _WIN32
			case wiInput::KEYBOARD_BUTTON_UP:
				keycode = VK_UP;
				break;
			case wiInput::KEYBOARD_BUTTON_DOWN:
				keycode = VK_DOWN;
				break;
			case wiInput::KEYBOARD_BUTTON_LEFT:
				keycode = VK_LEFT;
				break;
			case wiInput::KEYBOARD_BUTTON_RIGHT:
				keycode = VK_RIGHT;
				break;
			case wiInput::KEYBOARD_BUTTON_SPACE:
				keycode = VK_SPACE;
				break;
			case wiInput::KEYBOARD_BUTTON_RSHIFT:
				keycode = VK_RSHIFT;
				break;
			case wiInput::KEYBOARD_BUTTON_LSHIFT:
				keycode = VK_LSHIFT;
				break;
			case wiInput::KEYBOARD_BUTTON_F1: 
				keycode = VK_F1;
				break;
			case wiInput::KEYBOARD_BUTTON_F2:
				keycode = VK_F2;
				break;
			case wiInput::KEYBOARD_BUTTON_F3:
				keycode = VK_F3;
				break;
			case wiInput::KEYBOARD_BUTTON_F4:
				keycode = VK_F4;
				break;
			case wiInput::KEYBOARD_BUTTON_F5:
				keycode = VK_F5;
				break;
			case wiInput::KEYBOARD_BUTTON_F6:
				keycode = VK_F6;
				break;
			case wiInput::KEYBOARD_BUTTON_F7:
				keycode = VK_F7;
				break;
			case wiInput::KEYBOARD_BUTTON_F8:
				keycode = VK_F8;
				break;
			case wiInput::KEYBOARD_BUTTON_F9:
				keycode = VK_F9;
				break;
			case wiInput::KEYBOARD_BUTTON_F10:
				keycode = VK_F10;
				break;
			case wiInput::KEYBOARD_BUTTON_F11:
				keycode = VK_F11;
				break;
			case wiInput::KEYBOARD_BUTTON_F12:
				keycode = VK_F12;
				break;
			case wiInput::KEYBOARD_BUTTON_ENTER:
				keycode = VK_RETURN;
				break;
			case wiInput::KEYBOARD_BUTTON_ESCAPE:
				keycode = VK_ESCAPE;
				break;
			case wiInput::KEYBOARD_BUTTON_HOME:
				keycode = VK_HOME;
				break;
			case wiInput::KEYBOARD_BUTTON_LCONTROL:
				keycode = VK_LCONTROL;
				break;
			case wiInput::KEYBOARD_BUTTON_RCONTROL:
				keycode = VK_RCONTROL;
				break;
			case wiInput::KEYBOARD_BUTTON_DELETE:
				keycode = VK_DELETE;
				break;
			case wiInput::KEYBOARD_BUTTON_BACKSPACE:
				keycode = VK_BACK;
				break;
			case wiInput::KEYBOARD_BUTTON_PAGEDOWN:
				keycode = VK_NEXT;
				break;
			case wiInput::KEYBOARD_BUTTON_PAGEUP:
				keycode = VK_PRIOR;
				break;
#elif SDL2
                case wiInput::KEYBOARD_BUTTON_UP:
                    keycode = SDL_SCANCODE_UP;
                    break;
                case wiInput::KEYBOARD_BUTTON_DOWN:
                    keycode = SDL_SCANCODE_DOWN;
                    break;
                case wiInput::KEYBOARD_BUTTON_LEFT:
                    keycode = SDL_SCANCODE_LEFT;
                    break;
                case wiInput::KEYBOARD_BUTTON_RIGHT:
                    keycode = SDL_SCANCODE_RIGHT;
                    break;
                case wiInput::KEYBOARD_BUTTON_SPACE:
                    keycode = SDL_SCANCODE_SPACE;
                    break;
                case wiInput::KEYBOARD_BUTTON_RSHIFT:
                    keycode = SDL_SCANCODE_RSHIFT;
                    break;
                case wiInput::KEYBOARD_BUTTON_LSHIFT:
                    keycode = SDL_SCANCODE_LSHIFT;
                    break;
                case wiInput::KEYBOARD_BUTTON_F1:
                    keycode = SDL_SCANCODE_F1;
                    break;
                case wiInput::KEYBOARD_BUTTON_F2:
                    keycode = SDL_SCANCODE_F2;
                    break;
                case wiInput::KEYBOARD_BUTTON_F3:
                    keycode = SDL_SCANCODE_F3;
                    break;
                case wiInput::KEYBOARD_BUTTON_F4:
                    keycode = SDL_SCANCODE_F4;
                    break;
                case wiInput::KEYBOARD_BUTTON_F5:
                    keycode = SDL_SCANCODE_F5;
                    break;
                case wiInput::KEYBOARD_BUTTON_F6:
                    keycode = SDL_SCANCODE_F6;
                    break;
                case wiInput::KEYBOARD_BUTTON_F7:
                    keycode = SDL_SCANCODE_F7;
                    break;
                case wiInput::KEYBOARD_BUTTON_F8:
                    keycode = SDL_SCANCODE_F8;
                    break;
                case wiInput::KEYBOARD_BUTTON_F9:
                    keycode = SDL_SCANCODE_F9;
                    break;
                case wiInput::KEYBOARD_BUTTON_F10:
                    keycode = SDL_SCANCODE_F10;
                    break;
                case wiInput::KEYBOARD_BUTTON_F11:
                    keycode = SDL_SCANCODE_F11;
                    break;
                case wiInput::KEYBOARD_BUTTON_F12:
                    keycode = SDL_SCANCODE_F12;
                    break;
                case wiInput::KEYBOARD_BUTTON_ENTER:
                    keycode = SDL_SCANCODE_RETURN;
                    break;
                case wiInput::KEYBOARD_BUTTON_ESCAPE:
                    keycode = SDL_SCANCODE_ESCAPE;
                    break;
                case wiInput::KEYBOARD_BUTTON_HOME:
                    keycode = SDL_SCANCODE_HOME;
                    break;
                case wiInput::KEYBOARD_BUTTON_LCONTROL:
                    keycode = SDL_SCANCODE_LCTRL;
                    break;
                case wiInput::KEYBOARD_BUTTON_RCONTROL:
                    keycode = SDL_SCANCODE_RCTRL;
                    break;
                case wiInput::KEYBOARD_BUTTON_DELETE:
                    keycode = SDL_SCANCODE_DELETE;
                    break;
                case wiInput::KEYBOARD_BUTTON_BACKSPACE:
                    keycode = SDL_SCANCODE_BACKSPACE;
                    break;
                case wiInput::KEYBOARD_BUTTON_PAGEDOWN:
                    keycode = SDL_SCANCODE_PAGEDOWN;
                    break;
                case wiInput::KEYBOARD_BUTTON_PAGEUP:
                    keycode = SDL_SCANCODE_PAGEUP;
                    break;
				//Translating engine's keycode mapping (which comes from Win32 i presume) to the matching SDL2 keycode
				//Win32 Link: https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
				//SDL2 Link: https://wiki.libsdl.org/SDL_Keycode
				//Numbers start from enum 48 to 57, 58 to 64 are undefined, 65 to 90 are alphabets.
				//Usage: the same as win32 platform, which is (for example) wiInput::Down((wiInput::BUTTON)'A' (referenced from Editor.cpp for the engine's editor program)
				case 48:
					keycode = SDL_SCANCODE_0;
					break;
				case 49:
					keycode = SDL_SCANCODE_1;
					break;
				case 50:
					keycode = SDL_SCANCODE_2;
					break;
				case 51:
					keycode = SDL_SCANCODE_3;
					break;
				case 52:
					keycode = SDL_SCANCODE_4;
					break;
				case 53:
					keycode = SDL_SCANCODE_5;
					break;
				case 54:
					keycode = SDL_SCANCODE_6;
					break;
				case 55:
					keycode = SDL_SCANCODE_7;
					break;
				case 56:
					keycode = SDL_SCANCODE_8;
					break;
				case 57:
					keycode = SDL_SCANCODE_9;
					break;
				case 65:
					keycode = SDL_SCANCODE_A;
					break;
				case 66:
					keycode = SDL_SCANCODE_B;
					break;
				case 67:
					keycode = SDL_SCANCODE_C;
					break;
				case 68:
					keycode = SDL_SCANCODE_D;
					break;
				case 69:
					keycode = SDL_SCANCODE_E;
					break;
				case 70:
					keycode = SDL_SCANCODE_F;
					break;
				case 71:
					keycode = SDL_SCANCODE_G;
					break;
				case 72:
					keycode = SDL_SCANCODE_H;
					break;
				case 73:
					keycode = SDL_SCANCODE_I;
					break;
				case 74:
					keycode = SDL_SCANCODE_J;
					break;
				case 75:
					keycode = SDL_SCANCODE_K;
					break;
				case 76:
					keycode = SDL_SCANCODE_L;
					break;
				case 77:
					keycode = SDL_SCANCODE_M;
					break;
				case 78:
					keycode = SDL_SCANCODE_N;
					break;
				case 79:
					keycode = SDL_SCANCODE_O;
					break;
				case 80:
					keycode = SDL_SCANCODE_P;
					break;
				case 81:
					keycode = SDL_SCANCODE_Q;
					break;
				case 82:
					keycode = SDL_SCANCODE_R;
					break;
				case 83:
					keycode = SDL_SCANCODE_S;
					break;
				case 84:
					keycode = SDL_SCANCODE_T;
					break;
				case 85:
					keycode = SDL_SCANCODE_U;
					break;
				case 86:
					keycode = SDL_SCANCODE_V;
					break;
				case 87:
					keycode = SDL_SCANCODE_W;
					break;
				case 88:
					keycode = SDL_SCANCODE_X;
					break;
				case 89:
					keycode = SDL_SCANCODE_Y;
					break;
				case 90:
					keycode = SDL_SCANCODE_Z;
					break;
				
#endif // _WIN32
			}

#ifdef _WIN32
			return KEY_DOWN(keycode) || KEY_TOGGLE(keycode);
#elif SDL2
			int numkeys;
			const uint8_t *state = SDL_GetKeyboardState(&numkeys);
			return state[keycode] == 1;
#else
#error KEYBOARD INPUT NOT SUPPORTED
#endif
		}

		return false;
	}
	bool Press(BUTTON button, int playerindex)
	{
		if (!Down(button, playerindex))
			return false;

		Input input;
		input.button = button;
		input.playerIndex = playerindex;
		auto iter = inputs.find(input);
		if (iter == inputs.end())
		{
			inputs.insert(make_pair(input, 0));
			return true;
		}
		if (iter->second == 0)
		{
			return true;
		}
		return false;
	}
	bool Hold(BUTTON button, uint32_t frames, bool continuous, int playerIndex)
	{
		if (!Down(button, playerIndex))
			return false;

		Input input;
		input.button = button;
		input.playerIndex = playerIndex;
		auto iter = inputs.find(input);
		if (iter == inputs.end())
		{
			inputs.insert(make_pair(input, 0));
			return false;
		}
		else if ((!continuous && iter->second == frames) || (continuous && iter->second >= frames))
		{
			return true;
		}
		return false;
	}
	XMFLOAT4 GetPointer()
	{
		return XMFLOAT4(mouse.position.x, mouse.position.y, mouse.delta_wheel, mouse.pressure);
	}
	void SetPointer(const XMFLOAT4& props)
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		HWND hWnd = GetActiveWindow();
		const float dpiscaling = (float)GetDpiForWindow(hWnd) / 96.0f;
		POINT p;
		p.x = (LONG)(props.x * dpiscaling);
		p.y = (LONG)(props.y * dpiscaling);
		ClientToScreen(hWnd, &p);
		SetCursorPos(p.x, p.y);
#else
		auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
		auto& bounds = window.Bounds();
		window.PointerPosition(winrt::Windows::Foundation::Point(props.x + bounds.X, props.y + bounds.Y));
#endif
#endif // _WIN32
	}
	void HidePointer(bool value)
	{
#ifdef _WIN32
#ifndef PLATFORM_UWP
		if (value)
		{
			while (ShowCursor(false) >= 0) {};
		}
		else
		{
			while (ShowCursor(true) < 0) {};
		}
#else
		auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
		static auto cursor = window.PointerCursor();
		if (value)
		{
			window.PointerCursor(nullptr);
		}
		else
		{
			window.PointerCursor(cursor);
		}
#endif
#elif SDL2
		SDL_ShowCursor(value ? SDL_DISABLE : SDL_ENABLE);
#endif // _WIN32
	}

	XMFLOAT4 GetAnalog(GAMEPAD_ANALOG analog, int playerindex)
	{
		if (playerindex < (int)controllers.size())
		{
			const ControllerState& state = controllers[playerindex].state;

			switch (analog)
			{
			case GAMEPAD_ANALOG_THUMBSTICK_L: return XMFLOAT4(state.thumbstick_L.x, state.thumbstick_L.y, 0, 0);
			case GAMEPAD_ANALOG_THUMBSTICK_R: return XMFLOAT4(state.thumbstick_R.x, state.thumbstick_R.y, 0, 0);
			case GAMEPAD_ANALOG_TRIGGER_L: return XMFLOAT4(state.trigger_L, 0, 0, 0);
			case GAMEPAD_ANALOG_TRIGGER_R: return XMFLOAT4(state.trigger_R, 0, 0, 0);
			}
		}

		return XMFLOAT4(0, 0, 0, 0);
	}

	void SetControllerFeedback(const ControllerFeedback& data, int playerindex)
	{
		if (playerindex < (int)controllers.size())
		{
			const Controller& controller = controllers[playerindex];

			if (controller.deviceType == Controller::XINPUT)
			{
				wiXInput::SetControllerFeedback(data, controller.deviceIndex);
			}
			else if (controller.deviceType == Controller::RAWINPUT)
			{
				wiRawInput::SetControllerFeedback(data, controller.deviceIndex);
			}
		}
	}

	const std::vector<Touch>& GetTouches()
	{
		return touches;
	}

}
