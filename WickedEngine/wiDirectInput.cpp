#include "wiDirectInput.h"
#include "CommonInclude.h"

short wiDirectInput::connectedJoys = 0;

#ifndef WINSTORE_SUPPORT

#include <stdio.h>

#include <wbemidl.h>
#include <oleauto.h>
//#include <wmsstd.h>

//-----------------------------------------------------------------------------
// Enum each PNP device using WMI and check each device ID to see if it contains 
// "IG_" (ex. "VID_045E&PID_028E&IG_00").  If it does, then it's an XInput device
// Unfortunately this information can not be found by just using DirectInput 
//-----------------------------------------------------------------------------
BOOL IsXInputDevice(const GUID* pGuidProductFromDirectInput)
{
	IWbemLocator*           pIWbemLocator = NULL;
	IEnumWbemClassObject*   pEnumDevices = NULL;
	IWbemClassObject*       pDevices[20] = { 0 };
	IWbemServices*          pIWbemServices = NULL;
	BSTR                    bstrNamespace = NULL;
	BSTR                    bstrDeviceID = NULL;
	BSTR                    bstrClassName = NULL;
	DWORD                   uReturned = 0;
	bool                    bIsXinputDevice = false;
	UINT                    iDevice = 0;
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

		for (iDevice = 0; iDevice<uReturned; iDevice++)
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
	for (iDevice = 0; iDevice<20; iDevice++)
		SAFE_RELEASE(pDevices[iDevice]);
	SAFE_RELEASE(pEnumDevices);
	SAFE_RELEASE(pIWbemLocator);
	SAFE_RELEASE(pIWbemServices);

	if (bCleanupCOM)
		CoUninitialize();

	return bIsXinputDevice;
}

wiDirectInput::wiDirectInput(HINSTANCE hinstance, HWND hwnd)
{
	for(int i=0;i<256;++i){
		m_keyboardState[i]=0;
	}
	Initialize(hinstance, hwnd);
}
wiDirectInput::~wiDirectInput()
{
	Shutdown();
}

IDirectInputDevice8* m_keyboard = 0;
IDirectInputDevice8* joystick[2] = {0,0};
IDirectInput8*  m_directInput = 0;

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
	HRESULT hr; 
	
	//SLOW!!!
	if (IsXInputDevice(&instance->guidProduct))
		return DIENUM_CONTINUE;

    // Obtain an interface to the enumerated joystick.
    hr = m_directInput->CreateDevice(instance->guidInstance, &joystick[wiDirectInput::connectedJoys], NULL);

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(hr)) { 
        return DIENUM_CONTINUE;
    }

    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
	wiDirectInput::connectedJoys++;
	if (wiDirectInput::connectedJoys<2)
		return DIENUM_CONTINUE;
	else 
		return DIENUM_STOP;
}
BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context)
{
	HWND hDlg = (HWND)context;

	DIPROPRANGE propRange; 
	propRange.diph.dwSize       = sizeof(DIPROPRANGE); 
	propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
	propRange.diph.dwHow        = DIPH_BYID; 
	propRange.diph.dwObj        = instance->dwType;
	propRange.lMin              = -1000; 
	propRange.lMax              = +1000; 
    
	// Set the range for the axis
	for (short i = 0; i<wiDirectInput::connectedJoys; i++)
		if (FAILED(joystick[i]->SetProperty(DIPROP_RANGE, &propRange.diph))) {
			return DIENUM_STOP;
		}

	return DIENUM_CONTINUE;
}


HRESULT wiDirectInput::Initialize(HINSTANCE hinstance, HWND hwnd)
{
	HRESULT result;


	// Initialize the main direct input interface.
	result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, NULL);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	//// Initialize the direct input interface for the keyboard.
	//result = m_directInput->CreateDevice(GUID_SysKeyboard, &m_keyboard, NULL);
	//if(FAILED(result))
	//{
	//	return E_FAIL;
	//}

	//// Set the data format.  In this case since it is a keyboard we can use the predefined data format.
	//result = m_keyboard->SetDataFormat(&c_dfDIKeyboard);
	//if(FAILED(result))
	//{
	//	return E_FAIL;
	//}

	//// Set the cooperative level of the keyboard to not share with other programs.
	//result = m_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	//if(FAILED(result))
	//{
	//	return E_FAIL;
	//}

	//// Now acquire the keyboard.
	//result = m_keyboard->Acquire();
	//if(FAILED(result))
	//{
	//	return E_FAIL;
	//}
	//m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);

	InitJoy(hwnd);

	return S_OK;
}
HRESULT wiDirectInput::InitJoy(HWND hwnd){
	HRESULT result;
	// Look for the first simple joystick we can find.
	if (FAILED(result = m_directInput->EnumDevices(DI8DEVCLASS_GAMECTRL, ::enumCallback, NULL, DIEDFL_ATTACHEDONLY))) {
		return result;
	}
	for(short i=0;i<connectedJoys;i++){

		DIDEVCAPS capabilities;

		// Set the data format to "simple joystick" - a predefined data format 
		//
		// A data format specifies which controls on a device we are interested in,
		// and how they should be reported. This tells DInput that we will be
		// passing a DIJOYSTATE2 structure to IDirectInputDevice::GetDeviceState().
		if (FAILED(result = joystick[i]->SetDataFormat(&c_dfDIJoystick2))) {
			return result;
		}

		// Set the cooperative level to let DInput know how this device should
		// interact with the system and with other DInput applications.
		if (FAILED(result = joystick[i]->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND))) {
			return result;
		}

		// Determine how many axis the joystick has (so we don't error out setting
		// properties for unavailable axis)
		capabilities.dwSize = sizeof(DIDEVCAPS);
		if (FAILED(result = joystick[i]->GetCapabilities(&capabilities))) {
			return result;
		}

		// Enumerate the axes of the joyctick and set the range of each axis. Note:
		// we could just use the defaults, but we're just trying to show an example
		// of enumerating device objects (axes, buttons, etc.).
		if (FAILED(result = joystick[i]->EnumObjects(::enumAxesCallback, NULL, DIDFT_AXIS))) 
			return result;
	}
	
	

	return S_OK;
}

void wiDirectInput::Shutdown()
{
	// Release the keyboard.
	if(m_keyboard)
	{
		m_keyboard->Unacquire();
		m_keyboard->Release();
		m_keyboard = 0;
	}
	// Release Joypad
	if(joystick){
		for(short i=0;i<connectedJoys;i++)
			if(joystick[i]){
				joystick[i]->Unacquire();
				joystick[i]->Release();
				joystick[i] = 0;
			}
	}
	// Release the main interface to direct input.
	if(m_directInput)
	{
		m_directInput->Release();
		m_directInput = 0;
	}


	return;
}

bool wiDirectInput::Frame()
{
	//if(m_keyboard==nullptr)
	//	return false;
	//// Read the keyboard device.
	//HRESULT result = m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);
	//if(FAILED(result))
	//{
	//	// If the keyboard lost focus or was not acquired then try to get control back.
	//	if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
	//	{
	//		m_keyboard->Acquire();
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}
	for(short i=0;i<connectedJoys;i++)
		poll(joystick[i],&joyState[i]);
		
	return true;
}

bool wiDirectInput::IsKeyDown(INT code)
{
	if(m_keyboardState[code] & 0x80)
	{
		return true;
	}

	return false;
}
int wiDirectInput::GetPressedKeys()
{
	int ret=0;
	for(int i=0;i<256;i++)
		if(m_keyboardState[i] & 0x80)
		{
			ret+=i;
		}

	return ret;
}



HRESULT wiDirectInput::poll(IDirectInputDevice8* joy, DIJOYSTATE2 *js)
{
    HRESULT     hr;

    if (joystick == NULL) {
        return S_OK;
    }


    // Poll the device to read the current state
    hr = joy->Poll(); 
    if (FAILED(hr)) {
        // DInput is telling us that the input stream has been
        // interrupted. We aren't tracking any state between polls, so
        // we don't have any special reset that needs to be done. We
        // just re-acquire and try again.
        hr = joy->Acquire();
        while (hr == DIERR_INPUTLOST) {
            hr = joy->Acquire();
        }

        // If we encounter a fatal error, return failure.
        if ((hr == DIERR_INVALIDPARAM) || (hr == DIERR_NOTINITIALIZED)) {
            return E_FAIL;
        }

        // If another application has control of this device, return successfully.
        // We'll just have to wait our turn to use the joystick.
        if (hr == DIERR_OTHERAPPHASPRIO) {
            return S_OK;
        }
    }

    // Get the input's device state
    if (FAILED(hr = joy->GetDeviceState(sizeof(DIJOYSTATE2), js))) {
        return hr; // The device should have been acquired during the Poll()
    }

    return S_OK;
}

bool wiDirectInput::isButtonDown(short pIndex, unsigned int buttoncode)
{
	if(connectedJoys && buttoncode < 128)
		if(joyState[pIndex].rgbButtons[buttoncode-1]!=0)
			return true;
	return false;
}
DWORD wiDirectInput::getDirections(short pIndex)
{
	DWORD buttons=0;
	if(connectedJoys)
		for(short i=0;i<4;i++){
			//if((LOWORD(joyState[pIndex].rgdwPOV[i]) != 0xFFFF))
			if(buttons+=joyState[pIndex].rgdwPOV[i] != DIRECTINPUT_POV_IDLE)
				buttons+=joyState[pIndex].rgdwPOV[i];
		}
	return buttons;
}

#endif
