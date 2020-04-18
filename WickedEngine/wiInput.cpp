#include "wiInput.h"
#include "wiPlatform.h"
#include "wiXInput.h"
#include "wiRawInput.h"
#include "wiHelper.h"
#include "wiBackLog.h"
#include "wiProfiler.h"
#include "wiColor.h"

#include <algorithm>
#include <map>
#include <atomic>
#include <thread>

using namespace std;

namespace wiInput
{

#ifndef WINSTORE_SUPPORT
#define KEY_DOWN(vk_code) (GetAsyncKeyState(vk_code) < 0)
#define KEY_TOGGLE(vk_code) ((GetAsyncKeyState(vk_code) & 1) != 0)
#else
#define KEY_DOWN(vk_code) ((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) < 0)
#define KEY_TOGGLE(vk_code) (((int)Windows::UI::Core::CoreWindow::GetForCurrentThread()->GetAsyncKeyState((Windows::System::VirtualKey)vk_code) & 1) != 0)
#endif //WINSTORE_SUPPORT
#define KEY_UP(vk_code) (!KEY_DOWN(vk_code))


	KeyboardState keyboard;
	MouseState mouse;

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

		wiRawInput::GetMouseState(&mouse); // currently only the relative data can be used from this
		wiRawInput::GetKeyboardState(&keyboard); // it contains pressed buttons as "keyboard/typewriter" like, so no continuous presses

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
		if (!wiPlatform::IsWindowActive())
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
				if (mouse.left_button_press) return true;
				keycode = VK_LBUTTON;
				break;
			case wiInput::MOUSE_BUTTON_RIGHT:
				if (mouse.right_button_press) return true;
				keycode = VK_RBUTTON;
				break;
			case wiInput::MOUSE_BUTTON_MIDDLE:
				if (mouse.middle_button_press) return true;
				keycode = VK_MBUTTON;
				break;
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
			}

			return KEY_DOWN(keycode) | KEY_TOGGLE(keycode);
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
		if (iter->second <= 0)
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
#ifndef WINSTORE_SUPPORT
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(wiPlatform::GetWindow(), &p);
		return XMFLOAT4((float)p.x, (float)p.y, mouse.delta_wheel, 0);
#else
		auto& p = Windows::UI::Core::CoreWindow::GetForCurrentThread()->PointerPosition;
		return XMFLOAT4(p.X, p.Y, 0, 0);
#endif
	}
	void SetPointer(const XMFLOAT4& props)
	{
#ifndef WINSTORE_SUPPORT
		POINT p;
		p.x = (LONG)props.x;
		p.y = (LONG)props.y;
		ClientToScreen(wiPlatform::GetWindow(), &p);
		SetCursorPos(p.x, p.y);
#endif
	}
	void HidePointer(bool value)
	{
#ifndef WINSTORE_SUPPORT
		if (value)
		{
			while (ShowCursor(false) >= 0) {};
		}
		else
		{
			while (ShowCursor(true) < 0) {};
		}
#endif
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

	
#ifdef WINSTORE_SUPPORT
	using namespace Windows::ApplicationModel;
	using namespace Windows::ApplicationModel::Core;
	using namespace Windows::ApplicationModel::Activation;
	using namespace Windows::UI::Core;
	using namespace Windows::UI::Input;
	using namespace Windows::System;
	using namespace Windows::Foundation;

	void _OnPointerPressed(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_PRESSED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		touches.push_back(touch);
	}
	void _OnPointerReleased(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_RELEASED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		touches.push_back(touch);
	}
	void _OnPointerMoved(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_MOVED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		touches.push_back(touch);
	}
#endif // WINSTORE_SUPPORT

	const std::vector<Touch>& GetTouches()
	{
		static bool isRegisteredTouch = false;
		if (!isRegisteredTouch)
		{
#ifdef WINSTORE_SUPPORT
			auto window = CoreWindow::GetForCurrentThread();

			window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerPressed);
			window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerReleased);
			window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(_OnPointerMoved);
#endif // WINSTORE_SUPPORT

			isRegisteredTouch = true;
		}

		return touches;
	}

}
