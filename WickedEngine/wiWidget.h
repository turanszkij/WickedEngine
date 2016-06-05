#pragma once
#include "CommonInclude.h"
#include "wiTransform.h"
#include "wiHashString.h"
#include "wiHitBox2D.h"
#include "wiColor.h"
#include "wiGraphicsDescriptors.h"

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
	XMFLOAT2 startPos;
	XMFLOAT2 deltaPos;
	XMFLOAT2 endPos;
	float fValue;
	bool bValue;
};

class wiWidget : public Transform
{
	friend class wiGUI;
private:
	float fontScaling;
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
	float GetScaledFontSize();
	wiGraphicsTypes::Rect scissorRect;
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
	virtual void SetEnabled(bool val);
	bool IsEnabled();
	virtual void SetVisible(bool val);
	bool IsVisible();
	void SetColor(const wiColor& color, WIDGETSTATE state);
	wiColor GetColor();
	void SetFontScaling(float val);
	float GetFontScaling();
	void SetScissorRect(const wiGraphicsTypes::Rect& rect);

	virtual void Update(wiGUI* gui);
	virtual void Render(wiGUI* gui) = 0;
};

// Clickable, draggable box
class wiButton : public wiWidget
{
protected:
	function<void(wiEventArgs args)> onClick;
	function<void(wiEventArgs args)> onDragStart;
	function<void(wiEventArgs args)> onDrag;
	function<void(wiEventArgs args)> onDragEnd;
	XMFLOAT2 dragStart;
	XMFLOAT2 prevPos;
	Hitbox2D hitBox;
public:
	wiButton(const string& name = "");
	virtual ~wiButton();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(function<void(wiEventArgs args)> func);
	void OnDragStart(function<void(wiEventArgs args)> func);
	void OnDrag(function<void(wiEventArgs args)> func);
	void OnDragEnd(function<void(wiEventArgs args)> func);
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
	wiButton* minimizeButton;
	wiButton* resizeDragger_UpperLeft;
	wiButton* resizeDragger_BottomRight;
	wiButton* moveDragger;
	list<wiWidget*> childrenWidgets;
	bool minimized;
public:
	wiWindow(wiGUI* gui, const string& name = "");
	virtual ~wiWindow();

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	void RemoveWidgets();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	virtual void SetVisible(bool value) override;
	virtual void SetEnabled(bool value) override;
	void SetMinimized(bool value);
	bool IsMinimized();
};

