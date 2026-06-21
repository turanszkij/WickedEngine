#pragma once
#include "wiGraphicsDevice.h"

#include <cstdint>

namespace wi::gui
{
	// Shared GUI render state, created lazily on first use and reloaded on
	// EVENT_RELOAD_SHADERS. Defined in GUI.cpp.
	struct InternalState
	{
		wi::graphics::PipelineState PSO_colored;

		InternalState();
		void LoadShaders();
	};
	InternalState& gui_internal();

	// Cross-widget interaction flags shared by all widgets.

	// This is used so that elements that support scroll could disable other scrolling elements:
	//	As opposed to click and other interaction types, we don't want to disable scroll on every focused widget
	//	because that would block scrolling the parent if a child element is hovered
	inline bool scroll_allowed = true;
	inline bool typing_active = false;
}
