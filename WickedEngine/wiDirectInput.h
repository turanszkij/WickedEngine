#pragma once

#define POV_IDLE 4294967292
#define POV_UP 1
#define POV_UPRIGHT 4501
#define POV_RIGHT 9001
#define POV_RIGHTDOWN 13501
#define POV_DOWN 18001
#define POV_DOWNLEFT 22501
#define POV_LEFT 27001
#define POV_LEFTUP 31501

#ifndef WINSTORE_SUPPORT
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#pragma comment(lib,"dinput8.lib")

#else
#include <Windows.h>
#endif

class wiDirectInput
{
#ifndef WINSTORE_SUPPORT
	friend BOOL CALLBACK enumCallback(const DIDEVICEINSTANCE* instance, VOID* context);
	friend BOOL CALLBACK enumAxesCallback(const DIDEVICEOBJECTINSTANCE* instance, VOID* context);
public:
	wiDirectInput(HINSTANCE, HWND);

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

#else
public:
	wiDirectInput(HINSTANCE hinst, HWND hwnd){}
	void Shutdown(){}
	bool Frame(){ return false; }
	bool IsKeyDown(INT){ return false; }
	int GetPressedKeys(){ return 0; }
	bool isButtonDown(short pIndex, unsigned int buttoncode){ return false; }
	DWORD getDirections(short pIndex){ return 0; }
	static HRESULT InitJoy(HWND hwnd){ return E_FAIL; }
#endif
};

