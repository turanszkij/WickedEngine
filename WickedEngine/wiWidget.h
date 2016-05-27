#pragma once
#include "CommonInclude.h"
#include "wiTransform.h"
#include "wiHashString.h"
#include "wiHitBox2D.h"
#include "wiColor.h"

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
	float fValue;
	bool bValue;
};

class wiWidget : public Transform
{
	friend class wiGUI;
protected:
	wiHashString fastName;
	string text;
	bool enabled;
	bool visible;

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
		WIDGETSTATE_COUNT,
	} state;
	void Activate();
	void Deactivate();
	wiColor colors[WIDGETSTATE_COUNT];
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
	void SetEnabled(bool val);
	bool IsEnabled();
	virtual void SetVisible(bool val);
	bool IsVisible();
	void SetColor(const wiColor& color, WIDGETSTATE state);
	wiColor GetColor();

	virtual void Update(wiGUI* gui);
	virtual void Render(wiGUI* gui) = 0;
};

// Clickable box
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

// Static box that holds text
class wiLabel : public wiWidget
{
protected:
public:
	wiLabel(const string& name = "");
	virtual ~wiLabel();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;
};

// Define an interval and slide the control along it
class wiSlider : public wiWidget
{
protected:
	function<void(wiEventArgs args)> onSlide;
	Hitbox2D hitBox;
	float start, end;
	float step;
	float value;
public:
	// start : slider minimum value
	// end : slider maximum value
	// defaultValue : slider default Value
	// step : slider step size
	wiSlider(float start = 0.0f, float end = 1.0f, float defaultValue = 0.5f, float step = 1000.0f, const string& name = "");
	virtual ~wiSlider();

	void SetValue(float value);
	float GetValue();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnSlide(function<void(wiEventArgs args)> func);
};

// Two-state clickable box
class wiCheckBox :public wiWidget
{
protected:
	function<void(wiEventArgs args)> onClick;
	Hitbox2D hitBox;
	bool checked;
public:
	wiCheckBox(const string& name = "");
	virtual ~wiCheckBox();

	void SetCheck(bool value);
	bool GetCheck();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(function<void(wiEventArgs args)> func);
};

// Widget container
class wiWindow :public wiWidget
{
protected:
	wiGUI* gui;
	wiButton* closeButton;
	list<wiWidget*> children;
public:
	wiWindow(wiGUI* gui, const string& name = "");
	virtual ~wiWindow();

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	void AddCloseButton();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	virtual void SetVisible(bool value) override;
};

