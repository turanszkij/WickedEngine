#pragma once
#include "CommonInclude.h"

class wiWidget;

class wiGUI
{
	friend class wiWidget;
private:
	list<wiWidget*> widgets;
	wiWidget* activeWidget;
public:
	wiGUI();
	~wiGUI();

	void Update();
	void Render();

	void AddWidget(wiWidget* widget);
	wiWidget* GetWidget(const string& name);
};

