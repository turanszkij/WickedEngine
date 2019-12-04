#include "wiRawInput.h"

#ifdef WICKEDENGINE_BUILD_RAWINPUT

#include <vector>
#include <unordered_map>
#include <string>

#define NOMINMAX
#include <windows.h>
#include <hidsdi.h>

#pragma comment(lib,"Hid.lib")

namespace wiRawInput
{
	KeyboardState keyboard;
	MouseState mouse;

	struct Internal_ControllerState
	{
		bool is_xinput = false;
		std::wstring name;
		ControllerState state;
	};
	std::unordered_map<HANDLE, Internal_ControllerState> controller_lookup;
	std::vector<HANDLE> controller_handles;

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
		for (auto& internal_controller : controller_lookup)
		{
			internal_controller.second.state = ControllerState();
		}

		// Enumerate devices to detect lost devices:
		UINT numDevices;
		UINT listResult = GetRawInputDeviceList(NULL, &numDevices, sizeof(RAWINPUTDEVICELIST));
		assert(listResult >= 0);

		static RAWINPUTDEVICELIST devicelist[64];
		listResult = GetRawInputDeviceList(devicelist, &numDevices, sizeof(RAWINPUTDEVICELIST));
		assert(listResult >= 0);
		assert(numDevices <= arraysize(devicelist));

		for (auto& handle : controller_handles)
		{
			if (handle)
			{
				bool connected = false;
				for (UINT i = 0; i < numDevices; ++i)
				{
					const RAWINPUTDEVICELIST& device = devicelist[i];
					if (device.hDevice == handle)
					{
						// device that was previously registered is still connected, nothing to do here:
						connected = true;
						break;
					}
				}
				if (!connected)
				{
					// device was not found among connected devices, this slot is now marked free:
					handle = NULL;
				}
			}
		}

		// Loop through reading raw input until no events are left
		while (true)
		{
			static RAWINPUT rawBuffer[128];
			UINT rawBufferSize = 0;
			UINT count = GetRawInputBuffer(NULL, &rawBufferSize, sizeof(RAWINPUTHEADER));
			assert(count == 0);
			rawBufferSize *= 8;
			assert(rawBufferSize <= sizeof(rawBuffer));
			if (rawBufferSize == 0)
			{
				return;
			}

			// Fill up buffer:
			count = GetRawInputBuffer(rawBuffer, &rawBufferSize, sizeof(RAWINPUTHEADER));
			if (count == -1)
			{
				HRESULT error = HRESULT_FROM_WIN32(GetLastError());
				assert(0);
				return;
			}

			// Process all the events:
			for (UINT current_raw = 0; current_raw < count; ++current_raw)
			{
				const RAWINPUT& raw = rawBuffer[current_raw];

				if (raw.header.dwType == RIM_TYPEKEYBOARD)
				{
					const RAWKEYBOARD& rawkeyboard = raw.data.keyboard;
					keyboard.buttons[rawkeyboard.VKey] = true;
				}
				else if (raw.header.dwType == RIM_TYPEMOUSE)
				{
					const RAWMOUSE& rawmouse = raw.data.mouse;
					if (raw.data.mouse.usFlags == MOUSE_MOVE_RELATIVE)
					{
						mouse.delta_position.x += rawmouse.lLastX;
						mouse.delta_position.y += rawmouse.lLastY;
						if (rawmouse.usButtonFlags == RI_MOUSE_WHEEL)
						{
							mouse.delta_wheel += float((SHORT)rawmouse.usButtonData) / float(WHEEL_DELTA);
						}
					}
					else if (raw.data.mouse.usFlags == MOUSE_MOVE_ABSOLUTE)
					{
						mouse.position.x += rawmouse.lLastX;
						mouse.position.y += rawmouse.lLastY;
					}
				}
				else if (raw.header.dwType == RIM_TYPEHID)
				{
					RID_DEVICE_INFO info;
					info.cbSize = sizeof(RID_DEVICE_INFO);
					UINT bufferSize = sizeof(RID_DEVICE_INFO);
					UINT result = GetRawInputDeviceInfo(raw.header.hDevice, RIDI_DEVICEINFO, &info, &bufferSize);
					assert(result != -1);
					assert(info.dwType == raw.header.dwType);

					Internal_ControllerState& controller_internal = controller_lookup[raw.header.hDevice];
					if (controller_internal.name.empty())
					{
						// lost controller slots can be retaken, otherwise create a new slot:
						bool create_slot = true;
						for (auto& handle : controller_handles)
						{
							if (handle == NULL)
							{
								create_slot = false;
								handle = raw.header.hDevice;
								break;
							}
						}
						if (create_slot)
						{
							controller_handles.push_back(raw.header.hDevice);
						}

						// If this is the first time we see this device handle, we check if it's an xinput device or not (xinput should have IG_ in its name).
						result = GetRawInputDeviceInfo(raw.header.hDevice, RIDI_DEVICENAME, NULL, &bufferSize);
						assert(result == 0);
						controller_internal.name.resize(bufferSize + 1);
						result = GetRawInputDeviceInfo(raw.header.hDevice, RIDI_DEVICENAME, (void*)controller_internal.name.data(), &bufferSize);
						assert(result != -1);
						controller_internal.is_xinput = controller_internal.name.find(L"IG_") != std::wstring::npos;
					}

					if (!controller_internal.is_xinput) // xinput enabled controller will be handled by xinput API
					{
						result = GetRawInputDeviceInfo(raw.header.hDevice, RIDI_PREPARSEDDATA, NULL, &bufferSize);
						assert(result == 0);

						static uint8_t preparsed_data_buffer[1024 * 16]; // 16 KB scratch buffer
						assert(bufferSize < arraysize(preparsed_data_buffer)); // data must fit into scratch buffer!
						PHIDP_PREPARSED_DATA pPreparsedData = (PHIDP_PREPARSED_DATA)preparsed_data_buffer;
						result = GetRawInputDeviceInfo(raw.header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &bufferSize);
						assert(result != -1);

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
							(PCHAR)raw.data.hid.bRawData, raw.data.hid.dwSizeHid
						);
						assert(status == HIDP_STATUS_SUCCESS);

						ControllerState& controller = controller_internal.state;

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
								(PCHAR)raw.data.hid.bRawData, raw.data.hid.dwSizeHid
							);
							assert(status == HIDP_STATUS_SUCCESS);

							switch (pValueCaps[i].Range.UsageMin)
							{
							case 0x30:  // X-axis
								controller.thumbstick_L.x += ((float)value - 128) / 128.0f;
								break;
							case 0x31:  // Y-axis
								controller.thumbstick_L.y += -((float)value - 128) / 128.0f;
								break;
							case 0x32: // Z-axis
								controller.thumbstick_R.x += ((float)value - 128) / 128.0f;
								break;
							case 0x33: // Rotate-X
								controller.trigger_L += (float)value / 256.0f;
								break;
							case 0x34: // Rotate-Y
								controller.trigger_R += (float)value / 256.0f;
								break;
							case 0x35: // Rotate-Z
								controller.thumbstick_R.y += ((float)value - 128) / 128.0f;
								break;
							case 0x39:  // Hat Switch
								controller.pov = (POV)value;
								break;
							}
						}

					}
				}

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
		return (short)controller_handles.size();
	}
	ControllerState GetControllerState(short index)
	{
		if (index < controller_handles.size())
		{
			return controller_lookup[controller_handles[index]].state;
		}
		return {};
	}
	void SetControllerFeedback(const wiInput::ControllerFeedback data, short index)
	{
		if (index < controller_handles.size())
		{
			HANDLE hid_device = CreateFile(
				controller_lookup[controller_handles[index]].name.c_str(), 
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
				OPEN_EXISTING, 0, NULL
			);
			assert(hid_device != INVALID_HANDLE_VALUE);
			uint8_t buf[32] = {};
			buf[0] = 0x05;
			buf[1] = 0xFF;
			buf[4] = uint8_t(std::max(0.0f, std::min(1.0f, data.motor_right)) * 255);
			buf[5] = uint8_t(std::max(0.0f, std::min(1.0f, data.motor_left)) * 255);
			buf[6] = data.led_color.getR();
			buf[7] = data.led_color.getG();
			buf[8] = data.led_color.getB();
			DWORD bytes_written;
			BOOL result = WriteFile(hid_device, buf, sizeof(buf), &bytes_written, NULL);
			assert(result == TRUE);
			assert(bytes_written == arraysize(buf));
			CloseHandle(hid_device);
		}
	}
}

#endif // WICKEDENGINE_BUILD_RAWINPUT
