#include "wiXInput.h"

#if __has_include("xinput.h")

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#pragma comment(lib,"xinput.lib")

namespace wiXInput
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
	bool GetControllerState(wiInput::ControllerState* state, int index)
	{
		if (index < arraysize(controllers))
		{
			if (connected[index])
			{
				if (state != nullptr)
				{
					*state = wiInput::ControllerState();
					const XINPUT_STATE& xinput_state = controllers[index];

					// Retrieve buttons:
					for (int button = wiInput::GAMEPAD_RANGE_START + 1; button < wiInput::GAMEPAD_RANGE_END; ++button)
					{
						bool down = false;
						switch (button)
						{
						case wiInput::GAMEPAD_BUTTON_UP: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP; break;
						case wiInput::GAMEPAD_BUTTON_LEFT: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT; break;
						case wiInput::GAMEPAD_BUTTON_DOWN: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN; break;
						case wiInput::GAMEPAD_BUTTON_RIGHT: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT; break;
						case wiInput::GAMEPAD_BUTTON_1: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_X; break;
						case wiInput::GAMEPAD_BUTTON_2: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_A; break;
						case wiInput::GAMEPAD_BUTTON_3: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_B; break;
						case wiInput::GAMEPAD_BUTTON_4: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_Y; break;
						case wiInput::GAMEPAD_BUTTON_5: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER; break;
						case wiInput::GAMEPAD_BUTTON_6: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER; break;
						case wiInput::GAMEPAD_BUTTON_7: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB; break;
						case wiInput::GAMEPAD_BUTTON_8: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB; break;
						case wiInput::GAMEPAD_BUTTON_9: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_BACK; break;
						case wiInput::GAMEPAD_BUTTON_10: down = xinput_state.Gamepad.wButtons & XINPUT_GAMEPAD_START; break;
						}

						if (down)
						{
							state->buttons |= 1 << (button - wiInput::GAMEPAD_RANGE_START - 1);
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
	void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index)
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
namespace wiXInput
{
	void Update() {}
	int GetMaxControllerCount() { return 0; }
	bool GetControllerState(wiInput::ControllerState* state, int index) { return false; }
	void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index) {}
}
#endif // __has_include("xinput.h")
