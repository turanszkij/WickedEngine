#pragma once

namespace wi::initializer
{
	// Initializes systems and blocks CPU until it is complete
	void InitializeComponentsImmediate();
	// Begins initializing systems, but doesn't block CPU. Check completion status with IsInitializeFinished()
	void InitializeComponentsAsync();
	// Check if systems have been initialized or not
	bool IsInitializeFinished();
}
