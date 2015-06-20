#pragma once

#include <dinput.h>

#define POV_IDLE 4294967292
#define POV_UP 1
#define POV_UPRIGHT 4501
#define POV_RIGHT 9001
#define POV_RIGHTDOWN 13501
#define POV_DOWN 18001
#define POV_DOWNLEFT 22501
#define POV_LEFT 27001
#define POV_LEFTUP 31501

class DirectInput
{
	friend BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);
	friend BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context);
public:
	DirectInput(HINSTANCE, HWND);

	void Shutdown();
	bool Frame();

	//KEYBOARD
	bool IsKeyDown(INT);
	int GetPressedKeys();

	//JOYSTICK
	bool isButtonDown(short pIndex, unsigned int buttoncode);
	DWORD getDirections(short pIndex);
	
	DIJOYSTATE2 joyState[2];
	static short connectedJoys;

	
	static HRESULT InitJoy(HWND hwnd);

private:
	HRESULT Initialize(HINSTANCE, HWND);
	
	HRESULT poll(IDirectInputDevice8*joy,DIJOYSTATE2 *js);
	unsigned char m_keyboardState[256];
};

