#pragma once

/**
 * @file
 * The `wi::gui::ColorPicker` widget.
 *
 * An HSV color picker window: a hue ring and saturation/luminance area for
 * picking visually, plus text inputs for RGB / HSV / hex values and an alpha
 * slider.
 */

#include "wiGUI/components/Window.h"
#include "wiGUI/components/TextInputField.h"
#include "wiGUI/components/Slider.h"

namespace wi::gui
{
	/**
	 * HSV color picker.
	 *
	 * A @ref Window that lets the user pick a color via a hue ring and a
	 * saturation/luminance area, or by typing exact values into the RGB, HSV
	 * and hex text fields; alpha is set with @ref alphaSlider. Changing the
	 * color raises @ref OnColorChanged with the new color in
	 * `EventArgs::color`. The color is stored internally as HSV plus a separate
	 * alpha.
	 */
	class ColorPicker : public Window
	{
	protected:
		/** Callback invoked whenever the picked color changes. */
		std::function<void(const EventArgs& args)> onColorChanged;

		/**
		 * Which part of the picker is being dragged.
		 */
		enum COLORPICKERSTATE
		{
			/** Not being dragged. */
			CPS_IDLE,

			/** Dragging the hue ring. */
			CPS_HUE,

			/** Dragging the saturation/luminance area. */
			CPS_SATURATION,
		} colorpickerstate = CPS_IDLE;

		/** Hue, in `[0, 360]` degrees. */
		float hue = 0.0f;

		/** Saturation, in `[0, 1]`. */
		float saturation = 0.0f;

		/** Luminance, in `[0, 1]`. */
		float luminance = 1.0f;

		/**
		 * Updates the text fields from the current color and fires callbacks.
		 */
		void FireEvents();
	public:
		/**
		 * Initializes the color picker window and its sub-widgets.
		 *
		 * @param[in] name - Widget name; also used as the window title.
		 * @param[in] window_controls - Which window controls to create.
		 */
		void Create(
			const std::string& name,
			WindowControls window_controls = WindowControls::ALL
		);

		/**
		 * Updates the picker interaction and its sub-widgets.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the hue ring, saturation area, sub-widgets and shadow.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Recomputes the layout of the picker's sub-widgets.
		 */
		void ResizeLayout() override;

		/**
		 * Sets the color of the picker window.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Sets the background image of the picker window.
		 *
		 * @param[in] resource - Texture resource to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetImage(wi::Resource resource, int id = -1) override;

		/**
		 * Returns the widget's type name ("ColorPicker").
		 */
		const char* GetWidgetTypeName() const override { return "ColorPicker"; }

		/**
		 * Returns the currently picked color (including alpha).
		 */
		wi::Color GetPickColor() const;

		/**
		 * Sets the picked color (including alpha).
		 *
		 * @param[in] value - New color.
		 */
		void SetPickColor(wi::Color value);

		/**
		 * Sets the color-changed callback.
		 *
		 * @param[in] func - Callback invoked when the color changes.
		 */
		void OnColorChanged(std::function<void(const EventArgs& args)> func);

		/** Red channel text input. */
		TextInputField text_R;

		/** Green channel text input. */
		TextInputField text_G;

		/** Blue channel text input. */
		TextInputField text_B;

		/** Hue text input (degrees). */
		TextInputField text_H;

		/** Saturation text input. */
		TextInputField text_S;

		/** Value/luminance text input. */
		TextInputField text_V;

		/** Hex color text input. */
		TextInputField text_hex;

		/** Alpha (opacity) slider. */
		Slider alphaSlider;
	};
}
