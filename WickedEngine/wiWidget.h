#pragma once
#include "CommonInclude.h"
#include "wiLoader.h"
#include "wiHashString.h"
#include "wiHitBox2D.h"

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
};

class wiWidget : public Transform
{
protected:
	wiHashString fastName;
	string text;

	enum WIDGETSTATE
	{
		// widget is doing nothing
		IDLE,
		// widget got pointer dragged on or selected
		FOCUS,
		// widget is interacted with right now
		ACTIVE,
	} state;
public:
	wiWidget();
	virtual ~wiWidget();

	wiHashString GetName();
	void SetName(const string& value);
	string GetText();
	void SetText(const string& value);
	void SetPos(const XMFLOAT2& value);
	void SetSize(const XMFLOAT2& value);
	WIDGETSTATE GetState();

	virtual void Update(wiGUI* gui);
	virtual void Render(wiGUI* gui) = 0;
};

class wiButton : public wiWidget
{
protected:
	function<void(wiEventArgs args)> onClick;
	Hitbox2D hitBox;
public:
	wiButton(const string& newName = "Button_Unnamed");
	virtual ~wiButton();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(function<void(wiEventArgs args)> func);
};

