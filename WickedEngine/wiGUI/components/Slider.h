#pragma once

/**
 * @file
 * The `wi::gui::Slider` widget.
 *
 * A horizontal slider that picks a value within a `[start, end]` range by
 * dragging a knob, with an embedded text input field for typing an exact value
 * (including arithmetic expressions and out-of-range values).
 */

#include "wiGUI/components/TextInputField.h"

namespace wi::gui
{
	/**
	 * Selects a value within an interval by sliding a knob.
	 *
	 * Dragging the knob picks a value in `[start, end]`, quantized to @ref step
	 * increments, and raises @ref OnSlide with the value in `EventArgs::fValue`
	 * / `EventArgs::iValue`. An embedded @ref valueInputField lets the user
	 * type an exact value, which may fall outside the current range (the range
	 * expands to include it). The slider is drawn as a base track plus a knob
	 * sprite per @ref WIDGETSTATE.
	 */
	class Slider : public Widget
	{
	protected:
		/** Callback invoked when the value changes via drag or input. */
		std::function<void(const EventArgs& args)> onSlide;

		/** Minimum value of the range. */
		float start = 0;

		/** Maximum value of the range. */
		float end = 1;

		/** Number of discrete steps the knob quantizes to. */
		float step = 1000;

		/** Current value, within `[start, end]`. */
		float value = 0;
	public:
		/**
		 * Initializes the slider with a range, default value and step.
		 *
		 * @param[in] start - Minimum (left) value of the range.
		 * @param[in] end - Maximum (right) value of the range.
		 * @param[in] defaultValue - Initial value.
		 * @param[in] step - Step granularity (clamped to a minimum of 1).
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(
			float start,
			float end,
			float defaultValue,
			float step,
			const std::string& name
		);

		/** Knob sprite for each @ref WIDGETSTATE. */
		wi::Sprite sprites_knob[WIDGETSTATE_COUNT];

		/**
		 * Sets the current value.
		 *
		 * @param[in] value - New value.
		 */
		void SetValue(float value);

		/**
		 * Sets the current value from an integer.
		 *
		 * @param[in] value - New value.
		 */
		void SetValue(int value);

		/**
		 * Returns the current value.
		 */
		[[nodiscard]] float GetValue() const noexcept;

		/**
		 * Sets the range and clamps the current value into it.
		 *
		 * @param[in] start - New minimum value.
		 * @param[in] end - New maximum value.
		 */
		void SetRange(float start, float end);

		/**
		 * Updates the knob, the embedded input field, and the drag interaction.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the base track, knob, text, shadow and input field.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Draws the slider's tooltip and the input field's tooltip.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void RenderTooltip(
			const wi::Canvas& canvas,
			wi::graphics::CommandList cmd
		) const override;

		/**
		 * Sets the color of the slider, its knob/base, and the input field.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; the slider's
		 *                 `WIDGET_ID_SLIDER_*` IDs address its base/knob
		 *                 states. -1 applies to all.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the slider, its knob/base, and the input field.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("Slider").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override {
			return "Slider";
		}

		/**
		 * Returns the most-active state of the slider and its input field.
		 */
		[[nodiscard]] WIDGETSTATE GetState() const override {
			return std::max(state, valueInputField.GetState());
		}

		/**
		 * Sets the slide callback.
		 *
		 * @param[in] func - Callback invoked when the value changes.
		 */
		void OnSlide(std::function<void(const EventArgs& args)> func);

		/** Text input field for typing an exact value. */
		TextInputField valueInputField;
	};
}
