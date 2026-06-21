#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	// Clickable, draggable box
	class Button : public Widget
	{
	protected:
		std::function<void(const EventArgs& args)> onClick;
		std::function<void(const EventArgs& args)> onRightClick;
		std::function<void(const EventArgs& args)> onDragStart;
		std::function<void(const EventArgs& args)> onDrag;
		std::function<void(const EventArgs& args)> onDragEnd;
		XMFLOAT2 dragStart = XMFLOAT2(0, 0);
		XMFLOAT2 prevPos = XMFLOAT2(0, 0);
		bool disableClicking = false;
	public:
		void Create(const std::string& name);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Button"; }

		void OnClick(std::function<void(const EventArgs& args)> func);
		void OnRightClick(std::function<void(const EventArgs& args)> func);
		void OnDragStart(std::function<void(const EventArgs& args)> func);
		void OnDrag(std::function<void(const EventArgs& args)> func);
		void OnDragEnd(std::function<void(const EventArgs& args)> func);

		wi::SpriteFont font_description;
		void SetDescription(const std::string& desc) { font_description.SetText(desc); }
		const std::string GetDescription() const { return font_description.GetTextA(); }

		void DisableClickForCurrentDragOperation() { disableClicking = true; }
	};
}
