#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"

#include <list>

class wiHashString;

class wiWidget;

class wiGUI
{
	friend class wiWidget;
private:
	std::list<wiWidget*> widgets;
	wiWidget* activeWidget;
	GRAPHICSTHREAD threadID;
	bool focus;

	XMFLOAT2 pointerpos;
public:
	wiGUI(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
	~wiGUI();

	void Update(float dt);
	void Render();

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	wiWidget* GetWidget(const wiHashString& name);

	void ActivateWidget(wiWidget* widget);
	void DeactivateWidget(wiWidget* widget);
	wiWidget* GetActiveWidget();
	// true if another widget is currently active
	bool IsWidgetDisabled(wiWidget* widget);

	// returns true if any gui element has the focus
	bool HasFocus();

	GRAPHICSTHREAD GetGraphicsThread() { return threadID; }


	const XMFLOAT2& GetPointerPos() const
	{
		return pointerpos;
	}
};

