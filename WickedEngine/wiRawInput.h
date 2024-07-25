#pragma once
#include "CommonInclude.h"
#include "wiInput.h"

namespace wi::input::rawinput
{
	// Call this once to register raw input devices
	void Initialize();

	// Updates the state of raw input devices, call once per frame
	void Update();

	// Parse Windows message from message loop. Not necessary to call if you don't use message loop in the application.
	void ParseMessage(void* lparam);

	// Writes the keyboard state into state parameter
	void GetKeyboardState(wi::input::KeyboardState* state);

	// Writes the mouse state into state parameter
	void GetMouseState(wi::input::MouseState* state);

	// Returns how many controller devices have received input ever. This doesn't correlate with which ones are currently available
	int GetMaxControllerCount();

	// Returns whether the controller identified by index parameter is available or not
	//	Id state parameter is not nullptr, and the controller is available, the state will be written into it
	bool GetControllerState(wi::input::ControllerState* state, int index);

	// Sends feedback data for the controller identified by index parameter to output
	void SetControllerFeedback(const wi::input::ControllerFeedback& data, int index);
}
