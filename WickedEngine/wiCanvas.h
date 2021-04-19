#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"

// The canvas specifies a DPI-aware positioning area
struct wiCanvas
{
	int width = 0;
	int height = 0;
	float dpi = 96;

	// Create a canvas from physical measurements
	inline void init(int width, int height, float dpi = 96)
	{
		this->width = width;
		this->height = height;
		this->dpi = dpi;
	}
	// Create the canvas straight from window handle
	inline void init(wiPlatform::window_type window)
	{
		wiPlatform::WindowProperties windowprops;
		wiPlatform::GetWindowProperties(window, &windowprops);
		init(windowprops.width, windowprops.height, windowprops.dpi);
	}

	// How many pixels there are per inch
	inline float GetDPI() const { return dpi; }
	// The scaling factor between logical and physical coordinates
	inline float GetDPIScaling() const { return GetDPI() / 96.f; }
	// Returns native resolution width in pixels:
	//	Use this for texture allocations
	//	Use this for scissor, viewport
	inline int GetPhysicalWidth() const { return width; }
	// Returns native resolution height in pixels:
	//	Use this for texture allocations
	//	Use this for scissor, viewport
	inline int GetPhysicalHeight() const { return height; }
	// Returns the width with DPI scaling applied (subpixel size):
	//	Use this for logic and positioning elements on screen
	inline float GetLogicalWidth() const { return GetPhysicalWidth() / GetDPIScaling(); }
	// Returns the height with DPI scaling applied (subpixel size):
	//	Use this for logic and positioning elements on screen
	inline float GetLogicalHeight() const { return GetPhysicalHeight() / GetDPIScaling(); }
	// Returns projection matrix that maps logical to physical space
	//	Use this to render to a graphics viewport
	inline XMMATRIX GetProjection() const
	{
		return XMMatrixOrthographicOffCenterLH(0, (float)GetLogicalWidth(), (float)GetLogicalHeight(), 0, -1, 1);
	}
};
