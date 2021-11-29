#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"
#include "wiWidget.h"
#include "wiVector.h"

namespace wi
{
	class GUI
	{
	private:
		wi::vector<wi::widget::Widget*> widgets;
		bool focus = false;
		bool visible = true;
	public:

		void Update(const wi::Canvas& canvas, float dt);
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		void AddWidget(wi::widget::Widget* widget);
		void RemoveWidget(wi::widget::Widget* widget);
		wi::widget::Widget* GetWidget(const std::string& name);

		// returns true if any gui element has the focus
		bool HasFocus();

		void SetVisible(bool value) { visible = value; }
		bool IsVisible() { return visible; }
	};
}
