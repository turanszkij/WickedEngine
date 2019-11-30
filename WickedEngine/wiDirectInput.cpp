#include "wiDirectInput.h"

#ifdef WICKEDENGINE_BUILD_DIRECTINPUT

#pragma comment(lib,"dinput8.lib")

#include "wiWindowRegistration.h"

#include <stdio.h> // cstdio doesn't work here!!
#include <wbemidl.h>
#include <oleauto.h>

namespace wiDirectInput
{
	IDirectInput8* directInput = nullptr;
	IDirectInputDevice8* joysticks[4] = {};
	GUID joystick_guids[arraysize(joysticks)] = {};
	bool joystick_initialized[arraysize(joysticks)] = {};
	DIJOYSTATE2 joystick_states[arraysize(joysticks)] = {};
	short connectedJoys = 0;

	//-----------------------------------------------------------------------------
	// Enum each PNP device using WMI and check each device ID to see if it contains 
	// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
	// Unfortunately this information can not be found by just using DirectInput 
	//-----------------------------------------------------------------------------
	BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput)
	{
		// Cache the GUIDs that were recognized as xinput, because this function is slow and it can be called frequently:
		static GUID xinput_guids[16] = {};
		static short xinput_count = 0;
		for (short i = 0; i < xinput_count; ++i)
		{
			if (*pGuidProductFromDirectInput == xinput_guids[i])
			{
				return TRUE;
			}
		}

		IWbemLocator* pIWbemLocator = NULL;
		IEnumWbemClassObject* pEnumDevices = NULL;
		IWbemClassObject* pDevices[20] = { 0 };
		IWbemServices* pIWbemServices = NULL;
		BSTR                    bstrNamespace = NULL;
		BSTR                    bstrDeviceID = NULL;
		BSTR                    bstrClassName = NULL;
		DWORD                   uReturned = 0;
		bool                    bIsXinputDevice = false;
		uint32_t                iDevice = 0;
		VARIANT                 var;
		HRESULT                 hr;

		// CoInit if needed
		hr = CoInitialize(NULL);
		bool bCleanupCOM = SUCCEEDED(hr);

		// Create WMI
		hr = CoCreateInstance(__uuidof(WbemLocator),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(IWbemLocator),
			(LPVOID*)&pIWbemLocator);
		if (FAILED(hr) || pIWbemLocator == NULL)
			goto LCleanup;

		bstrNamespace = SysAllocString(L"\\\\.\\root\\cimv2"); if (bstrNamespace == NULL) goto LCleanup;
		bstrClassName = SysAllocString(L"Win32_PNPEntity");   if (bstrClassName == NULL) goto LCleanup;
		bstrDeviceID = SysAllocString(L"DeviceID");          if (bstrDeviceID == NULL)  goto LCleanup;

		// Connect to WMI 
		hr = pIWbemLocator->ConnectServer(bstrNamespace, NULL, NULL, 0L,
			0L, NULL, NULL, &pIWbemServices);
		if (FAILED(hr) || pIWbemServices == NULL)
			goto LCleanup;

		// Switch security level to IMPERSONATE. 
		CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
			RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);

		hr = pIWbemServices->CreateInstanceEnum(bstrClassName, 0, NULL, &pEnumDevices);
		if (FAILED(hr) || pEnumDevices == NULL)
			goto LCleanup;

		// Loop over all devices
		for (;;)
		{
			// Get 20 at a time
			hr = pEnumDevices->Next(10000, 20, pDevices, &uReturned);
			if (FAILED(hr))
				goto LCleanup;
			if (uReturned == 0)
				break;

			for (iDevice = 0; iDevice < uReturned; iDevice++)
			{
				// For each device, get its device ID
				hr = pDevices[iDevice]->Get(bstrDeviceID, 0L, &var, NULL, NULL);
				if (SUCCEEDED(hr) && var.vt == VT_BSTR && var.bstrVal != NULL)
				{
					// Check if the device ID contains "IG_".  If it does, then it's an XInput device
					// This information can not be found from DirectInput 
					if (wcsstr(var.bstrVal, L"IG_"))
					{
						// If it does, then get the VID/PID from var.bstrVal
						DWORD dwPid = 0, dwVid = 0;
						WCHAR* strVid = wcsstr(var.bstrVal, L"VID_");
						if (strVid && swscanf_s(strVid, L"VID_%4X", &dwVid) != 1)
							dwVid = 0;
						WCHAR* strPid = wcsstr(var.bstrVal, L"PID_");
						if (strPid && swscanf_s(strPid, L"PID_%4X", &dwPid) != 1)
							dwPid = 0;

						// Compare the VID/PID to the DInput device
						DWORD dwVidPid = MAKELONG(dwVid, dwPid);
						if (dwVidPid == pGuidProductFromDirectInput->Data1)
						{
							bIsXinputDevice = true;
							goto LCleanup;
						}
					}
				}
				SAFE_RELEASE(pDevices[iDevice]);
			}
		}

	LCleanup:
		if (bstrNamespace)
			SysFreeString(bstrNamespace);
		if (bstrDeviceID)
			SysFreeString(bstrDeviceID);
		if (bstrClassName)
			SysFreeString(bstrClassName);
		for (iDevice = 0; iDevice < 20; iDevice++)
			SAFE_RELEASE(pDevices[iDevice]);
		SAFE_RELEASE(pEnumDevices);
		SAFE_RELEASE(pIWbemLocator);
		SAFE_RELEASE(pIWbemServices);

		if (bCleanupCOM)
			CoUninitialize();

		if (bIsXinputDevice)
		{
			xinput_guids[xinput_count++] = *pGuidProductFromDirectInput;
		}

		return bIsXinputDevice;
	}
	BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
	{
		if (connectedJoys >= arraysize(joysticks))
		{
			return DIENUM_STOP;
		}
		if (joystick_guids[connectedJoys] == instance->guidInstance)
		{
			// If joystick guid in this slot hasn't changed, we don't create it again:
			connectedJoys++;
			return DIENUM_CONTINUE;
		}

		if (IsXInputDevice(&instance->guidProduct)) // <- Watch out, this is very slow!!!
		{
			return DIENUM_CONTINUE;
		}

		if (joysticks[connectedJoys] != nullptr)
		{
			// This slot GUID is changed, that means this is an other device now, so delete it:
			joysticks[connectedJoys]->Unacquire();
			SAFE_RELEASE(joysticks[connectedJoys]);
		}

		// Obtain an interface to the enumerated joystick.
		HRESULT hr = directInput->CreateDevice(instance->guidInstance, &joysticks[connectedJoys], NULL);

		// If it failed, then we can't use this joystick. (Maybe the user unplugged
		// it while we were in the middle of enumerating it.)
		if (FAILED(hr)) 
		{
			return DIENUM_CONTINUE;
		}

		// Save the joystick guid, so it will not be readded if hadn't changed:
		joystick_guids[connectedJoys] = instance->guidInstance;
		joystick_initialized[connectedJoys] = false;

		connectedJoys++;
		return DIENUM_CONTINUE;
	}
	BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
	{
		HWND hDlg = (HWND)context;

		DIPROPRANGE propRange;
		propRange.diph.dwSize = sizeof(DIPROPRANGE);
		propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		propRange.diph.dwHow = DIPH_BYID;
		propRange.diph.dwObj = instance->dwType;
		propRange.lMin = -1000;
		propRange.lMax = +1000;

		// Set the range for the axis
		for (short i = 0; i < connectedJoys; i++)
		{
			if (FAILED(joysticks[i]->SetProperty(DIPROP_RANGE, &propRange.diph)))
			{
				return DIENUM_STOP;
			}
		}

		return DIENUM_CONTINUE;
	}

	void Initialize()
	{
		HRESULT hr;

		hr = DirectInput8Create(wiWindowRegistration::GetRegisteredInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInput, NULL);
		assert(SUCCEEDED(hr));
	}
	void CleanUp()
	{
		if (joysticks != nullptr)
		{
			for (short i = 0; i < arraysize(joysticks); i++)
			{
				if (joysticks[i] != nullptr) 
				{
					joysticks[i]->Unacquire();
					SAFE_RELEASE(joysticks[i]);
				}
			}
		}

		SAFE_RELEASE(directInput);
	}
	void Update()
	{
		connectedJoys = 0;

		HRESULT hr;

		hr = directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, enumCallback, NULL, DIEDFL_ATTACHEDONLY);
		assert(SUCCEEDED(hr));

		for (short i = 0; i < connectedJoys; i++)
		{
			if (!joystick_initialized[i])
			{
				joystick_initialized[i] = true;

				DIDEVCAPS capabilities;

				// Set the data format to "simple joystick" - a predefined data format 
				//
				// A data format specifies which controls on a device we are interested in,
				// and how they should be reported. This tells DInput that we will be
				// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
				hr = joysticks[i]->SetDataFormat(&c_dfDIJoystick2);
				assert(SUCCEEDED(hr));

				// Set the cooperative level to let DInput know how this device should
				// interact with the system and with other DInput applications.
				hr = joysticks[i]->SetCooperativeLevel(wiWindowRegistration::GetRegisteredWindow(), DISCL_EXCLUSIVE | DISCL_FOREGROUND);
				assert(SUCCEEDED(hr));

				// Determine how many axis the joystick has (so we don't error out setting
				// properties for unavailable axis)
				capabilities.dwSize = sizeof(DIDEVCAPS);
				hr = joysticks[i]->GetCapabilities(&capabilities);
				assert(SUCCEEDED(hr));

				// Enumerate the axes of the joyctick and set the range of each axis. Note:
				// we could just use the defaults, but we're just trying to show an example
				// of enumerating device objects (axes, buttons, etc.).
				hr = joysticks[i]->EnumObjects(enumAxesCallback, NULL, DIDFT_AXIS);
				assert(SUCCEEDED(hr));
			}

			// Poll the device to read the current state
			hr = joysticks[i]->Poll();
			if (FAILED(hr))
			{
				// DInput is telling us that the input stream has been
				// interrupted. We aren't tracking any state between polls, so
				// we don't have any special reset that needs to be done. We
				// just re-acquire and try again.
				hr = joysticks[i]->Acquire();
				while (hr == DIERR_INPUTLOST)
				{
					hr = joysticks[i]->Acquire();
				}

				// If we encounter a fatal error, return failure.
				if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED))
				{
					assert(0);
					continue;
				}

				// If another application has control of this device, return successfully.
				// We'll just have to wait our turn to use the joystick.
				if (hr == DIERR_OTHERAPPHASPRIO)
				{
					continue;
				}
			}

			// Get the input's device state
			hr = joysticks[i]->GetDeviceState(sizeof(DIJOYSTATE2), &joystick_states[i]);
			if (FAILED(hr))
			{
				joystick_guids[i] = {};
			}
		}
	}

	short GetConnectedJoystickCount()
	{
		return connectedJoys;
	}
	DIJOYSTATE2 GetJoystickData(short index)
	{
		if (index < connectedJoys)
		{
			return joystick_states[index];
		}
		return {};
	}

}

#endif // WICKEDENGINE_BUILD_DIRECTINPUT
