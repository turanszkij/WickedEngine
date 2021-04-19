#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiCanvas.h"
#include "wiWidget.h"

#include <list>

class wiGUI
{
private:
	std::list<wiWidget*> widgets;
	std::vector<wiWidget*> priorityChangeQueue;
	bool focus = false;
	bool visible = true;
public:

	void Update(const wiCanvas& canvas, float dt);
	void Render(const wiCanvas& canvas, wiGraphics::CommandList cmd) const;

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	wiWidget* GetWidget(const std::string& name);

	// returns true if any gui element has the focus
	bool HasFocus();

	void SetVisible(bool value) { visible = value; }
	bool IsVisible() { return visible; }
};

