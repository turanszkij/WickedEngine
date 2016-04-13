#pragma once
#include "CommonInclude.h"

class wiGUI;

class wiWidget
{
private:
	string name;
public:
	wiWidget();
	~wiWidget();

	string GetName();
	void SetName(const string& newName);

	virtual void Update(wiGUI* gui) = 0;
	virtual void Render(wiGUI* gui) = 0;
};

class wiButton : public wiWidget
{
private:
	function<void()> onClick;
public:
	wiButton(const string& newName);
	~wiButton();

	virtual void Update(wiGUI* gui);
	virtual void Render(wiGUI* gui);

	void OnClick(function<void()> func);
};

