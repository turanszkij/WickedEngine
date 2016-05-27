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
	void SetEnabled(bool val) { enabled = val; }
	bool IsEnabled() { return enabled; }
	void SetVisible(bool val) { visible = val; }
	bool IsVisible() { return visible; }
	void SetColor(const wiColor& color, WIDGETSTATE state);
	wiColor GetColor();

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

