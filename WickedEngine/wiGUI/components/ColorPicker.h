#pragma once
#include "wiGUI/components/Window.h"
#include "wiGUI/components/TextInputField.h"
#include "wiGUI/components/Slider.h"

namespace wi::gui
{
	// HSV-Color Picker
	class ColorPicker : public Window
	{
	protected:
		std::function<void(const EventArgs& args)> onColorChanged;
		enum COLORPICKERSTATE
		{
			CPS_IDLE,
			CPS_HUE,
			CPS_SATURATION,
		} colorpickerstate = CPS_IDLE;
		float hue = 0.0f;			// [0, 360] degrees
		float saturation = 0.0f;	// [0, 1]
		float luminance = 1.0f;		// [0, 1]

		void FireEvents();
	public:
		void Create(const std::string& name, WindowControls window_controls = WindowControls::ALL);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void ResizeLayout() override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetImage(wi::Resource resource, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "ColorPicker"; }

		wi::Color GetPickColor() const;
		void SetPickColor(wi::Color value);

		void OnColorChanged(std::function<void(const EventArgs& args)> func);

		TextInputField text_R;
		TextInputField text_G;
		TextInputField text_B;
		TextInputField text_H;
		TextInputField text_S;
		TextInputField text_V;
		TextInputField text_hex;
		Slider alphaSlider;
	};
}
