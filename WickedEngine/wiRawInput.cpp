#include "wiRawInput.h"

#ifdef WICKEDENGINE_BUILD_RAWINPUT

#include <windows.h>
typedef UINT64 QWORD;

namespace wiRawInput
{
	struct State
	{
		XMINT2 mouse_delta = XMINT2(0, 0);
		float mouse_delta_wheel = 0;
		bool keyboard_buttons[128] = {};
	} state;

	void Initialize()
	{
		RAWINPUTDEVICE Rid[2];

		// Register mouse:
		Rid[0].usUsagePage = 0x01;
		Rid[0].usUsage = 0x02;
		Rid[0].dwFlags = 0;
		Rid[0].hwndTarget = 0;

		// Register keyboard:
		Rid[1].usUsagePage = 0x01;
		Rid[1].usUsage = 0x06;
		Rid[1].dwFlags = 0;
		Rid[1].hwndTarget = 0;

		if (RegisterRawInputDevices(Rid, arraysize(Rid), sizeof(Rid[0])) == FALSE) 
		{
			//registration failed. Call GetLastError for the cause of the error
			assert(0);
		}
	}

	void Update()
	{
		static RAWINPUT rawBuffer[128];
		UINT bytes = sizeof(rawBuffer);

		state = {};

		// Loop through reading raw input until no events are left,
		while (true)
		{
			// Fill up buffer,
			UINT count = GetRawInputBuffer((PRAWINPUT)rawBuffer, &bytes, sizeof(RAWINPUTHEADER));
			if (count <= 0)
			{
				return;
			}

			// Process all the events, 
			const RAWINPUT* raw = (const RAWINPUT*)rawBuffer;
			while (true) 
			{
				if (raw->header.dwType == RIM_TYPEKEYBOARD)
				{
					const RAWKEYBOARD& keyboard = raw->data.keyboard;
					state.keyboard_buttons[keyboard.VKey] = true;
				}
				else if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					const RAWMOUSE& mouse = raw->data.mouse;
					if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
					{
						state.mouse_delta.x += mouse.lLastX;
						state.mouse_delta.y += mouse.lLastY;
						if (mouse.usButtonFlags == RI_MOUSE_WHEEL)
						{
							state.mouse_delta_wheel += float(*(SHORT*)&mouse.usButtonData) / float(WHEEL_DELTA);
						}
					}
				}

				// Go to next raw event.
				count--;
				if (count <= 0)
				{
					break;
				}
				raw = NEXTRAWINPUTBLOCK(raw);
			}
		}
	}

	XMINT2 GetMouseDelta_XY()
	{
		return state.mouse_delta;
	}
	float GetMouseDelta_Wheel()
	{
		return state.mouse_delta_wheel;
	}

	bool IsKeyDown(unsigned char keycode)
	{
		return state.keyboard_buttons[keycode];
	}
}

#endif // WICKEDENGINE_BUILD_RAWINPUT
