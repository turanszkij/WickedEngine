#pragma once
#include "CommonInclude.h"
#include <Xinput.h>


#if (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
#pragma comment(lib,"xinput.lib")
#else
#pragma comment(lib,"xinput9_1_0.lib")
#endif


#define MAX_CONTROLLERS 4  // XInput handles up to 4 controllers 
#define INPUT_DEADZONE  ( 0.24f * FLOAT(0x7FFF) )  // Default to 24% of the +/- 32767 range.   This is a reasonable default value but can be altered if needed.

class wiXInput
{
private:

	struct CONTROLLER_STATE
	{
		XINPUT_STATE state;
		bool bConnected;
	};

public:
	wiXInput();
	~wiXInput();
	HRESULT UpdateControllerState();
	DWORD	GetButtons(SHORT);
	DWORD	GetDirections(short);
	bool	isButtonDown(short,DWORD);
	void	CleanUp();

	CONTROLLER_STATE		controllers[MAX_CONTROLLERS];
	WCHAR					g_szMessage[4][1024];
	bool					g_bDeadZoneOn;
};

