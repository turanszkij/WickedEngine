#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiMath.h"

namespace wi
{
	// The canvas specifies a DPI-aware drawing area
	struct Canvas
	{
		uint32_t width = 0;
		uint32_t height = 0;
		float dpi = 96;

		// Create a canvas from physical measurements
		inline void init(uint32_t width, uint32_t height, float dpi = 96)
		{
			this->width = width;
			this->height = height;
			this->dpi = dpi;
		}
		// Copy canvas from other canvas
		inline void init(const Canvas& other)
		{
			*this = other;
		}
		// Create the canvas straight from window handle
		inline void init(platform::window_type window)
		{
			platform::WindowProperties windowprops;
			platform::GetWindowProperties(window, &windowprops);
			init((uint32_t)windowprops.width, (uint32_t)windowprops.height, windowprops.dpi);
		}

		// How many pixels there are per inch
		inline float GetDPI() const { return dpi; }
		// The scaling factor between logical and physical coordinates
		inline float GetDPIScaling() const { return GetDPI() / 96.f; }
		// Returns native resolution width in pixels:
		//	Use this for texture allocations
		//	Use this for scissor, viewport
		inline uint32_t GetPhysicalWidth() const { return width; }
		// Returns native resolution height in pixels:
		//	Use this for texture allocations
		//	Use this for scissor, viewport
		inline uint32_t GetPhysicalHeight() const { return height; }
		// Returns the width with DPI scaling applied (subpixel size):
		//	Use this for logic and positioning drawable elements
		inline float GetLogicalWidth() const { return GetPhysicalWidth() / GetDPIScaling(); }
		// Returns the height with DPI scaling applied (subpixel size):
		//	Use this for logic and positioning drawable elements
		inline float GetLogicalHeight() const { return GetPhysicalHeight() / GetDPIScaling(); }
		// Returns projection matrix that maps logical to physical space
		//	Use this to render to a graphics viewport
		inline XMMATRIX GetProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetLogicalWidth(), (float)GetLogicalHeight(), 0, -1, 1);
		}
	};

}
