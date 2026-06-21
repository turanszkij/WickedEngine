#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	class GUI
	{
	private:
		wi::vector<Widget*> widgets;
		bool focus = false;
		bool visible = true;
	public:

		void Update(const wi::Canvas& canvas, float dt);
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		void AddWidget(Widget* widget);
		void RemoveWidget(Widget* widget);
		Widget* GetWidget(const std::string& name);

		// returns true if any gui element has the focus
		bool HasFocus() const;
		// returns true if text input is happening
		bool IsTyping() const;

		void SetVisible(bool value) { visible = value; }
		bool IsVisible() const { return visible; }

		void SetColor(wi::Color color, int id = -1);
		void SetImage(const wi::Resource& resource, int id = -1);
		void SetShadowColor(wi::Color color);
		void SetTheme(const Theme& theme, int id = -1);

		void ExportLocalization(wi::Localization& localization) const;
		void ImportLocalization(const wi::Localization& localization);
	};
}
