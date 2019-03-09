#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"
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
	GRAPHICSTHREAD threadID;
	bool focus;
	bool visible;

	XMFLOAT2 pointerpos;
public:
	wiGUI(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
	~wiGUI();

	void Update(float dt);
	void Render() const;

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

	GRAPHICSTHREAD GetGraphicsThread() const { return threadID; }

	void ResetScissor() const;


	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
};

