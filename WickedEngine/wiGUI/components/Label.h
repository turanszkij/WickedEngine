#pragma once
#include "wiGUI/components/ScrollBar.h"

namespace wi::gui
{
	// Static box that holds text
	class Label : public Widget
	{
	protected:
		bool wrap_enabled = true;
		bool fittext_enabled = false;
	public:
		void Create(const std::string& name);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Label"; }

		float scrollbar_width = 18;
		ScrollBar scrollbar;

		void SetWrapEnabled(bool value) { wrap_enabled = value; }
		void SetFitTextEnabled(bool value) { fittext_enabled = value; }

		float margin_left = 0;
		float margin_right = 0;
		float margin_top = 0;
		float margin_bottom = 0;
	};
}
