#pragma once
#include "CommonInclude.h"
#include "wiPlatform.h"
#include "wiMath.h"

namespace wi
{
	// The canvas specifies a DPI-aware drawing area
	struct Canvas
	{
		virtual ~Canvas() = default;

		uint32_t width = 0;
		uint32_t height = 0;
		float dpi = 96;
		float scaling = 1; // custom DPI scaling factor (optional)
		
		// Safe area insets in logical coordinates:
		//	the safe area can indicate an offset from the borders of the canvas from where it is safe to put important visual elements that will be always well-visible
		float window_inset_left = 0;
		float window_inset_right = 0;
		float window_inset_top = 0;
		float window_inset_bottom = 0;

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
			window_inset_left = windowprops.inset_left;
			window_inset_right = windowprops.inset_right;
			window_inset_top = windowprops.inset_top;
			window_inset_bottom = windowprops.inset_bottom;
		}

		// How many pixels there are per inch
		constexpr float GetDPI() const { return dpi * scaling; }
		// The scaling factor between logical and physical coordinates
		constexpr float GetDPIScaling() const { return GetDPI() / 96.f; }
		// Convert from logical to physical coordinates
		constexpr uint32_t LogicalToPhysical(float logical) const { return uint32_t(logical * GetDPIScaling()); }
		// Convert from physical to logical coordinates
		constexpr float PhysicalToLogical(float physical) const { return physical / GetDPIScaling(); }
		constexpr float PhysicalToLogical(uint32_t physical) const { return PhysicalToLogical(float(physical)); }
		constexpr float PhysicalToLogical(int physical) const { return PhysicalToLogical(float(physical)); }
		// Returns native resolution width in pixels:
		//	Use this for texture allocations
		//	Use this for scissor, viewport
		constexpr uint32_t GetPhysicalWidth() const { return width; }
		// Returns native resolution height in pixels:
		//	Use this for texture allocations
		//	Use this for scissor, viewport
		constexpr uint32_t GetPhysicalHeight() const { return height; }
		// Returns the width with DPI scaling applied (subpixel size):
		//	Use this for logic and positioning drawable elements
		constexpr float GetLogicalWidth() const { return PhysicalToLogical(GetPhysicalWidth()); }
		// Returns the height with DPI scaling applied (subpixel size):
		//	Use this for logic and positioning drawable elements
		constexpr float GetLogicalHeight() const { return PhysicalToLogical(GetPhysicalHeight()); }
		// Returns projection matrix that maps logical to physical space
		//	Use this to render to a graphics viewport
		inline XMMATRIX GetProjection() const
		{
			return XMMatrixOrthographicOffCenterLH(0, (float)GetLogicalWidth(), (float)GetLogicalHeight(), 0, -1, 1);
		}
		// Returns the aspect (width/height)
		constexpr float GetAspect() const
		{
			if (height == 0)
				return float(width);
			return float(width) / float(height);
		}
		
		// Safety inset for visual elements from the left side in logical coordinates:
		constexpr float GetSafeInsetLeftLogical() const { return window_inset_left / scaling; }
		// Safety inset for visual elements from the right side in logical coordinates:
		constexpr float GetSafeInsetRightLogical() const { return window_inset_right / scaling; }
		// Safety inset for visual elements from the top side in logical coordinates:
		constexpr float GetSafeInsetTopLogical() const { return window_inset_top / scaling; }
		// Safety inset for visual elements from the bottom side in logical coordinates:
		constexpr float GetSafeInsetBottomLogical() const { return window_inset_bottom / scaling; }
		
		// Safety inset for visual elements from the left side in physical coordinates:
		constexpr uint32_t GetSafeInsetLeftPhysical() const { return LogicalToPhysical(GetSafeInsetLeftLogical()); }
		// Safety inset for visual elements from the left side in physical coordinates:
		constexpr uint32_t GetSafeInsetRightPhysical() const { return LogicalToPhysical(GetSafeInsetRightLogical()); }
		// Safety inset for visual elements from the left side in physical coordinates:
		constexpr uint32_t GetSafeInsetTopPhysical() const { return LogicalToPhysical(GetSafeInsetTopLogical()); }
		// Safety inset for visual elements from the left side in physical coordinates:
		constexpr uint32_t GetSafeInsetBottomPhysical() const { return LogicalToPhysical(GetSafeInsetBottomLogical()); }
	};

}
