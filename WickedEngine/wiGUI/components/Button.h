#pragma once

/**
 * @file
 * The `wi::gui::Button` widget.
 *
 * A clickable, draggable box that raises callbacks for left/right clicks and
 * for the drag lifecycle (start / move / end), with an optional description
 * label drawn beside it.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Clickable, draggable box.
	 *
	 * Tracks pointer interaction and raises user callbacks: @ref OnClick and
	 * @ref OnRightClick on release within bounds, and @ref OnDragStart /
	 * @ref OnDrag / @ref OnDragEnd across a press-move-release gesture. A
	 * separate @ref font_description label can be placed beside the button
	 * (e.g. as a caption). A click can be suppressed for the current gesture
	 * via
	 * @ref DisableClickForCurrentDragOperation.
	 */
	class Button : public Widget
	{
	protected:
		/** Callback invoked on a left click (release within bounds). */
		std::function<void(const EventArgs& args)> onClick;

		/** Callback invoked on a right click. */
		std::function<void(const EventArgs& args)> onRightClick;

		/** Callback invoked when a drag gesture begins. */
		std::function<void(const EventArgs& args)> onDragStart;

		/** Callback invoked each frame while dragging. */
		std::function<void(const EventArgs& args)> onDrag;

		/** Callback invoked when a drag gesture ends. */
		std::function<void(const EventArgs& args)> onDragEnd;

		/** Pointer position where the current drag started. */
		XMFLOAT2 dragStart = XMFLOAT2(0, 0);

		/** Pointer position from the previous frame (for drag deltas). */
		XMFLOAT2 prevPos = XMFLOAT2(0, 0);

		/** When true, the click callback is suppressed for this gesture. */
		bool disableClicking = false;
	public:
		/**
		 * Initializes the button with default size, text and empty callbacks.
		 *
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(const std::string& name);

		/**
		 * Updates interaction state and raises click/drag callbacks.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the button background, text, shadow and description label.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Applies a theme to the button and its description label.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("Button").
		 */
		const char* GetWidgetTypeName() const override { return "Button"; }

		/**
		 * Sets the left-click callback.
		 *
		 * @param[in] func - Callback invoked on a left click.
		 */
		void OnClick(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the right-click callback.
		 *
		 * @param[in] func - Callback invoked on a right click.
		 */
		void OnRightClick(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the drag-start callback.
		 *
		 * @param[in] func - Callback invoked when a drag begins.
		 */
		void OnDragStart(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the per-frame drag callback.
		 *
		 * @param[in] func - Callback invoked each frame while dragging.
		 */
		void OnDrag(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the drag-end callback.
		 *
		 * @param[in] func - Callback invoked when a drag ends.
		 */
		void OnDragEnd(std::function<void(const EventArgs& args)> func);

		/** Optional description label drawn beside the button. */
		wi::SpriteFont font_description;

		/**
		 * Sets the description label text.
		 *
		 * @param[in] desc - Description text.
		 */
		void SetDescription(const std::string& desc) {
			font_description.SetText(desc);
		}

		/**
		 * Returns the description label text as an ASCII string.
		 */
		const std::string GetDescription() const {
			return font_description.GetTextA();
		}

		/**
		 * Suppresses the click callback for the current drag gesture.
		 *
		 * Useful when a drag handler has already consumed the interaction and
		 * the trailing release should not also fire @ref OnClick.
		 */
		void DisableClickForCurrentDragOperation() { disableClicking = true; }
	};
}
