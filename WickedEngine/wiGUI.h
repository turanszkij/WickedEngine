#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiIntersect.h"
#include "wiCanvas.h"

#include <list>

class wiWidget;

class wiGUI
{
	friend class wiWidget;
private:
	std::list<wiWidget*> widgets;
	std::vector<wiWidget*> priorityChangeQueue;
	wiWidget* activeWidget = nullptr;
	bool focus = false;
	bool visible = true;
	XMFLOAT2 pointerpos = XMFLOAT2(0, 0);
	Hitbox2D pointerhitbox;
public:

	void Update(float dt);
	void Render(wiGraphics::CommandList cmd) const;

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	wiWidget* GetWidget(const std::string& name);

	void ActivateWidget(wiWidget* widget);
	void DeactivateWidget(wiWidget* widget);
	const wiWidget* GetActiveWidget() const;
	// true if another widget is currently active
	bool IsWidgetDisabled(wiWidget* widget);

	// returns true if any gui element has the focus
	bool HasFocus();

	void SetVisible(bool value) { visible = value; }
	bool IsVisible() { return visible; }

	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
	const Hitbox2D& GetPointerHitbox() const
	{
		return pointerhitbox;
	}
};

