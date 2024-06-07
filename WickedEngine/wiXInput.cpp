#include "wiXInput.h"

#if __has_include("xinput.h")

#if defined(PLATFORM_WINDOWS_DESKTOP)
#include <xinput.h>
#pragma comment(lib,"xinput.lib")
#endif // PLATFORM_WINDOWS_DESKTOP

#ifdef PLATFORM_XBOX
#include <XInputOnGameInput.h>
using namespace XInputOnGameInput;
#endif // PLATFORM_XBOX

namespace wi::input::xinput
{
	XINPUT_STATE controllers[4] = {};
	bool connected[arraysize(controllers)] = {};

	void Update()
	{
		for (DWORD i = 0; i < arraysize(controllers); i++)
		{
			controllers[i] = {};
			DWORD dwResult = XInputGetState(i, &controllers[i]);

			if (dwResult == ERROR_SUCCESS)
			{
				connected[i] = true;
			}
			else
			{
				connected[i] = false;
			}
		}
	}

	int GetMaxControllerCount()
	{
		return arraysize(controllers);
	}
	constexpr float deadzone(float x)
	{
		if ((x) > -0.24f && (x) < 0.24f)
			x = 0;
		if (x < -1.0f)
			x = -1.0f;
		if (x > 1.0f)
			x = 1.0f;
		return x;
	}
	bool GetControllerState(wi::input::ControllerState* state, int index)
	{
		if (index < arraysize(controllers))
		{
			if (connected[index])
			{
				if (state != nullptr)
				{
					*state = wi::input::ControllerState();
					const XINPUT_STATE& xinput_state = controllers[index];

					// Retrieve buttons:
					for (int button = wi::input::GAMEPAD_RANGE_START + 1; button < wi::input::GAMEPAD_RANGE_END; ++button)
					{
						bool down = false;
						switch (button)
						{
						case wi::input::GAMEPAD_BUTTON_UP: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP; break;
						case wi::input::GAMEPAD_BUTTON_LEFT: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT; break;
						case wi::input::GAMEPAD_BUTTON_DOWN: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN; break;
						case wi::input::GAMEPAD_BUTTON_RIGHT: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT; break;
						case wi::input::GAMEPAD_BUTTON_1: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X; break;
						case wi::input::GAMEPAD_BUTTON_2: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A; break;
						case wi::input::GAMEPAD_BUTTON_3: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B; break;
						case wi::input::GAMEPAD_BUTTON_4: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y; break;
						case wi::input::GAMEPAD_BUTTON_5: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER; break;
						case wi::input::GAMEPAD_BUTTON_6: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
						case wi::input::GAMEPAD_BUTTON_7: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB; break;
						case wi::input::GAMEPAD_BUTTON_8: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB; break;
						case wi::input::GAMEPAD_BUTTON_9: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK; break;
						case wi::input::GAMEPAD_BUTTON_10: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START; break;
						}

						if (down)
						{
							state->buttons |= 1 << (button - wi::input::GAMEPAD_RANGE_START - 1);
						}
					}

					// Retrieve analog inoputs:
					state->thumbstick_L = XMFLOAT2(deadzone((float)xinput_state.Gamepad.sThumbLX / 32767.0f), deadzone((float)xinput_state.Gamepad.sThumbLY / 32767.0f));
					state->thumbstick_R = XMFLOAT2(deadzone((float)xinput_state.Gamepad.sThumbRX / 32767.0f), -deadzone((float)xinput_state.Gamepad.sThumbRY / 32767.0f));
					state->trigger_L = (float)xinput_state.Gamepad.bLeftTrigger / 255.0f;
					state->trigger_R = (float)xinput_state.Gamepad.bRightTrigger / 255.0f;

				}

				return true;
			}
		}
		return false;
	}
	void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index)
	{
		if (index < arraysize(controllers))
		{
			XINPUT_VIBRATION vibration = {};
			vibration.wLeftMotorSpeed = (WORD)(std::max(0.0f, std::min(1.0f, data.vibration_left)) * 65535);
			vibration.wRightMotorSpeed = (WORD)(std::max(0.0f, std::min(1.0f, data.vibration_right)) * 65535);
			XInputSetState((DWORD)index, &vibration);
		}
	}
}

#else
namespace wi::input::xinput
{
	void Update() {}
	int GetMaxControllerCount() { return 0; }
	bool GetControllerState(wi::input::ControllerState* state, int index) { return false; }
	void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index) {}
}
#endif // __has_include("xinput.h")
