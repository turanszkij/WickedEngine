#pragma once

/**
 * @file
 * The `wi::gui::CheckBox` widget.
 *
 * A two-state (checked / unchecked) clickable box that toggles on click and
 * raises a callback, drawing a configurable glyph (or a default square) for the
 * checked state.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Two-state clickable box.
	 *
	 * Toggles between checked and unchecked on click and raises @ref OnClick
	 * with the new state in `EventArgs::bValue`. When checked it draws, in
	 * priority order, this box's own check glyph (@ref SetCheckText), the
	 * global check glyph (@ref SetCheckTextGlobal), or a simple filled square.
	 * An optional glyph can also be drawn for the unchecked state
	 * (@ref SetUnCheckText).
	 */
	class CheckBox :public Widget
	{
	protected:
		/** Callback invoked on toggle; `EventArgs::bValue` is the new state. */
		std::function<void(const EventArgs& args)> onClick;

		/** Current checked state. */
		bool checked = false;

		/** Glyph drawn when checked (empty falls back to global/square). */
		std::wstring check_text;

		/** Glyph drawn when unchecked (empty draws nothing). */
		std::wstring uncheck_text;
	public:
		/**
		 * Initializes the check box with default size, text and callback.
		 *
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(const std::string& name);

		/**
		 * Sets the checked state.
		 *
		 * @param[in] value - true for checked, false for unchecked.
		 */
		void SetCheck(bool value);

		/**
		 * Returns the current checked state.
		 */
		bool GetCheck() const;

		/**
		 * Updates interaction state and toggles on click.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the box, its shadow, and the check/uncheck glyph.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Returns the widget's type name ("CheckBox").
		 */
		const char* GetWidgetTypeName() const override { return "CheckBox"; }

		/**
		 * Sets the toggle callback.
		 *
		 * @param[in] func - Callback invoked on toggle.
		 */
		void OnClick(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the global check glyph used by all check boxes.
		 *
		 * Used as a fallback for any check box that has no own check glyph set
		 * via @ref SetCheckText.
		 *
		 * @param[in] text - Glyph / symbol text (UTF-8).
		 */
		static void SetCheckTextGlobal(const std::string& text);

		/**
		 * Sets this box's check glyph (drawn when checked).
		 *
		 * @param[in] text - Glyph / symbol text (UTF-8).
		 */
		void SetCheckText(const std::string& text);

		/**
		 * Sets this box's uncheck glyph (drawn when unchecked).
		 *
		 * @param[in] text - Glyph / symbol text (UTF-8).
		 */
		void SetUnCheckText(const std::string& text);
	};
}
