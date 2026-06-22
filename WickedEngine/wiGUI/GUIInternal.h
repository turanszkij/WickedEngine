#pragma once

/**
 * @file
 * Module-private shared state for the `wi::gui` widgets.
 *
 * Holds the render state and cross-widget interaction flags that every widget
 * touches but that are not part of the public widget API. Defined out-of-line
 * in `GUI.cpp`. Not included by the umbrella `wiGUI.h`; only widget `.cpp`
 * files that need this shared state include it directly.
 */

#include "wiGraphicsDevice.h"

#include <cstdint>

namespace wi::gui
{
	/**
	 * Shared GPU render state for the GUI module.
	 *
	 * Created lazily on first use (see @ref gui_internal) and reloaded on the
	 * `EVENT_RELOAD_SHADERS` event. Holds the pipeline state used to draw the
	 * widgets' flat-colored geometry.
	 */
	struct InternalState
	{
		/** Pipeline state for drawing vertex-colored (flat) GUI geometry. */
		wi::graphics::PipelineState PSO_colored;

		/**
		 * Constructs the state and subscribes to shader-reload events.
		 *
		 * Loads the shaders once and registers a handler that reloads them on
		 * `EVENT_RELOAD_SHADERS`.
		 */
		InternalState();

		/**
		 * Builds @ref PSO_colored from the current renderer shaders/states.
		 *
		 * Called on construction and again whenever shaders are reloaded.
		 */
		void LoadShaders();
	};

	/**
	 * Returns the lazily-created, process-wide GUI render state.
	 *
	 * @return Reference to the singleton @ref InternalState.
	 */
	InternalState& gui_internal();

	// Cross-widget interaction flags shared by all widgets.

	/**
	 * Whether scrolling is still available to widgets this frame.
	 *
	 * Lets a widget that consumes a scroll gesture disable scrolling on other
	 * scrollable widgets. Unlike click and other interactions, scroll is not
	 * disabled on every focused widget, because that would block scrolling a
	 * parent while a child element is merely hovered. Reset to `true` at the
	 * start of each @ref GUI::Update.
	 */
	inline bool scroll_allowed = true;

	/**
	 * Whether text input is currently active in any widget.
	 *
	 * Set while a @ref TextInputField (or similar) has keyboard focus, so the
	 * GUI can report that typing is happening and route input accordingly.
	 */
	inline bool typing_active = false;
}
