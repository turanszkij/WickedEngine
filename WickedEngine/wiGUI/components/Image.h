#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	// Image display widget
	class Image : public Widget
	{
	public:
		void Create(const std::string& name);

		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		const char* GetWidgetTypeName() const override { return "Image"; }
	};
}
