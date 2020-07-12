#pragma once
#include "CommonInclude.h"
#include "wiInput.h"

#ifdef SDL2
#include <SDL2/SDL.h>
#endif

namespace wiSDLInput
{
	// Call this once to register raw input devices
	void Initialize();

	// Updates the state of raw input devices, call once per frame
	void Update();

	// Writes the keyboard state into state parameter
	void GetKeyboardState(wiInput::KeyboardState* state);

	// Writes the mouse state into state parameter
	void GetMouseState(wiInput::MouseState* state);

	// Returns how many controller devices have received input ever. This doesn't correlate with which ones are currently available
	int GetMaxControllerCount();

	// Returns whether the controller identified by index parameter is available or not
	//	Id state parameter is not nullptr, and the controller is available, the state will be written into it
	bool GetControllerState(wiInput::ControllerState* state, int index);

	// Sends feedback data for the controller identified by index parameter to output
	void SetControllerFeedback(const wiInput::ControllerFeedback& data, int index);
}
