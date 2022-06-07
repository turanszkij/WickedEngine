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
		float scaling = 1; // custom DPI scaling factor (optional)

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
		inline float GetDPI() const { return dpi * scaling; }
		// The scaling factor between logical and physical coordinates
		inline float GetDPIScaling() const { return GetDPI() / 96.f; }
		// Convert from logical to physical coordinates
		inline uint32_t LogicalToPhysical(float logical) const { return uint32_t(logical * GetDPIScaling()); }
		// Convert from physical to logical coordinates
		inline float PhysicalToLogical(uint32_t physical) const { return float(physical) / GetDPIScaling(); }
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
		inline float GetLogicalWidth() const { return PhysicalToLogical(GetPhysicalWidth()); }
		// Returns the height with DPI scaling applied (subpixel size):
		//	Use this for logic and positioning drawable elements
		inline float GetLogicalHeight() const { return PhysicalToLogical(GetPhysicalHeight()); }
		// Returns projection matrix that maps logical to physical space
		//	Use this to render to a graphics viewport
		inline XMMATRIX GetProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetLogicalWidth(), (float)GetLogicalHeight(), 0, -1, 1);
		}
	};

}
