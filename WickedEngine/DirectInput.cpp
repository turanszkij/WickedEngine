#include "DirectInput.h"

#ifndef WINSTORE_SUPPORT

DirectInput::DirectInput(HINSTANCE hinstance, HWND hwnd)
{
	for(int i=0;i<256;++i){
		m_keyboardState[i]=0;
	}
	Initialize(hinstance, hwnd);
}


IDirectInputDevice8* m_keyboard = 0;
IDirectInputDevice8* joystick[2] = {0,0};
IDirectInput8*  m_directInput = 0;
	short DirectInput::connectedJoys=0;

BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context)
{
    HRESULT hr;

    // Obtain an interface to the enumerated joystick.
    hr = m_directInput->CreateDevice(instance->guidInstance, &joystick[DirectInput::connectedJoys], NULL);

    // If it failed, then we can't use this joystick. (Maybe the user unplugged
    // it while we were in the middle of enumerating it.)
    if (FAILED(hr)) { 
        return DIENUM_CONTINUE;
    }

    // Stop enumeration. Note: we're just taking the first joystick we get. You
    // could store all the enumerated joysticks and let the user pick.
	DirectInput::connectedJoys++;
	if(DirectInput::connectedJoys<2)
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
	for(short i=0;i<DirectInput::connectedJoys;i++)
		if (FAILED(joystick[i]->SetProperty(DIPROP_RANGE, &propRange.diph))) {
			return DIENUM_STOP;
		}

	return DIENUM_CONTINUE;
}


HRESULT DirectInput::Initialize(HINSTANCE hinstance, HWND hwnd)
{
	HRESULT result;


	// Initialize the main direct input interface.
	result = DirectInput8Create(hinstance, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&m_directInput, NULL);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Initialize the direct input interface for the keyboard.
	result = m_directInput->CreateDevice(GUID_SysKeyboard, &m_keyboard, NULL);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Set the data format.  In this case since it is a keyboard we can use the predefined data format.
	result = m_keyboard->SetDataFormat(&c_dfDIKeyboard);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Set the cooperative level of the keyboard to not share with other programs.
	result = m_keyboard->SetCooperativeLevel(hwnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
	if(FAILED(result))
	{
		return E_FAIL;
	}

	// Now acquire the keyboard.
	result = m_keyboard->Acquire();
	if(FAILED(result))
	{
		return E_FAIL;
	}
	m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);

	InitJoy(hwnd);

	return S_OK;
}
HRESULT DirectInput::InitJoy(HWND hwnd){
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

void DirectInput::Shutdown()
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

bool DirectInput::Frame()
{
	if(m_keyboard==nullptr)
		return false;
	// Read the keyboard device.
	HRESULT result = m_keyboard->GetDeviceState(sizeof(m_keyboardState), (LPVOID)&m_keyboardState);
	if(FAILED(result))
	{
		// If the keyboard lost focus or was not acquired then try to get control back.
		if((result == DIERR_INPUTLOST) || (result == DIERR_NOTACQUIRED))
		{
			m_keyboard->Acquire();
		}
		else
		{
			return false;
		}
	}
	for(short i=0;i<connectedJoys;i++)
		poll(joystick[i],&joyState[i]);
		
	return true;
}

bool DirectInput::IsKeyDown(INT code)
{
	if(m_keyboardState[code] & 0x80)
	{
		return true;
	}

	return false;
}
int DirectInput::GetPressedKeys()
{
	int ret=0;
	for(int i=0;i<256;i++)
		if(m_keyboardState[i] & 0x80)
		{
			ret+=i;
		}

	return ret;
}



HRESULT DirectInput::poll(IDirectInputDevice8* joy,DIJOYSTATE2 *js)
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

bool DirectInput::isButtonDown(short pIndex, unsigned int buttoncode)
{
	if(connectedJoys)
		if(joyState[pIndex].rgbButtons[buttoncode-1]!=0)
			return true;
	return false;
}
DWORD DirectInput::getDirections(short pIndex)
{
	DWORD buttons=0;
	if(connectedJoys)
		for(short i=0;i<4;i++){
			//if((LOWORD(joyState[pIndex].rgdwPOV[i]) != 0xFFFF))
			if(buttons+=joyState[pIndex].rgdwPOV[i]!=POV_IDLE)
				buttons+=joyState[pIndex].rgdwPOV[i];
		}
	return buttons;
}

#endif
