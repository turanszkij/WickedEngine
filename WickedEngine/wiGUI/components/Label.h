#pragma once

/**

 * @file
 * The `wi::gui::Label` widget.
 *
 * A static text box that renders its text within its bounds, with optional word
 * wrapping, auto-fit-to-text height, and a scrollbar when the text overflows.
 */

#include "wiGUI/components/ScrollBar.h"

namespace wi::gui
{
	/**
	 * Static box that displays text.
	 *
	 * Renders the widget's text (see @ref Widget::SetText) inside its bounds,
	 * honoring the font's horizontal/vertical alignment and the configured
	 * margins. Text can optionally be word-wrapped (@ref SetWrapEnabled) and
	 * the widget height can auto-fit the text (@ref SetFitTextEnabled). When
	 * the text is taller than the box, the embedded @ref scrollbar lets the
	 * user scroll.
	 */
	class Label : public Widget
	{
	protected:
		/** Whether text is word-wrapped to the widget width. */
		bool wrap_enabled = true;

		/** Whether the widget height auto-fits the text height. */
		bool fittext_enabled = false;
	public:
		/**
		 * Initializes the label with default size, text and scrollbar styling.
		 *
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(const std::string& name);

		/**
		 * Lays out the text, handles scrolling, and updates the scrollbar.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the background, text, optional shadow and scrollbar.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Sets the color of the label and its scrollbar.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the label and its scrollbar.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("Label").
		 */
		const char* GetWidgetTypeName() const override { return "Label"; }

		/** Width of the scrollbar shown when text overflows. */
		float scrollbar_width = 18;

		/** Vertical scrollbar used when the text overflows the box. */
		ScrollBar scrollbar;

		/**
		 * Enables or disables word wrapping.
		 *
		 * @param[in] value - true to wrap text to the widget width.
		 */
		void SetWrapEnabled(bool value) { wrap_enabled = value; }

		/**
		 * Enables or disables auto-fitting the height to the text.
		 *
		 * @param[in] value - true to resize the widget height to the text.
		 */
		void SetFitTextEnabled(bool value) { fittext_enabled = value; }

		/** Left inset between the box edge and the text (pixels). */
		float margin_left = 0;

		/** Right inset between the box edge and the text (pixels). */
		float margin_right = 0;

		/** Top inset between the box edge and the text (pixels). */
		float margin_top = 0;

		/** Bottom inset between the box edge and the text (pixels). */
		float margin_bottom = 0;
	};
}
