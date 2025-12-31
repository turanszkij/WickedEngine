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
#include "Utility/win32ico.h"
#endif // SDL2

#ifdef PLATFORM_PS5
#include "wiInput_PS5.h"
#endif // PLATFORM_PS5

namespace wi::input
{
#ifdef _WIN32
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
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
	bool double_click = false;
	wi::Timer doubleclick_timer;
	XMFLOAT2 doubleclick_prevpos = XMFLOAT2(0, 0);
	CURSOR cursor_current = CURSOR_COUNT; // something that's not default, because at least once code should change it to default
	CURSOR cursor_next = CURSOR_DEFAULT;

#ifdef SDL2
	static SDL_Cursor* cursor_table_original[] = {
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO),
		SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR),
	};
	static SDL_Cursor* cursor_table[arraysize(cursor_table_original)] = {};
#elif defined(_WIN32)
	static const HCURSOR cursor_table_original[] = {
		::LoadCursor(nullptr, IDC_ARROW),
		::LoadCursor(nullptr, IDC_IBEAM),
		::LoadCursor(nullptr, IDC_SIZEALL),
		::LoadCursor(nullptr, IDC_SIZENS),
		::LoadCursor(nullptr, IDC_SIZEWE),
		::LoadCursor(nullptr, IDC_SIZENESW),
		::LoadCursor(nullptr, IDC_SIZENWSE),
		::LoadCursor(nullptr, IDC_HAND),
		::LoadCursor(nullptr, IDC_NO),
		::LoadCursor(nullptr, IDC_CROSS),
	};
	static HCURSOR cursor_table[arraysize(cursor_table_original)] = {};
#endif


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

		for (int i = 0; i < arraysize(cursor_table); ++i)
		{
			cursor_table[i] = cursor_table_original[i];
		}

		wilog("wi::input Initialized (%d ms)", (int)std::round(timer.elapsed()));
		initialized.store(true);
	}

	void Update(wi::platform::window_type _window, const wi::Canvas& _canvas)
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

#ifdef SDL2
		wi::input::sdlinput::GetMouseState(&mouse);
		wi::input::sdlinput::GetKeyboardState(&keyboard);
#elif defined(_WIN32) && !defined(PLATFORM_XBOX)
		wi::input::rawinput::GetMouseState(&mouse); // currently only the relative data can be used from this
		wi::input::rawinput::GetKeyboardState(&keyboard);

		// apparently checking the mouse here instead of Down() avoids missing the button presses (review!)
		mouse.left_button_press |= KEY_DOWN(VK_LBUTTON);
		mouse.middle_button_press |= KEY_DOWN(VK_MBUTTON);
		mouse.right_button_press |= KEY_DOWN(VK_RBUTTON);

		// Since raw input doesn't contain absolute mouse position, we get it with regular winapi:
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(window, &p);
		mouse.position.x = (float)p.x;
		mouse.position.y = (float)p.y;
#endif

		if (pen_override)
		{
			mouse.position = pen.position;
			mouse.left_button_press = pen.pressure > 0;
			mouse.pressure = pen.pressure;
			pen_override = false;
		}

		// The application always works with logical mouse coordinates:
		mouse.position.x = canvas.PhysicalToLogical(mouse.position.x);
		mouse.position.y = canvas.PhysicalToLogical(mouse.position.y);

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
				case Controller::PS5:
#ifdef PLATFORM_PS5
					connected = wi::input::ps5::GetControllerState(&controller.state, controller.deviceIndex);
#endif // PLATFORM_PS5
					break;
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

		double_click = false;
		if (Press(MOUSE_BUTTON_LEFT))
		{
			XMFLOAT2 pos = mouse.position;
			double elapsed = doubleclick_timer.record_elapsed_seconds();
			if (elapsed < 0.5 && wi::math::Distance(doubleclick_prevpos, pos) < 5)
			{
				double_click = true;
			}
			doubleclick_prevpos = pos;
		}

		// Cursor update:
		if(cursor_next != cursor_current || cursor_next != CURSOR_DEFAULT)
		{
#ifdef SDL2
			SDL_SetCursor(cursor_table[cursor_next] ? cursor_table[cursor_next] : cursor_table[CURSOR_DEFAULT]);
#elif defined(PLATFORM_WINDOWS_DESKTOP)
			::SetCursor(cursor_table[cursor_next]);
#endif
			cursor_current = cursor_next;
		}
		cursor_next = CURSOR_DEFAULT;

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

		if(IsGamepadButton(button))
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

				case GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_UP: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L).y > 0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_LEFT: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L).x < -0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_DOWN: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L).y < -0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_RIGHT: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_L).x > 0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_UP: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R).y > 0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_LEFT: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R).x < -0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_DOWN: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R).y < -0.5f;
				case GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_RIGHT: return GetAnalog(GAMEPAD_ANALOG_THUMBSTICK_R).x > 0.5f;
				case GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON: return GetAnalog(GAMEPAD_ANALOG_TRIGGER_L).x > 0.5f;
				case GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON: return GetAnalog(GAMEPAD_ANALOG_TRIGGER_R).x > 0.5f;

				default: break;
				}

			}
		}
		else if (playerindex == 0) // keyboard or mouse
		{
			if (button == MOUSE_SCROLL_AS_BUTTON_UP)
			{
				return mouse.delta_wheel > 0.1f;
			}
			if (button == MOUSE_SCROLL_AS_BUTTON_DOWN)
			{
				return mouse.delta_wheel < -0.1f;
			}

			uint16_t keycode = (uint16_t)button;

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
			case wi::input::KEYBOARD_BUTTON_INSERT:
				keycode = VK_INSERT;
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
			case KEYBOARD_BUTTON_TAB:
				keycode = VK_TAB;
				break;
			case KEYBOARD_BUTTON_TILDE:
				keycode = VK_OEM_FJ_JISHO; // http://kbdedit.com/manual/low_level_vk_list.html
				break;
			case KEYBOARD_BUTTON_ALT:
				keycode = VK_LMENU;
				break;
			case KEYBOARD_BUTTON_ALTGR:
				keycode = VK_RMENU;
				break;
#endif // _WIN32
				default: break;
			}
#ifdef SDL2
			return keyboard.buttons[keycode] == 1;
#elif defined(_WIN32) && !defined(PLATFORM_XBOX)
			return KEY_DOWN(keycode) || KEY_TOGGLE(keycode);
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
		// The application's logical mouse coordinates are converted to the system's physical coordinates:
		const uint32_t posX = canvas.LogicalToPhysical(props.x);
		const uint32_t posY = canvas.LogicalToPhysical(props.y);

#ifdef SDL2
		SDL_WarpMouseInWindow(window, posX, posY);
#elif defined(PLATFORM_WINDOWS_DESKTOP)
		HWND hWnd = window;
		POINT p;
		p.x = (LONG)(posX);
		p.y = (LONG)(posY);
		ClientToScreen(hWnd, &p);
		SetCursorPos(p.x, p.y);
#endif // SDL2
	}
	void HidePointer(bool value)
	{
#ifdef SDL2
		SDL_SetRelativeMouseMode(value ? SDL_TRUE : SDL_FALSE);
#elif defined(_WIN32)
		if (value)
		{
			while (ShowCursor(false) >= 0) {};
		}
		else
		{
			while (ShowCursor(true) < 0) {};
		}
#endif // _WIN32
	}

	BUTTON WhatIsPressed(int playerindex)
	{
		// First go through predefined enums:
		for (int i = GAMEPAD_RANGE_START + 1; i < BUTTON_ENUM_SIZE; ++i)
		{
			if (Press((BUTTON)i, playerindex))
				return (BUTTON)i;
		}
		// Go through remaining digits, letters and undefined:
		for (int i = DIGIT_RANGE_START; i < GAMEPAD_RANGE_START; ++i)
		{
			if (Press((BUTTON)i, playerindex))
				return (BUTTON)i;
		}
		return BUTTON_NONE;
	}

	bool IsDoubleClicked()
	{
		return double_click;
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

	void SetCursor(CURSOR cursor)
	{
		cursor_next = cursor;
	}

	void SetCursorFromFile(CURSOR cursor, const char* filename)
	{
#ifdef SDL2
		// In SDL extract the raw color data from win32 .CUR file and use SDL to create cursor:
		wi::vector<uint8_t> data;
		if (wi::helper::FileRead(filename, data))
		{
			// Extract only the first image data:
			ico::ICONDIRENTRY* icondirentry = (ico::ICONDIRENTRY*)(data.data() + sizeof(ico::ICONDIR));

			int hotspotX = icondirentry->wPlanes;
			int hotspotY = icondirentry->wBitCount;

			uint8_t* pixeldata = (data.data() + icondirentry->dwImageOffset + sizeof(ico::BITMAPINFOHEADER));

			const uint32_t width = icondirentry->bWidth;
			const uint32_t height = icondirentry->bHeight;

			// Convert BGRA to ARGB and flip vertically
            wi::vector<wi::Color> colors;
            colors.reserve(width * height);
			for (uint32_t y = height; y > 0; --y)
			{
				uint8_t* src = pixeldata + (y - 1) * width * 4;
				for (uint32_t x = 0; x < width; ++x)
				{
					colors.push_back(wi::Color(src[3], src[2], src[1], src[0]));
					src += 4;
				}
			}

			SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
				colors.data(),
				width,
				height,
				4 * 8,
				4 * width,
				0x0000ff00,
				0x00ff0000,
				0xff000000,
				0x000000ff
			);

			if (surface != nullptr)
			{
				cursor_table[cursor] = SDL_CreateColorCursor(surface, hotspotX, hotspotY);
				SDL_FreeSurface(surface);
			}
		}
#elif defined(PLATFORM_WINDOWS_DESKTOP)
		wchar_t wfilename[1024] = {};
		wi::helper::StringConvert(filename, wfilename, arraysize(wfilename));
		cursor_table[cursor] = LoadCursorFromFile(wfilename);
#endif // PLATFORM_WINDOWS_DESKTOP


		// refresh in case we set the current one:
		cursor_next = cursor_current;
		cursor_current = CURSOR_COUNT;
	}

	void ResetCursor(CURSOR cursor)
	{
		cursor_table[cursor] = cursor_table_original[cursor];

		// refresh in case we set the current one:
		cursor_next = cursor_current;
		cursor_current = CURSOR_COUNT;
	}

	void ResetCursors()
	{
		for (int i = 0; i < CURSOR_COUNT; ++i)
		{
			ResetCursor(CURSOR(i));
		}
	}

	void NotifyCursorChanged()
	{
		cursor_current = CURSOR_COUNT;
	}

	BUTTON StringToButton(const char* str)
	{
		if (str == nullptr)
			return BUTTON_NONE;

		if (strcmp(str, "■") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_SQUARE;
		if (strcmp(str, "✖") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_CROSS;
		if (strcmp(str, "●") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_CIRCLE;
		if (strcmp(str, "▲") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE;
		if (strcmp(str, "L1") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_L1;
		if (strcmp(str, "L2") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_L2;
		if (strcmp(str, "L3") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_L3;
		if (strcmp(str, "R1") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_R1;
		if (strcmp(str, "R2") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_R2;
		if (strcmp(str, "R3") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_R3;
		if (strcmp(str, "Share") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_SHARE;
		if (strcmp(str, "Option") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_OPTION;
		if (strcmp(str, "Touchpad") == 0)
			return GAMEPAD_BUTTON_PLAYSTATION_OPTION;

		if (strcmp(str, "(X)") == 0)
			return GAMEPAD_BUTTON_XBOX_X;
		if (strcmp(str, "(A)") == 0)
			return GAMEPAD_BUTTON_XBOX_A;
		if (strcmp(str, "(B)") == 0)
			return GAMEPAD_BUTTON_XBOX_B;
		if (strcmp(str, "(Y)") == 0)
			return GAMEPAD_BUTTON_XBOX_Y;
		if (strcmp(str, "L1") == 0)
			return GAMEPAD_BUTTON_XBOX_L1;
		if (strcmp(str, "LT") == 0)
			return GAMEPAD_BUTTON_XBOX_LT;
		if (strcmp(str, "L3") == 0)
			return GAMEPAD_BUTTON_XBOX_L3;
		if (strcmp(str, "R1") == 0)
			return GAMEPAD_BUTTON_XBOX_R1;
		if (strcmp(str, "RT") == 0)
			return GAMEPAD_BUTTON_XBOX_RT;
		if (strcmp(str, "R3") == 0)
			return GAMEPAD_BUTTON_XBOX_R3;
		if (strcmp(str, "Back") == 0)
			return GAMEPAD_BUTTON_XBOX_BACK;
		if (strcmp(str, "Start") == 0)
			return GAMEPAD_BUTTON_XBOX_START;

		if (strcmp(str, "Dpad ↑") == 0)
			return GAMEPAD_BUTTON_UP;
		if (strcmp(str, "Dpad ←") == 0)
			return GAMEPAD_BUTTON_LEFT;
		if (strcmp(str, "Dpad ↓") == 0)
			return GAMEPAD_BUTTON_DOWN;
		if (strcmp(str, "Dpad →") == 0)
			return GAMEPAD_BUTTON_RIGHT;

		if (strcmp(str, "Gamepad 1") == 0)
			return GAMEPAD_BUTTON_1;
		if (strcmp(str, "Gamepad 2") == 0)
			return GAMEPAD_BUTTON_2;
		if (strcmp(str, "Gamepad 3") == 0)
			return GAMEPAD_BUTTON_3;
		if (strcmp(str, "Gamepad 4") == 0)
			return GAMEPAD_BUTTON_4;
		if (strcmp(str, "Gamepad 5") == 0)
			return GAMEPAD_BUTTON_5;
		if (strcmp(str, "Gamepad 6") == 0)
			return GAMEPAD_BUTTON_6;
		if (strcmp(str, "Gamepad 7") == 0)
			return GAMEPAD_BUTTON_7;
		if (strcmp(str, "Gamepad 8") == 0)
			return GAMEPAD_BUTTON_8;
		if (strcmp(str, "Gamepad 9") == 0)
			return GAMEPAD_BUTTON_9;
		if (strcmp(str, "Gamepad 10") == 0)
			return GAMEPAD_BUTTON_10;
		if (strcmp(str, "Gamepad 11") == 0)
			return GAMEPAD_BUTTON_11;
		if (strcmp(str, "Gamepad 12") == 0)
			return GAMEPAD_BUTTON_12;
		if (strcmp(str, "Gamepad 13") == 0)
			return GAMEPAD_BUTTON_13;
		if (strcmp(str, "Gamepad 14") == 0)
			return GAMEPAD_BUTTON_14;

		if (strcmp(str, "Left Stick ↑") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_UP;
		if (strcmp(str, "Left Stick ←") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_LEFT;
		if (strcmp(str, "Left Stick ↓") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_DOWN;
		if (strcmp(str, "Left Stick →") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_RIGHT;
		if (strcmp(str, "Right Stick ↑") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_UP;
		if (strcmp(str, "Right Stick ←") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_LEFT;
		if (strcmp(str, "Right Stick ↓") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_DOWN;
		if (strcmp(str, "Right Stick →") == 0)
			return GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_RIGHT;

		if (strcmp(str, "Left Trigger") == 0)
			return GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON;
		if (strcmp(str, "Right Trigger") == 0)
			return GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON;

		if (strcmp(str, "Left Mouse Button") == 0)
			return MOUSE_BUTTON_LEFT;
		if (strcmp(str, "Right Mouse Button") == 0)
			return MOUSE_BUTTON_RIGHT;
		if (strcmp(str, "Middle Mouse Button") == 0)
			return MOUSE_BUTTON_MIDDLE;
		if (strcmp(str, "Mouse Wheel ↑") == 0)
			return MOUSE_SCROLL_AS_BUTTON_UP;
		if (strcmp(str, "Mouse Wheel ↓") == 0)
			return MOUSE_SCROLL_AS_BUTTON_DOWN;

		if (strcmp(str, "↑") == 0)
			return KEYBOARD_BUTTON_UP;
		if (strcmp(str, "←") == 0)
			return KEYBOARD_BUTTON_LEFT;
		if (strcmp(str, "↓") == 0)
			return KEYBOARD_BUTTON_DOWN;
		if (strcmp(str, "→") == 0)
			return KEYBOARD_BUTTON_RIGHT;

		if (strcmp(str, "Space") == 0)
			return KEYBOARD_BUTTON_SPACE;
		if (strcmp(str, "Enter") == 0)
			return KEYBOARD_BUTTON_ENTER;
		if (strcmp(str, "Escape") == 0)
			return KEYBOARD_BUTTON_ENTER;
		if (strcmp(str, "Home") == 0)
			return KEYBOARD_BUTTON_HOME;
		if (strcmp(str, "Delete") == 0)
			return KEYBOARD_BUTTON_DELETE;
		if (strcmp(str, "Backspace") == 0)
			return KEYBOARD_BUTTON_BACKSPACE;
		if (strcmp(str, "Page Down") == 0)
			return KEYBOARD_BUTTON_PAGEDOWN;
		if (strcmp(str, "Page Up") == 0)
			return KEYBOARD_BUTTON_PAGEUP;
		if (strcmp(str, "*") == 0)
			return KEYBOARD_BUTTON_MULTIPLY;
		if (strcmp(str, "+") == 0)
			return KEYBOARD_BUTTON_ADD;
		if (strcmp(str, "Separator") == 0)
			return KEYBOARD_BUTTON_SEPARATOR;
		if (strcmp(str, "-") == 0)
			return KEYBOARD_BUTTON_SUBTRACT;
		if (strcmp(str, "Decimal") == 0)
			return KEYBOARD_BUTTON_DECIMAL;
		if (strcmp(str, "/") == 0)
			return KEYBOARD_BUTTON_DIVIDE;
		if (strcmp(str, "Tab") == 0)
			return KEYBOARD_BUTTON_TAB;
		if (strcmp(str, "Tilde") == 0)
			return KEYBOARD_BUTTON_TILDE;
		if (strcmp(str, "Insert") == 0)
			return KEYBOARD_BUTTON_INSERT;
		if (strcmp(str, "Alt") == 0)
			return KEYBOARD_BUTTON_ALT;
		if (strcmp(str, "Alt Gr") == 0)
			return KEYBOARD_BUTTON_ALTGR;

		if (strcmp(str, "Left Shift") == 0)
			return KEYBOARD_BUTTON_LSHIFT;
		if (strcmp(str, "Right Shift") == 0)
			return KEYBOARD_BUTTON_RSHIFT;
		if (strcmp(str, "Left Control") == 0)
			return KEYBOARD_BUTTON_LCONTROL;
		if (strcmp(str, "Right Control") == 0)
			return KEYBOARD_BUTTON_RCONTROL;

		if (strcmp(str, "F1") == 0)
			return KEYBOARD_BUTTON_F1;
		if (strcmp(str, "F2") == 0)
			return KEYBOARD_BUTTON_F2;
		if (strcmp(str, "F3") == 0)
			return KEYBOARD_BUTTON_F3;
		if (strcmp(str, "F4") == 0)
			return KEYBOARD_BUTTON_F4;
		if (strcmp(str, "F5") == 0)
			return KEYBOARD_BUTTON_F5;
		if (strcmp(str, "F6") == 0)
			return KEYBOARD_BUTTON_F6;
		if (strcmp(str, "F7") == 0)
			return KEYBOARD_BUTTON_F7;
		if (strcmp(str, "F8") == 0)
			return KEYBOARD_BUTTON_F8;
		if (strcmp(str, "F9") == 0)
			return KEYBOARD_BUTTON_F9;
		if (strcmp(str, "F10") == 0)
			return KEYBOARD_BUTTON_F10;
		if (strcmp(str, "F11") == 0)
			return KEYBOARD_BUTTON_F11;
		if (strcmp(str, "F12") == 0)
			return KEYBOARD_BUTTON_F12;

		if (strcmp(str, "Numpad 0") == 0)
			return KEYBOARD_BUTTON_NUMPAD0;
		if (strcmp(str, "Numpad 1") == 0)
			return KEYBOARD_BUTTON_NUMPAD1;
		if (strcmp(str, "Numpad 2") == 0)
			return KEYBOARD_BUTTON_NUMPAD2;
		if (strcmp(str, "Numpad 3") == 0)
			return KEYBOARD_BUTTON_NUMPAD3;
		if (strcmp(str, "Numpad 4") == 0)
			return KEYBOARD_BUTTON_NUMPAD4;
		if (strcmp(str, "Numpad 5") == 0)
			return KEYBOARD_BUTTON_NUMPAD5;
		if (strcmp(str, "Numpad 6") == 0)
			return KEYBOARD_BUTTON_NUMPAD6;
		if (strcmp(str, "Numpad 7") == 0)
			return KEYBOARD_BUTTON_NUMPAD7;
		if (strcmp(str, "Numpad 8") == 0)
			return KEYBOARD_BUTTON_NUMPAD8;
		if (strcmp(str, "Numpad 9") == 0)
			return KEYBOARD_BUTTON_NUMPAD9;

		if (str[0] >= DIGIT_RANGE_START /*&& str[0] < GAMEPAD_RANGE_START*/)
		{
			return (BUTTON)str[0];
		}

		return BUTTON_NONE;
	}

	ShortReturnString ButtonToString(BUTTON button, CONTROLLER_PREFERENCE preference)
	{
#ifdef PLATFORM_PS5
		preference = CONTROLLER_PREFERENCE_PLAYSTATION;
#endif // PLATFORM_PS5
#ifdef PLATFORM_XBOX
		preference = CONTROLLER_PREFERENCE_XBOX;
#endif // PLATFORM_XBOX

		if (preference == CONTROLLER_PREFERENCE_PLAYSTATION)
		{
			switch (button)
			{
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_SQUARE: return "■";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_CROSS: return "✖";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_CIRCLE: return "●";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_TRIANGLE: return "▲";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_L1: return "L1";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_L2: return "L2";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_R1: return "R1";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_R2: return "R2";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_L3: return "L3";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_R3: return "R3";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_SHARE: return "Share";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_OPTION: return "Option";
			case wi::input::GAMEPAD_BUTTON_PLAYSTATION_TOUCHPAD: return "Touchpad";
			default:
				break;
			}
		}

		if (preference == CONTROLLER_PREFERENCE_XBOX)
		{
			switch (button)
			{
			case wi::input::GAMEPAD_BUTTON_XBOX_X: return "(X)";
			case wi::input::GAMEPAD_BUTTON_XBOX_A: return "(A)";
			case wi::input::GAMEPAD_BUTTON_XBOX_B: return "(B)";
			case wi::input::GAMEPAD_BUTTON_XBOX_Y: return "(Y)";
			case wi::input::GAMEPAD_BUTTON_XBOX_L1: return "L1";
			case wi::input::GAMEPAD_BUTTON_XBOX_LT: return "LT";
			case wi::input::GAMEPAD_BUTTON_XBOX_R1: return "R1";
			case wi::input::GAMEPAD_BUTTON_XBOX_RT: return "RT";
			case wi::input::GAMEPAD_BUTTON_XBOX_L3: return "L3";
			case wi::input::GAMEPAD_BUTTON_XBOX_R3: return "R3";
			case wi::input::GAMEPAD_BUTTON_XBOX_BACK: return "Back";
			case wi::input::GAMEPAD_BUTTON_XBOX_START: return "Start";
			default:
				break;
			}
		}

		switch (button)
		{
		case wi::input::GAMEPAD_BUTTON_UP: return "Dpad ↑";
		case wi::input::GAMEPAD_BUTTON_LEFT: return "Dpad ←";
		case wi::input::GAMEPAD_BUTTON_DOWN: return "Dpad ↓";
		case wi::input::GAMEPAD_BUTTON_RIGHT: return "Dpad →";
		case wi::input::GAMEPAD_BUTTON_1: return "Gamepad 1";
		case wi::input::GAMEPAD_BUTTON_2: return "Gamepad 2";
		case wi::input::GAMEPAD_BUTTON_3: return "Gamepad 3";
		case wi::input::GAMEPAD_BUTTON_4: return "Gamepad 4";
		case wi::input::GAMEPAD_BUTTON_5: return "Gamepad 5";
		case wi::input::GAMEPAD_BUTTON_6: return "Gamepad 6";
		case wi::input::GAMEPAD_BUTTON_7: return "Gamepad 7";
		case wi::input::GAMEPAD_BUTTON_8: return "Gamepad 8";
		case wi::input::GAMEPAD_BUTTON_9: return "Gamepad 9";
		case wi::input::GAMEPAD_BUTTON_10: return "Gamepad 10";
		case wi::input::GAMEPAD_BUTTON_11: return "Gamepad 11";
		case wi::input::GAMEPAD_BUTTON_12: return "Gamepad 12";
		case wi::input::GAMEPAD_BUTTON_13: return "Gamepad 13";
		case wi::input::GAMEPAD_BUTTON_14: return "Gamepad 14";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_UP: return "Left Stick ↑";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_LEFT: return "Left Stick ←";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_DOWN: return "Left Stick ↓";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_L_AS_BUTTON_RIGHT: return "Left Stick →";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_UP: return "Right Stick ↑";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_LEFT: return "Right Stick ←";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_DOWN: return "Right Stick ↓";
		case wi::input::GAMEPAD_ANALOG_THUMBSTICK_R_AS_BUTTON_RIGHT: return "Right Stick →";
		case wi::input::GAMEPAD_ANALOG_TRIGGER_L_AS_BUTTON: return "Left Trigger";
		case wi::input::GAMEPAD_ANALOG_TRIGGER_R_AS_BUTTON: return "Right Trigger";
		case wi::input::MOUSE_BUTTON_LEFT: return "Left Mouse Button";
		case wi::input::MOUSE_BUTTON_RIGHT: return "Right Mouse Button";
		case wi::input::MOUSE_BUTTON_MIDDLE: return "Middle Mouse Button";
		case wi::input::MOUSE_SCROLL_AS_BUTTON_UP: return "Mouse Wheel ↑";
		case wi::input::MOUSE_SCROLL_AS_BUTTON_DOWN: return "Mouse Wheel ↓";
		case wi::input::KEYBOARD_BUTTON_UP: return "↑";
		case wi::input::KEYBOARD_BUTTON_DOWN: return "↓";
		case wi::input::KEYBOARD_BUTTON_LEFT: return "←";
		case wi::input::KEYBOARD_BUTTON_RIGHT: return "→";
		case wi::input::KEYBOARD_BUTTON_SPACE: return "Space";
		case wi::input::KEYBOARD_BUTTON_RSHIFT: return "Right Shift";
		case wi::input::KEYBOARD_BUTTON_LSHIFT: return "Left Shift";
		case wi::input::KEYBOARD_BUTTON_F1: return "F1";
		case wi::input::KEYBOARD_BUTTON_F2: return "F2";
		case wi::input::KEYBOARD_BUTTON_F3: return "F3";
		case wi::input::KEYBOARD_BUTTON_F4: return "F4";
		case wi::input::KEYBOARD_BUTTON_F5: return "F5";
		case wi::input::KEYBOARD_BUTTON_F6: return "F6";
		case wi::input::KEYBOARD_BUTTON_F7: return "F7";
		case wi::input::KEYBOARD_BUTTON_F8: return "F8";
		case wi::input::KEYBOARD_BUTTON_F9: return "F9";
		case wi::input::KEYBOARD_BUTTON_F10: return "F10";
		case wi::input::KEYBOARD_BUTTON_F11: return "F11";
		case wi::input::KEYBOARD_BUTTON_F12: return "F12";
		case wi::input::KEYBOARD_BUTTON_ENTER: return "Enter";
		case wi::input::KEYBOARD_BUTTON_ESCAPE: return "Escape";
		case wi::input::KEYBOARD_BUTTON_HOME: return "Home";
		case wi::input::KEYBOARD_BUTTON_RCONTROL: return "Right Control";
		case wi::input::KEYBOARD_BUTTON_LCONTROL: return "Left Control";
		case wi::input::KEYBOARD_BUTTON_DELETE: return "Delete";
		case wi::input::KEYBOARD_BUTTON_BACKSPACE: return "Backspace";
		case wi::input::KEYBOARD_BUTTON_PAGEDOWN: return "Page Down";
		case wi::input::KEYBOARD_BUTTON_PAGEUP: return "Page Up";
		case wi::input::KEYBOARD_BUTTON_NUMPAD0: return "Numpad 0";
		case wi::input::KEYBOARD_BUTTON_NUMPAD1: return "Numpad 1";
		case wi::input::KEYBOARD_BUTTON_NUMPAD2: return "Numpad 2";
		case wi::input::KEYBOARD_BUTTON_NUMPAD3: return "Numpad 3";
		case wi::input::KEYBOARD_BUTTON_NUMPAD4: return "Numpad 4";
		case wi::input::KEYBOARD_BUTTON_NUMPAD5: return "Numpad 5";
		case wi::input::KEYBOARD_BUTTON_NUMPAD6: return "Numpad 6";
		case wi::input::KEYBOARD_BUTTON_NUMPAD7: return "Numpad 7";
		case wi::input::KEYBOARD_BUTTON_NUMPAD8: return "Numpad 8";
		case wi::input::KEYBOARD_BUTTON_NUMPAD9: return "Numpad 9";
		case wi::input::KEYBOARD_BUTTON_MULTIPLY: return "*";
		case wi::input::KEYBOARD_BUTTON_ADD: return "+";
		case wi::input::KEYBOARD_BUTTON_SEPARATOR: return "Separator";
		case wi::input::KEYBOARD_BUTTON_SUBTRACT: return "-";
		case wi::input::KEYBOARD_BUTTON_DECIMAL: return "Decimal";
		case wi::input::KEYBOARD_BUTTON_DIVIDE: return "/";
		case wi::input::KEYBOARD_BUTTON_TAB: return "Tab";
		case wi::input::KEYBOARD_BUTTON_TILDE: return "Tilde";
		case wi::input::KEYBOARD_BUTTON_INSERT: return "Insert";
		case wi::input::KEYBOARD_BUTTON_ALT: return "Alt";
		case wi::input::KEYBOARD_BUTTON_ALTGR: return "Alt Gr";
		default:
			break;
		}

		if (button >= DIGIT_RANGE_START && button < GAMEPAD_RANGE_START)
		{
			ShortReturnString str;
			str.text[0] = (char)button;
			return str;
		}

		return "";
	}

}
