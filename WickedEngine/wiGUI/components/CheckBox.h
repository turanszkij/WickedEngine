#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	// Two-state clickable box
	class CheckBox :public Widget
	{
	protected:
		std::function<void(const EventArgs& args)> onClick;
		bool checked = false;
		std::wstring check_text;
		std::wstring uncheck_text;
	public:
		void Create(const std::string& name);

		void SetCheck(bool value);
		bool GetCheck() const;

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		const char* GetWidgetTypeName() const override { return "CheckBox"; }

		void OnClick(std::function<void(const EventArgs& args)> func);

		static void SetCheckTextGlobal(const std::string& text);
		void SetCheckText(const std::string& text);
		void SetUnCheckText(const std::string& text);
	};
}
