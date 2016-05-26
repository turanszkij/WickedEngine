#include <Windows.h>
#include "wiXInput.h"


wiXInput::wiXInput()
{
	g_bDeadZoneOn = true;
	for(int i=0;i<MAX_CONTROLLERS;++i){
		controllers[i] = CONTROLLER_STATE();
		controllers[i].bConnected=false;
	}
}
wiXInput::~wiXInput()
{
	CleanUp();
}

HRESULT wiXInput::UpdateControllerState()
{
    DWORD dwResult = 0;
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        // Simply get the state of the controller from XInput.
		dwResult = XInputGetState( i, &controllers[i].state );

        if( dwResult == ERROR_SUCCESS )
            controllers[i].bConnected = true;
        else
            controllers[i].bConnected = false;
    }


    return S_OK;
}

DWORD wiXInput::GetButtons(SHORT newPlayerIndex)
{
	if(controllers[newPlayerIndex].bConnected)
		return controllers[newPlayerIndex].state.Gamepad.wButtons;
	return 0;
}
DWORD wiXInput::GetDirections(short newPlayerIndex){
	if(controllers[newPlayerIndex].bConnected)
		return controllers[newPlayerIndex].state.Gamepad.wButtons;
	return 0;
}
bool wiXInput::isButtonDown(short newPlayerIndex, DWORD checkforButton){
	if(controllers[newPlayerIndex].bConnected)
		return (controllers[newPlayerIndex].state.Gamepad.wButtons & checkforButton) != 0;
	return false;
}

void wiXInput::CleanUp()
{
}