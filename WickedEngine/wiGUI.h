#pragma once
#include "CommonInclude.h"
#include "wiEnums.h"

class wiHashString;

class wiWidget;

class wiGUI
{
	friend class wiWidget;
private:
	list<wiWidget*> widgets;
	wiWidget* activeWidget;
	GRAPHICSTHREAD threadID;
public:
	wiGUI(GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE);
	~wiGUI();

	void Update();
	void Render();

	void AddWidget(wiWidget* widget);
	wiWidget* GetWidget(const wiHashString& name);

	void ActivateWidget(wiWidget* widget);
	void DeactivateWidget(wiWidget* widget);
	wiWidget* GetActiveWidget();
	// true if another widget is currently active
	bool IsWidgetDisabled(wiWidget* widget);

	GRAPHICSTHREAD GetGraphicsThread() { return threadID; }
};

