#include "wiXInput.h"

#ifdef WICKEDENGINE_BUILD_XINPUT

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

	short GetMaxControllerCount()
	{
		return arraysize(controllers);
	}
	bool IsControllerConnected(short index)
	{
		if (index < arraysize(connected))
		{
			return connected[index];
		}
		return false;
	}
	XINPUT_STATE GetControllerState(short index)
	{
		if (index < arraysize(controllers))
		{
			return controllers[index];
		}
		return {};
	}
	void SetControllerFeedback(const wiInput::ControllerFeedback data, short index)
	{
		if (index < arraysize(controllers))
		{
			XINPUT_VIBRATION vibration = {};
			vibration.wLeftMotorSpeed = (WORD)(std::max(0.0f, std::min(1.0f, data.motor_left)) * 65535);
			vibration.wRightMotorSpeed = (WORD)(std::max(0.0f, std::min(1.0f, data.motor_right)) * 65535);
			XInputSetState((DWORD)index, &vibration);
		}
	}
}

#endif // WICKEDENGINE_BUILD_XINPUT
