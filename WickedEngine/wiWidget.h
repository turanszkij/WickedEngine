#pragma once
#include "CommonInclude.h"
#include "wiTransform.h"
#include "wiHashString.h"
#include "wiColor.h"
#include "wiGraphicsAPI.h"
#include "wiIntersectables.h"

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
	XMFLOAT2 startPos;
	XMFLOAT2 deltaPos;
	XMFLOAT2 endPos;
	float fValue;
	bool bValue;
	XMFLOAT4 color;
};

class wiWidget : public Transform
{
	friend class wiGUI;
public:
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
	};
private:
	float fontScaling;
protected:
	wiHashString fastName;
	string text;
	bool enabled;
	bool visible;

	WIDGETSTATE state;
	void Activate();
	void Deactivate();
	wiColor colors[WIDGETSTATE_COUNT];
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
	// last param default: set color for all states
	void SetColor(const wiColor& color, WIDGETSTATE state = WIDGETSTATE_COUNT);
	wiColor GetColor();
	void SetScissorRect(const wiGraphicsTypes::Rect& rect);

	virtual void Update(wiGUI* gui);
	virtual void Render(wiGUI* gui) = 0;

	wiWidget* container;
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
	void RemoveWidgets(bool alsoDelete = false);

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	virtual void SetVisible(bool value) override;
	virtual void SetEnabled(bool value) override;
	void SetMinimized(bool value);
	bool IsMinimized();
};

// HSV-Color Picker
class wiColorPicker : public wiWindow
{
protected:
	function<void(wiEventArgs args)> onColorChanged;
	XMFLOAT2 hue_picker;
	XMFLOAT2 saturation_picker;
	XMFLOAT3 saturation_picker_barycentric;
	XMFLOAT4 hue_color;
	XMFLOAT4 final_color;
	float angle;
	bool huefocus; // whether the hue is in focus or the saturation
public:
	wiColorPicker(wiGUI* gui, const string& name = "");
	virtual ~wiColorPicker();

	virtual void Update(wiGUI* gui) override;
	virtual void Render(wiGUI* gui) override;

	XMFLOAT4 GetColor();

	void OnColorChanged(function<void(wiEventArgs args)> func);
};

