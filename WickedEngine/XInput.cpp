#include "XInput.h"


XInput::XInput(void)
{
	g_bDeadZoneOn = true;
	for(int i=0;i<MAX_CONTROLLERS;++i){
		controllers[i] = CONTROLLER_STATE();
		controllers[i].bConnected=false;
	}
}

HRESULT XInput::UpdateControllerState()
{
    DWORD dwResult = 0;
    for( DWORD i = 0; i < MAX_CONTROLLERS; i++ )
    {
        // Simply get the state of the controller from XInput.
		//dwResult = XInputGetState( i, &controllers[i].state );

        if( dwResult == ERROR_SUCCESS )
            controllers[i].bConnected = true;
        else
            controllers[i].bConnected = false;
    }


    return S_OK;
}

DWORD XInput::GetButtons(SHORT newPlayerIndex)
{
	//if(controllers[newPlayerIndex].bConnected)
	//	return controllers[newPlayerIndex].state.Gamepad.wButtons;
	return 0;
}
DWORD XInput::GetDirections(short newPlayerIndex){
	//if(controllers[newPlayerIndex].bConnected)
	//	return controllers[newPlayerIndex].state.Gamepad.wButtons;
	return 0;
}
bool XInput::isButtonDown(short newPlayerIndex,DWORD checkforButton){
	//if(controllers[newPlayerIndex].bConnected)
	//	return controllers[newPlayerIndex].state.Gamepad.wButtons & checkforButton;
	return false;
}

void XInput::CleanUp()
{
	delete(this);
}