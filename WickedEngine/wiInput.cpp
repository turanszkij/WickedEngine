#include "wiInput.h"
#include "Platform.h"
#include "wiXInput.h"
#include "wiDirectInput.h"
#include "wiRawInput.h"
#include "wiHelper.h"
#include "wiBackLog.h"
#include "wiWindowRegistration.h"

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


	struct Input 
	{
		BUTTON button = BUTTON_NONE;
		short playerIndex = 0;

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
			XINPUT,
			DIRECTINPUT,
		};
		DeviceType deviceType;
		int deviceIndex;
	};
	std::vector<Controller> controllers;
	std::atomic_bool initialized{ false };

	void Initialize()
	{
#ifdef WICKEDENGINE_BUILD_DIRECTINPUT
		wiDirectInput::Initialize();
#endif // WICKEDENGINE_BUILD_DIRECTINPUT

#ifdef WICKEDENGINE_BUILD_RAWINPUT
		wiRawInput::Initialize();
#endif // WICKEDENGINE_BUILD_RAWINPUT

		wiBackLog::post("wiInput Initialized");
		initialized.store(true);
	}

	void CleanUp()
	{
#ifdef WICKEDENGINE_BUILD_DIRECTINPUT
		wiDirectInput::CleanUp();
#endif // WICKEDENGINE_BUILD_DIRECTINPUT
	}

	void Update()
	{
		if (!initialized.load())
		{
			return;
		}

		controllers.clear();

#ifdef WICKEDENGINE_BUILD_XINPUT
		wiXInput::Update();
		for (short i = 0; i < wiXInput::GetMaxGamepadCount(); ++i)
		{
			if (wiXInput::IsGamepadConnected(i))
			{
				controllers.push_back({ Controller::XINPUT, i });
			}
		}
#endif // WICKEDENGINE_BUILD_XINPUT

#ifdef WICKEDENGINE_BUILD_DIRECTINPUT
		wiDirectInput::Update();
		for (int i = 0; i < wiDirectInput::GetConnectedJoystickCount(); ++i)
		{
			controllers.push_back({ Controller::DIRECTINPUT, i });
		}
#endif // WICKEDENGINE_BUILD_DIRECTINPUT

#ifdef WICKEDENGINE_BUILD_RAWINPUT
		wiRawInput::Update();
#endif // WICKEDENGINE_BUILD_RAWINPUT

		for (auto iter = inputs.begin(); iter != inputs.end();)
		{
			BUTTON button = iter->first.button;
			short playerIndex = iter->first.playerIndex;

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

	}

	bool Down(BUTTON button, short playerindex)
	{
		if (!initialized.load())
		{
			return false;
		}
		if (!wiWindowRegistration::IsWindowActive())
		{
			return false;
		}

		if(button > GAMEPAD_RANGE_START)
		{
			if (playerindex < (int)controllers.size())
			{
				const Controller& controller = controllers[playerindex];

#ifdef WICKEDENGINE_BUILD_XINPUT
				if (controller.deviceType == Controller::XINPUT)
				{
					XINPUT_STATE state = wiXInput::GetGamepadData(controller.deviceIndex);

					switch (button)
					{
					case GAMEPAD_BUTTON_UP: return state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
					case GAMEPAD_BUTTON_LEFT: return state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
					case GAMEPAD_BUTTON_DOWN: return state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
					case GAMEPAD_BUTTON_RIGHT: return state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
					case GAMEPAD_BUTTON_1: return state.Gamepad.wButtons & XINPUT_GAMEPAD_X;
					case GAMEPAD_BUTTON_2: return state.Gamepad.wButtons & XINPUT_GAMEPAD_A;
					case GAMEPAD_BUTTON_3: return state.Gamepad.wButtons & XINPUT_GAMEPAD_B;
					case GAMEPAD_BUTTON_4: return state.Gamepad.wButtons & XINPUT_GAMEPAD_Y;
					case GAMEPAD_BUTTON_5: return state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
					case GAMEPAD_BUTTON_6: return state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
					case GAMEPAD_BUTTON_7: return state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB;
					case GAMEPAD_BUTTON_8: return state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB;
					case GAMEPAD_BUTTON_9: return state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK;
					case GAMEPAD_BUTTON_10: return state.Gamepad.wButtons & XINPUT_GAMEPAD_START;
					default:
						break;
					}
				}
#endif // WICKEDENGINE_BUILD_XINPUT

#ifdef WICKEDENGINE_BUILD_DIRECTINPUT
				if (controller.deviceType == Controller::DIRECTINPUT)
				{
					DIJOYSTATE2 state = wiDirectInput::GetJoystickData(controller.deviceIndex);
					DWORD dinput_directions = state.rgdwPOV[0];

					switch (button)
					{
					case GAMEPAD_BUTTON_UP: return dinput_directions == wiDirectInput::POV_UP || dinput_directions == wiDirectInput::POV_LEFTUP || dinput_directions == wiDirectInput::POV_UPRIGHT;
					case GAMEPAD_BUTTON_LEFT: return dinput_directions == wiDirectInput::POV_LEFT || dinput_directions == wiDirectInput::POV_LEFTUP || dinput_directions == wiDirectInput::POV_DOWNLEFT;
					case GAMEPAD_BUTTON_DOWN: return dinput_directions == wiDirectInput::POV_DOWN || dinput_directions == wiDirectInput::POV_DOWNLEFT || dinput_directions == wiDirectInput::POV_RIGHTDOWN;
					case GAMEPAD_BUTTON_RIGHT: return dinput_directions == wiDirectInput::POV_RIGHT || dinput_directions == wiDirectInput::POV_RIGHTDOWN || dinput_directions == wiDirectInput::POV_UPRIGHT;
					case GAMEPAD_BUTTON_1: return state.rgbButtons[0] != 0;
					case GAMEPAD_BUTTON_2: return state.rgbButtons[1] != 0;
					case GAMEPAD_BUTTON_3: return state.rgbButtons[2] != 0;
					case GAMEPAD_BUTTON_4: return state.rgbButtons[3] != 0;
					case GAMEPAD_BUTTON_5: return state.rgbButtons[4] != 0;
					case GAMEPAD_BUTTON_6: return state.rgbButtons[5] != 0;
					case GAMEPAD_BUTTON_7: return state.rgbButtons[6] != 0;
					case GAMEPAD_BUTTON_8: return state.rgbButtons[7] != 0;
					case GAMEPAD_BUTTON_9: return state.rgbButtons[8] != 0;
					case GAMEPAD_BUTTON_10: return state.rgbButtons[9] != 0;
					case GAMEPAD_BUTTON_11: return state.rgbButtons[10] != 0;
					case GAMEPAD_BUTTON_12: return state.rgbButtons[11] != 0;
					case GAMEPAD_BUTTON_13: return state.rgbButtons[12] != 0;
					case GAMEPAD_BUTTON_14: return state.rgbButtons[13] != 0;
					default:
						break;
					}
				}
#endif // WICKEDENGINE_BUILD_DIRECTINPUT

			}
		}
		else if (playerindex == 0) // keyboard or mouse
		{
			int keycode = (int)button;

			switch (button)
			{
			case wiInput::MOUSE_BUTTON_LEFT:
				keycode = VK_LBUTTON;
				break;
			case wiInput::MOUSE_BUTTON_RIGHT:
				keycode = VK_RBUTTON;
				break;
			case wiInput::MOUSE_BUTTON_MIDDLE:
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
			}

			return KEY_DOWN(keycode) | KEY_TOGGLE(keycode);
		}

		return false;
	}
	bool Press(BUTTON button, short playerindex)
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
	bool Hold(BUTTON button, uint32_t frames, bool continuous, short playerIndex)
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
		float mousewheel_scrolled = 0;
#ifdef WICKEDENGINE_BUILD_RAWINPUT
		mousewheel_scrolled = wiRawInput::GetMouseDelta_Wheel();
#endif // WICKEDENGINE_BUILD_RAWINPUT

#ifndef WINSTORE_SUPPORT
		POINT p;
		GetCursorPos(&p);
		ScreenToClient(wiWindowRegistration::GetRegisteredWindow(), &p);
		return XMFLOAT4((float)p.x, (float)p.y, mousewheel_scrolled, 0);
#else
		auto& p = Windows::UI::Core::CoreWindow::GetForCurrentThread()->PointerPosition;
		return XMFLOAT4(p.X, p.Y, mousewheel_scrolled, 0);
#endif
	}
	void SetPointer(const XMFLOAT4& props)
	{
#ifndef WINSTORE_SUPPORT
		POINT p;
		p.x = (LONG)props.x;
		p.y = (LONG)props.y;
		ClientToScreen(wiWindowRegistration::GetRegisteredWindow(), &p);
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

	inline float deadzone(float x)
	{
		if ((x) > -0.24f && (x) < 0.24f)
			x = 0;
		if (x < -1.0f)
			x = -1.0f;
		if (x > 1.0f)
			x = 1.0f;
		return x;
	}
	XMFLOAT4 GetAnalog(GAMEPAD_ANALOG analog, short playerindex)
	{
		if (playerindex < (int)controllers.size())
		{
			const Controller& controller = controllers[playerindex];

#ifdef WICKEDENGINE_BUILD_XINPUT
			if (controller.deviceType == Controller::XINPUT)
			{
				XINPUT_STATE state = wiXInput::GetGamepadData(controller.deviceIndex);

				switch (analog)
				{
				case GAMEPAD_ANALOG_THUMBSTICK_L: return XMFLOAT4(deadzone((float)state.Gamepad.sThumbLX / 32767.0f), deadzone((float)state.Gamepad.sThumbLY / 32767.0f), 0, 0);
				case GAMEPAD_ANALOG_THUMBSTICK_R: return XMFLOAT4(deadzone((float)state.Gamepad.sThumbRX / 32767.0f), -deadzone((float)state.Gamepad.sThumbRY / 32767.0f), 0, 0);
				case GAMEPAD_ANALOG_TRIGGER_L: return XMFLOAT4((float)state.Gamepad.bLeftTrigger / 255.0f, 0, 0, 0);
				case GAMEPAD_ANALOG_TRIGGER_R: return XMFLOAT4((float)state.Gamepad.bRightTrigger / 255.0f, 0, 0, 0);
				default:
					break;
				}
			}
#endif // WICKEDENGINE_BUILD_XINPUT

#ifdef WICKEDENGINE_BUILD_DIRECTINPUT
			if (controller.deviceType == Controller::DIRECTINPUT)
			{
				DIJOYSTATE2 state = wiDirectInput::GetJoystickData(controller.deviceIndex);

				switch (analog)
				{
				case GAMEPAD_ANALOG_THUMBSTICK_L: return XMFLOAT4(deadzone((float)state.lX / 1000.0f), deadzone(-(float)state.lY / 1000.0f), 0, 0);
				case GAMEPAD_ANALOG_THUMBSTICK_R: return XMFLOAT4(deadzone((float)state.lZ / 1000.0f), deadzone((float)state.lRz / 1000.0f), 0, 0);
				case GAMEPAD_ANALOG_TRIGGER_L: return XMFLOAT4((float)state.lRx / 1000.0f * 0.5f + 0.5f, 0, 0, 0);
				case GAMEPAD_ANALOG_TRIGGER_R: return XMFLOAT4((float)state.lRy / 1000.0f * 0.5f + 0.5f, 0, 0, 0);
				default:
					break;
				}
			}
#endif // WICKEDENGINE_BUILD_DIRECTINPUT

		}

		return XMFLOAT4(0, 0, 0, 0);
	}


	void AddTouch(const Touch& touch)
	{
		touches.push_back(touch);
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
		AddTouch(touch);
	}
	void _OnPointerReleased(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_RELEASED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		AddTouch(touch);
	}
	void _OnPointerMoved(CoreWindow^ window, PointerEventArgs^ pointer)
	{
		auto p = pointer->CurrentPoint;

		Touch touch;
		touch.state = Touch::TOUCHSTATE_MOVED;
		touch.pos = XMFLOAT2(p->Position.X, p->Position.Y);
		AddTouch(touch);
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
