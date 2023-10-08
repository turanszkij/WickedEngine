#include "wiInput.h"
#include "wiPlatform.h"
#include "wiXInput.h"
#include "wiRawInput.h"
#include "wiSDLInput.h"
#include "wiHelper.h"
#include "wiBacklog.h"
#include "wiProfiler.h"
#include "wiColor.h"
#include "wiTimer.h"

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

#ifdef PLATFORM_PS5
#include "wiInput_PS5.h"
#endif // PLATFORM_PS5

namespace wi::input
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

	wi::platform::window_type window = nullptr;
	wi::Canvas canvas;
	KeyboardState keyboard;
	MouseState mouse;
	Pen pen;
	bool pen_override = false;

	const KeyboardState& GetKeyboardState() { return keyboard; }
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
	std::map<Input, int, Input::LessComparer> inputs; // Input -> down frames (-1 = released)
	wi::vector<Touch> touches;

	struct Controller
	{
		enum DeviceType
		{
			DISCONNECTED,
			XINPUT,
			RAWINPUT,
			SDLINPUT,
			PS5,
		};
		DeviceType deviceType;
		int deviceIndex;
		ControllerState state;
	};
	wi::vector<Controller> controllers;
	std::atomic_bool initialized{ false };

	void Initialize()
	{
		wi::Timer timer;

		wi::input::rawinput::Initialize();
		wi::input::sdlinput::Initialize();

#ifdef PLATFORM_PS5
		wi::input::ps5::Initialize();
#endif // PLATFORM_PS5

		wi::backlog::post("wi::input Initialized (" + std::to_string((int)std::round(timer.elapsed())) + " ms)");
		initialized.store(true);
	}

	void Update(wi::platform::window_type _window, wi::Canvas _canvas)
	{
		window = _window;
		canvas = _canvas;
		if (!initialized.load())
		{
			return;
		}

		auto range = wi::profiler::BeginRangeCPU("Input");

		wi::input::xinput::Update();
		wi::input::rawinput::Update();
		wi::input::sdlinput::Update();

#ifdef PLATFORM_PS5
		wi::input::ps5::Update();
#endif // PLATFORM_PS5

#if defined(_WIN32) && !defined(PLATFORM_XBOX)
		wi::input::rawinput::GetMouseState(&mouse); // currently only the relative data can be used from this
		wi::input::rawinput::GetKeyboardState(&keyboard); 

		// apparently checking the mouse here instead of Down() avoids missing the button presses (review!)
        mouse.left_button_press |= KEY_DOWN(VK_LBUTTON);
        mouse.middle_button_press |= KEY_DOWN(VK_MBUTTON);
        mouse.right_button_press |= KEY_DOWN(VK_RBUTTON);

#ifndef PLATFORM_UWP
		// Since raw input doesn't contain absolute mouse position, we get it with regular winapi:
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(window, &p);
		mouse.position.x = (float)p.x;
		mouse.position.y = (float)p.y;
		mouse.position.x /= canvas.GetDPIScaling();
		mouse.position.y /= canvas.GetDPIScaling();
#endif // PLATFORM_UWP

#elif SDL2
		wi::input::sdlinput::GetMouseState(&mouse);
		mouse.position.x /= canvas.GetDPIScaling();
		mouse.position.y /= canvas.GetDPIScaling();
		wi::input::sdlinput::GetKeyboardState(&keyboard);
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
					// UWP mouse position already has DPI applied, so only apply custom scale factor in here:
					mouse.position.x /= canvas.scaling;
					mouse.position.y /= canvas.scaling;
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
					// UWP mouse position already has DPI applied, so only apply custom scale factor in here:
					mouse.position.x /= canvas.scaling;
					mouse.position.y /= canvas.scaling;
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

		if (pen_override)
		{
			mouse.position = pen.position;
			mouse.left_button_press = pen.pressure > 0;
			mouse.pressure = pen.pressure;
			pen_override = false;
		}

		// Check if low-level XINPUT controller is not registered for playerindex slot and register:
		for (int i = 0; i < wi::input::xinput::GetMaxControllerCount(); ++i)
		{
			if (wi::input::xinput::GetControllerState(nullptr, i))
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
		for (int i = 0; i < wi::input::rawinput::GetMaxControllerCount(); ++i)
		{
			if (wi::input::rawinput::GetControllerState(nullptr, i))
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

		// Check if low-level SDLINPUT controller is not registered for playerindex slot and register:
		for (int i = 0; i < wi::input::sdlinput::GetMaxControllerCount(); ++i)
		{
			if (wi::input::sdlinput::GetControllerState(nullptr, i))
			{
				int slot = -1;
				for (int j = 0; j < (int)controllers.size(); ++j)
				{
					if (slot < 0 && controllers[j].deviceType == Controller::DISCONNECTED)
					{
						// take the first disconnected slot
						slot = j;
					}
					if (controllers[j].deviceType == Controller::SDLINPUT && controllers[j].deviceIndex == i)
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
				controllers[slot].deviceType = Controller::SDLINPUT;
				controllers[slot].deviceIndex = i;
			}
		}

#ifdef PLATFORM_PS5
		// Check if low-level PS5 controller is not registered for playerindex slot and register:
		for (int i = 0; i < wi::input::ps5::GetMaxControllerCount(); ++i)
		{
			if (wi::input::ps5::GetControllerState(nullptr, i))
			{
				int slot = -1;
				for (int j = 0; j < (int)controllers.size(); ++j)
				{
					if (slot < 0 && controllers[j].deviceType == Controller::DISCONNECTED)
					{
						// take the first disconnected slot
						slot = j;
					}
					if (controllers[j].deviceType == Controller::PS5 && controllers[j].deviceIndex == i)
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
				controllers[slot].deviceType = Controller::PS5;
				controllers[slot].deviceIndex = i;
			}
		}
#endif // PLATFORM_PS5

		// Read low-level controllers:
		for (auto& controller : controllers)
		{
			bool connected = false;
			switch (controller.deviceType)
			{
				case Controller::XINPUT:
					connected = wi::input::xinput::GetControllerState(&controller.state, controller.deviceIndex);
					break;
				case Controller::RAWINPUT:
					connected = wi::input::rawinput::GetControllerState(&controller.state, controller.deviceIndex);
					break;
				case Controller::SDLINPUT:
					connected = wi::input::sdlinput::GetControllerState(&controller.state, controller.deviceIndex);
					break;
#ifdef PLATFORM_PS5
				case Controller::PS5:
					connected = wi::input::ps5::GetControllerState(&controller.state, controller.deviceIndex);
					break;
#endif // PLATFORM_PS5
				case Controller::DISCONNECTED:
					connected = false;
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
			else if (iter->second == -1)
			{
				todelete = true;
			}
			else
			{
				iter->second = -1;
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

		wi::profiler::EndRange(range);
	}

	void ClearForNextFrame()
	{
		mouse.delta_wheel = 0;
		mouse.delta_position = XMFLOAT2(0, 0);
		touches.clear();
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
				default: break;
				}

			}
		}
		else if (playerindex == 0) // keyboard or mouse
		{
			uint8_t keycode = (uint8_t)button;

			switch (button)
			{
			case wi::input::MOUSE_BUTTON_LEFT:
				if (mouse.left_button_press) 
					return true;
				return false;
			case wi::input::MOUSE_BUTTON_RIGHT:
				if (mouse.right_button_press) 
					return true;
				return false;
			case wi::input::MOUSE_BUTTON_MIDDLE:
				if (mouse.middle_button_press) 
					return true;
				return false;
#ifdef _WIN32
			case wi::input::KEYBOARD_BUTTON_UP:
				keycode = VK_UP;
				break;
			case wi::input::KEYBOARD_BUTTON_DOWN:
				keycode = VK_DOWN;
				break;
			case wi::input::KEYBOARD_BUTTON_LEFT:
				keycode = VK_LEFT;
				break;
			case wi::input::KEYBOARD_BUTTON_RIGHT:
				keycode = VK_RIGHT;
				break;
			case wi::input::KEYBOARD_BUTTON_SPACE:
				keycode = VK_SPACE;
				break;
			case wi::input::KEYBOARD_BUTTON_RSHIFT:
				keycode = VK_RSHIFT;
				break;
			case wi::input::KEYBOARD_BUTTON_LSHIFT:
				keycode = VK_LSHIFT;
				break;
			case wi::input::KEYBOARD_BUTTON_F1: 
				keycode = VK_F1;
				break;
			case wi::input::KEYBOARD_BUTTON_F2:
				keycode = VK_F2;
				break;
			case wi::input::KEYBOARD_BUTTON_F3:
				keycode = VK_F3;
				break;
			case wi::input::KEYBOARD_BUTTON_F4:
				keycode = VK_F4;
				break;
			case wi::input::KEYBOARD_BUTTON_F5:
				keycode = VK_F5;
				break;
			case wi::input::KEYBOARD_BUTTON_F6:
				keycode = VK_F6;
				break;
			case wi::input::KEYBOARD_BUTTON_F7:
				keycode = VK_F7;
				break;
			case wi::input::KEYBOARD_BUTTON_F8:
				keycode = VK_F8;
				break;
			case wi::input::KEYBOARD_BUTTON_F9:
				keycode = VK_F9;
				break;
			case wi::input::KEYBOARD_BUTTON_F10:
				keycode = VK_F10;
				break;
			case wi::input::KEYBOARD_BUTTON_F11:
				keycode = VK_F11;
				break;
			case wi::input::KEYBOARD_BUTTON_F12:
				keycode = VK_F12;
				break;
			case wi::input::KEYBOARD_BUTTON_ENTER:
				keycode = VK_RETURN;
				break;
			case wi::input::KEYBOARD_BUTTON_ESCAPE:
				keycode = VK_ESCAPE;
				break;
			case wi::input::KEYBOARD_BUTTON_HOME:
				keycode = VK_HOME;
				break;
			case wi::input::KEYBOARD_BUTTON_LCONTROL:
				keycode = VK_LCONTROL;
				break;
			case wi::input::KEYBOARD_BUTTON_RCONTROL:
				keycode = VK_RCONTROL;
				break;
			case wi::input::KEYBOARD_BUTTON_DELETE:
				keycode = VK_DELETE;
				break;
			case wi::input::KEYBOARD_BUTTON_BACKSPACE:
				keycode = VK_BACK;
				break;
			case wi::input::KEYBOARD_BUTTON_PAGEDOWN:
				keycode = VK_NEXT;
				break;
			case wi::input::KEYBOARD_BUTTON_PAGEUP:
				keycode = VK_PRIOR;
				break;
			case KEYBOARD_BUTTON_NUMPAD0:
				keycode = VK_NUMPAD0;
				break;
			case KEYBOARD_BUTTON_NUMPAD1:
				keycode = VK_NUMPAD1;
				break;
			case KEYBOARD_BUTTON_NUMPAD2:
				keycode = VK_NUMPAD2;
				break;
			case KEYBOARD_BUTTON_NUMPAD3:
				keycode = VK_NUMPAD3;
				break;
			case KEYBOARD_BUTTON_NUMPAD4:
				keycode = VK_NUMPAD4;
				break;
			case KEYBOARD_BUTTON_NUMPAD5:
				keycode = VK_NUMPAD5;
				break;
			case KEYBOARD_BUTTON_NUMPAD6:
				keycode = VK_NUMPAD6;
				break;
			case KEYBOARD_BUTTON_NUMPAD7:
				keycode = VK_NUMPAD7;
				break;
			case KEYBOARD_BUTTON_NUMPAD8:
				keycode = VK_NUMPAD8;
				break;
			case KEYBOARD_BUTTON_NUMPAD9:
				keycode = VK_NUMPAD9;
				break;
			case KEYBOARD_BUTTON_MULTIPLY:
				keycode = VK_MULTIPLY;
				break;
			case KEYBOARD_BUTTON_ADD:
				keycode = VK_ADD;
				break;
			case KEYBOARD_BUTTON_SEPARATOR:
				keycode = VK_SEPARATOR;
				break;
			case KEYBOARD_BUTTON_SUBTRACT:
				keycode = VK_SUBTRACT;
				break;
			case KEYBOARD_BUTTON_DECIMAL:
				keycode = VK_DECIMAL;
				break;
			case KEYBOARD_BUTTON_DIVIDE:
				keycode = VK_DIVIDE;
				break;
#endif // _WIN32
				default: break;
			}
#if defined(_WIN32) && !defined(PLATFORM_XBOX)
			return KEY_DOWN(keycode) || KEY_TOGGLE(keycode);
#elif SDL2
			return keyboard.buttons[keycode] == 1;
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
			inputs.insert(std::make_pair(input, 0));
			return true;
		}
		if (iter->second == 0)
		{
			return true;
		}
		return false;
	}
	bool Release(BUTTON button, int playerindex)
	{
		Input input;
		input.button = button;
		input.playerIndex = playerindex;
		auto iter = inputs.find(input);
		if (iter == inputs.end())
		{
			if (Down(button, playerindex))
				inputs.insert(std::make_pair(input, 0));
			return false;
		}
		if (iter->second == -1)
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
			inputs.insert(std::make_pair(input, 0));
			return false;
		}
		else if ((!continuous && iter->second == (int)frames) || (continuous && iter->second >= (int)frames))
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
#ifdef PLATFORM_WINDOWS_DESKTOP
		HWND hWnd = window;
		const float dpiscaling = (float)GetDpiForWindow(hWnd) / 96.0f;
		POINT p;
		p.x = (LONG)(props.x * dpiscaling);
		p.y = (LONG)(props.y * dpiscaling);
		ClientToScreen(hWnd, &p);
		SetCursorPos(p.x, p.y);
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_UWP
		auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
		auto& bounds = window.Bounds();
		window.PointerPosition(winrt::Windows::Foundation::Point(props.x + bounds.X, props.y + bounds.Y));
#endif // PLATFORM_UWP

#ifdef SDL2
	SDL_WarpMouseInWindow(window, props.x, props.y);
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
				wi::input::xinput::SetControllerFeedback(data, controller.deviceIndex);
			}
			else if (controller.deviceType == Controller::RAWINPUT)
			{
				wi::input::rawinput::SetControllerFeedback(data, controller.deviceIndex);
			}
			else if (controller.deviceType == Controller::SDLINPUT)
			{
				wi::input::sdlinput::SetControllerFeedback(data, controller.deviceIndex);
			}
#ifdef PLATFORM_PS5
			else if (controller.deviceType == Controller::PS5)
			{
				wi::input::ps5::SetControllerFeedback(data, controller.deviceIndex);
			}
#endif // PLATFORM_PS5
		}
	}

	void SetPen(const Pen& _pen)
	{
		pen = _pen;
		pen_override = true;
	}

	const wi::vector<Touch>& GetTouches()
	{
		return touches;
	}

}
