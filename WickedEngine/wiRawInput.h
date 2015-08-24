#pragma once
#include "CommonInclude.h"

class wiRawInput
{
public:
#ifndef WINSTORE_SUPPORT
	wiRawInput(HWND hWnd = NULL);
	~wiRawInput();

	//for generic joypad support
	bool RegisterJoys(HWND hWnd);
	//disables legacy mouse and keyboard support
	bool RegisterKeyboardMouse(HWND hWnd);
	//use this in WndProc in case of WM_INPUT event
	void RetrieveData(LPARAM lParam);
	//read buffered data
	void RetrieveBufferedData();

	RAWINPUT raw;

#else
	//Raw input is not available!
	wiRawInput(){}
	//Raw input is not available!
	void RetrieveBufferedData(){}
#endif //WINSTORE_SUPPORT
};

