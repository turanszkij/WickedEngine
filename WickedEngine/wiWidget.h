#pragma once
#include "CommonInclude.h"
#include "wiTransform.h"
#include "wiHashString.h"
#include "wiHitBox2D.h"

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
	float fValue;
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
		// widget has last been active but no more interactions are occuring
		DEACTIVATING,
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
	wiButton(const string& name = "");
	virtual ~wiButton();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(function<void(wiEventArgs args)> func);
};

class wiLabel : public wiWidget
{
protected:
public:
	wiLabel(const string& name = "");
	virtual ~wiLabel();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;
};

class wiSlider : public wiWidget
{
protected:
	function<void(wiEventArgs args)> onSlide;
	Hitbox2D hitBox;
	float start, end;
	float step;
	float value;
public:
	// start		: slider minimum value
	// end			: slider maximum value
	// defaultValue	: slider default Value
	// step			: slider step size
	wiSlider(float start = 0.0f, float end = 1.0f, float defaultValue = 0.5f, float step = 1000.0f, const string& name = "");
	virtual ~wiSlider();

	void SetValue(float value);
	float GetValue();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnSlide(function<void(wiEventArgs args)> func);
};

