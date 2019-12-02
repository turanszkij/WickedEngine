#include "wiRawInput.h"

#ifdef WICKEDENGINE_BUILD_RAWINPUT

#include <windows.h>
#include <hidsdi.h>
typedef UINT64 QWORD;

#pragma comment(lib,"Hid.lib")

namespace wiRawInput
{
	KeyboardState keyboard;
	MouseState mouse;
	ControllerState controllers[8];
	short controller_count = 0;

	void Initialize()
	{
		RAWINPUTDEVICE Rid[4] = {};

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
		
		// Register gamepad:
		Rid[2].usUsagePage = 0x01;
		Rid[2].usUsage = 0x05;
		Rid[2].dwFlags = 0;
		Rid[2].hwndTarget = 0;

		// Register joystick:
		Rid[3].usUsagePage = 0x01;
		Rid[3].usUsage = 0x04;
		Rid[3].dwFlags = 0;
		Rid[3].hwndTarget = 0;

		if (RegisterRawInputDevices(Rid, arraysize(Rid), sizeof(Rid[0])) == FALSE) 
		{
			//registration failed. Call GetLastError for the cause of the error
			assert(0);
		}
	}

	void Update()
	{
		keyboard = KeyboardState();
		mouse = MouseState();
		for (int i = 0; i < arraysize(controllers); ++i)
		{
			controllers[i] = ControllerState();
		}
		controller_count = 0;

		static RAWINPUT rawBuffer[128];
		UINT rawBufferSize = sizeof(rawBuffer);

		// Loop through reading raw input until no events are left,
		while (true)
		{
			// Fill up buffer,
			UINT count = GetRawInputBuffer((PRAWINPUT)rawBuffer, &rawBufferSize, sizeof(RAWINPUTHEADER));
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
					const RAWKEYBOARD& rawkeyboard = raw->data.keyboard;
					keyboard.buttons[rawkeyboard.VKey] = true;
				}
				else if (raw->header.dwType == RIM_TYPEMOUSE)
				{
					const RAWMOUSE& rawmouse = raw->data.mouse;
					if (raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
					{
						mouse.delta_position.x += rawmouse.lLastX;
						mouse.delta_position.y += rawmouse.lLastY;
						if (rawmouse.usButtonFlags == RI_MOUSE_WHEEL)
						{
							mouse.delta_wheel += float(*(SHORT*)&rawmouse.usButtonData) / float(WHEEL_DELTA);
						}
					}
				}
				else if(raw->header.dwType == RIM_TYPEHID && controller_count < arraysize(controllers))
				{
					RID_DEVICE_INFO info;
					info.cbSize = sizeof(RID_DEVICE_INFO);
					UINT bufferSize = sizeof(RID_DEVICE_INFO);
					UINT result = GetRawInputDeviceInfo(raw->header.hDevice, RIDI_DEVICEINFO, &info, &bufferSize);
					assert(result >= 0);

					if (/*info.hid.dwProductId != 767 &&*/ info.hid.dwVendorId != 1118) // filter out xbox controller, that will be handled by xinput (todo: probably there is a better way)
					{
						result = GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize);
						assert(result == 0);

						static uint8_t preparsed_data_buffer[1024 * 16]; // 16 KB scratch buffer
						assert(bufferSize < arraysize(preparsed_data_buffer)); // data must fit into scratch buffer!
						PHIDP_PREPARSED_DATA pPreparsedData = (PHIDP_PREPARSED_DATA)preparsed_data_buffer;
						result = GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize);
						assert(result >= 0);

						HIDP_CAPS Caps;
						NTSTATUS status = HidP_GetCaps(pPreparsedData, &Caps);
						assert(status == HIDP_STATUS_SUCCESS);

						static uint8_t buttoncaps_buffer[1024 * 4]; // 4 KB scratch buffer
						assert(sizeof(HIDP_BUTTON_CAPS) * Caps.NumberInputButtonCaps < arraysize(buttoncaps_buffer)); // data must fit into scratch buffer!
						PHIDP_BUTTON_CAPS pButtonCaps = (PHIDP_BUTTON_CAPS)buttoncaps_buffer;
						USHORT capsLength = Caps.NumberInputButtonCaps;
						status = HidP_GetButtonCaps(HidP_Input, pButtonCaps, &capsLength, pPreparsedData);
						assert(status == HIDP_STATUS_SUCCESS);

						static uint8_t valuecaps_buffer[1024 * 4]; // 4 KB scratch buffer
						assert(sizeof(HIDP_VALUE_CAPS) * Caps.NumberInputValueCaps < arraysize(valuecaps_buffer)); // data must fit into scratch buffer!
						ULONG numberOfButtons = pButtonCaps->Range.UsageMax - pButtonCaps->Range.UsageMin + 1;
						PHIDP_VALUE_CAPS pValueCaps = (PHIDP_VALUE_CAPS)valuecaps_buffer;
						capsLength = Caps.NumberInputValueCaps;
						status = HidP_GetValueCaps(HidP_Input, pValueCaps, &capsLength, pPreparsedData);
						assert(status == HIDP_STATUS_SUCCESS);

						static USAGE usage[128];
						assert(numberOfButtons <= arraysize(usage));
						status = HidP_GetUsages(
							HidP_Input, pButtonCaps->UsagePage, 0, usage,
							&numberOfButtons, pPreparsedData,
							(PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid
						);
						assert(status == HIDP_STATUS_SUCCESS);

						ControllerState& controller = controllers[controller_count];

						for (ULONG i = 0; i < numberOfButtons; i++)
						{
							int button = usage[i] - pButtonCaps->Range.UsageMin;
							if (button < arraysize(controller.buttons))
							{
								controller.buttons[button] = true;
							}
						}

						for (USHORT i = 0; i < Caps.NumberInputValueCaps; i++)
						{
							ULONG value;
							status = HidP_GetUsageValue(
								HidP_Input, pValueCaps[i].UsagePage, 0,
								pValueCaps[i].Range.UsageMin, &value, pPreparsedData,
								(PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid
							);
							assert(status == HIDP_STATUS_SUCCESS);

							switch (pValueCaps[i].Range.UsageMin)
							{
							case 0x30:  // X-axis
								controller.thumbstick_L.x = ((float)value - 128) / 128.0f;
								break;
							case 0x31:  // Y-axis
								controller.thumbstick_L.y = -((float)value - 128) / 128.0f;
								break;
							case 0x32: // Z-axis
								controller.thumbstick_R.x = ((float)value - 128) / 128.0f;
								break;
							case 0x33: // Rotate-X
								controller.trigger_L = (float)value / 256.0f;
								break;
							case 0x34: // Rotate-Y
								controller.trigger_R = (float)value / 256.0f;
								break;
							case 0x35: // Rotate-Z
								controller.thumbstick_R.y = ((float)value - 128) / 128.0f;
								break;
							case 0x39:  // Hat Switch
								controller.pov = (POV)value;
								break;
							}
						}

						controller_count++;
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

	KeyboardState GetKeyboardState()
	{
		return keyboard;
	}

	MouseState GetMouseState()
	{
		return mouse;
	}

	short GetControllerCount()
	{
		return controller_count;
	}
	ControllerState GetControllerState(short index)
	{
		return controllers[index];
	}
}

#endif // WICKEDENGINE_BUILD_RAWINPUT
