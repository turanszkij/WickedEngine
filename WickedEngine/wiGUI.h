#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiSceneSystem.h"

#include <list>

class wiHashString;

class wiWidget;

class wiGUI : public wiSceneSystem::TransformComponent
{
	friend class wiWidget;
private:
	std::list<wiWidget*> widgets;
	wiWidget* activeWidget;
	bool focus;
	bool visible;

	XMFLOAT2 pointerpos;
public:
	wiGUI();
	~wiGUI();

	void Update(float dt);
	void Render(GRAPHICSTHREAD threadID) const;

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	wiWidget* GetWidget(const wiHashString& name);

	void ActivateWidget(wiWidget* widget);
	void DeactivateWidget(wiWidget* widget);
	const wiWidget* GetActiveWidget() const;
	// true if another widget is currently active
	bool IsWidgetDisabled(wiWidget* widget);

	// returns true if any gui element has the focus
	bool HasFocus();

	void SetVisible(bool value) { visible = value; }
	bool IsVisible() { return visible; }

	void ResetScissor(GRAPHICSTHREAD threadID) const;


	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
};

