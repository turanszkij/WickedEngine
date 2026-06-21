#pragma once
#include "wiGUI/components/TextInputField.h"

namespace wi::gui
{
	// Define an interval and slide the control along it
	class Slider : public Widget
	{
	protected:
		std::function<void(const EventArgs& args)> onSlide;
		float start = 0, end = 1;
		float step = 1000;
		float value = 0;
	public:
		// start : slider minimum value
		// end : slider maximum value
		// defaultValue : slider default Value
		// step : slider step size
		void Create(float start, float end, float defaultValue, float step, const std::string& name);

		wi::Sprite sprites_knob[WIDGETSTATE_COUNT];

		void SetValue(float value);
		void SetValue(int value);
		float GetValue() const;
		void SetRange(float start, float end);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Slider"; }
		WIDGETSTATE GetState() const override { return std::max(state, valueInputField.GetState()); };

		void OnSlide(std::function<void(const EventArgs& args)> func);

		TextInputField valueInputField;
	};
}
