#pragma once
#include "CommonInclude.h"
#include "wiInput.h"

namespace wiXInput
{
	// Call once per frame to read and update controller states
	void Update();

	// Returns how many gamepads can Xinput handle
	int GetMaxControllerCount();

	// Returns whether the controller identified by index parameter is available or not.
	//	Id state parameter is not nullptr, and the controller is available, the state will be written into it
	bool GetControllerState(wiInput::ControllerState* state, int index);

	// Sends feedback data for the controller identified by index parameter to output
	void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index);
}
