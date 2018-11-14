#pragma once
#include "CommonInclude.h"
#include "wiHashString.h"
#include "wiColor.h"
#include "wiGraphicsDevice.h"
#include "wiIntersectables.h"
#include "wiSceneSystem.h"

#include <string>
#include <list>
#include <functional>

class wiGUI;

struct wiEventArgs
{
	XMFLOAT2 clickPos;
	XMFLOAT2 startPos;
	XMFLOAT2 deltaPos;
	XMFLOAT2 endPos;
	float fValue;
	bool bValue;
	int iValue;
	wiColor color;
	std::string sValue;
};

class wiWidget : public wiSceneSystem::TransformComponent
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
	int tooltipTimer;
protected:
	wiHashString fastName;
	std::string text;
	std::string tooltip;
	std::string scriptTip;
	bool enabled;
	bool visible;

	WIDGETSTATE state;
	void Activate();
	void Deactivate();
	wiColor colors[WIDGETSTATE_COUNT];
	wiGraphicsTypes::Rect scissorRect;

	wiColor textColor;
	wiColor textShadowColor;
public:
	wiWidget();
	virtual ~wiWidget();

	wiHashString GetName();
	void SetName(const std::string& value);
	std::string GetText();
	void SetText(const std::string& value);
	void SetTooltip(const std::string& value);
	void SetScriptTip(const std::string& value);
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
	void SetTextColor(const wiColor& value) { textColor = value; }
	void SetTextShadowColor(const wiColor& value) { textShadowColor = value; }

	virtual void Update(wiGUI* gui, float dt);
	virtual void Render(wiGUI* gui) = 0;
	void RenderTooltip(wiGUI* gui);

	XMFLOAT3 translation;
	XMFLOAT3 scale;

	Hitbox2D hitBox;

	wiSceneSystem::TransformComponent* parent;
	XMFLOAT4X4 world_parent_bind;
	void AttachTo(wiSceneSystem::TransformComponent* parent);
	void Detach();

	static void LoadShaders();
};

// Clickable, draggable box
class wiButton : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onClick;
	std::function<void(wiEventArgs args)> onDragStart;
	std::function<void(wiEventArgs args)> onDrag;
	std::function<void(wiEventArgs args)> onDragEnd;
	XMFLOAT2 dragStart;
	XMFLOAT2 prevPos;
public:
	wiButton(const std::string& name = "");
	virtual ~wiButton();

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(std::function<void(wiEventArgs args)> func);
	void OnDragStart(std::function<void(wiEventArgs args)> func);
	void OnDrag(std::function<void(wiEventArgs args)> func);
	void OnDragEnd(std::function<void(wiEventArgs args)> func);
};

// Static box that holds text
class wiLabel : public wiWidget
{
protected:
public:
	wiLabel(const std::string& name = "");
	virtual ~wiLabel();

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;
};

// Text input box
class wiTextInputField : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onInputAccepted;

	std::string value;
	static std::string value_new;
public:
	wiTextInputField(const std::string& name = "");
	virtual ~wiTextInputField();

	void SetValue(const std::string& newValue);
	void SetValue(int newValue);
	void SetValue(float newValue);
	const std::string& GetValue();

	// There can only be ONE active text input field, so these methods modify the active one
	static void AddInput(const char inputChar);
	static void DeleteFromInput();

	virtual void Update(wiGUI* gui, float dt) override;
	virtual void Render(wiGUI* gui) override;

	void OnInputAccepted(std::function<void(wiEventArgs args)> func);
};

// Define an interval and slide the control along it
class wiSlider : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onSlide;
	float start, end;
	float step;
	float value;

	wiTextInputField* valueInputField;
public:
	// start : slider minimum value
	// end : slider maximum value
	// defaultValue : slider default Value
	// step : slider step size
	wiSlider(float start = 0.0f, float end = 1.0f, float defaultValue = 0.5f, float step = 1000.0f, const std::string& name = "");
	virtual ~wiSlider();

	void SetValue(float value);
	float GetValue();
	void SetRange(float start, float end);

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;

	void OnSlide(std::function<void(wiEventArgs args)> func);
};

// Two-state clickable box
class wiCheckBox :public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onClick;
	bool checked;
public:
	wiCheckBox(const std::string& name = "");
	virtual ~wiCheckBox();

	void SetCheck(bool value);
	bool GetCheck();

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;

	void OnClick(std::function<void(wiEventArgs args)> func);
};

// Drop-down list
class wiComboBox :public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onSelect;
	int selected;
	int maxVisibleItemCount;
	int firstItemVisible;

	// While the widget is active (rolled down) these are the inner states that control behaviour
	enum COMBOSTATE
	{
		// When the list is just being dropped down, or the widget is not active
		COMBOSTATE_INACTIVE,
		// The widget is in drop-down state with the last item hovered highlited
		COMBOSTATE_HOVER,
		// The hovered item is clicked
		COMBOSTATE_SELECTING,
		COMBOSTATE_COUNT,
	} combostate;
	int hovered;

	std::vector<std::string> items;

	const float _GetItemOffset(int index) const;
public:
	wiComboBox(const std::string& name = "");
	virtual ~wiComboBox();

	void AddItem(const std::string& item);
	void RemoveItem(int index);
	void ClearItems();
	void SetMaxVisibleItemCount(int value);

	void SetSelected(int index);
	int GetSelected();
	std::string GetItemText(int index);

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;

	void OnSelect(std::function<void(wiEventArgs args)> func);
};

// Widget container
class wiWindow :public wiWidget
{
protected:
	wiGUI* gui = nullptr;
	wiButton* closeButton = nullptr;
	wiButton* minimizeButton = nullptr;
	wiButton* resizeDragger_UpperLeft = nullptr;
	wiButton* resizeDragger_BottomRight = nullptr;
	wiButton* moveDragger = nullptr;
	std::list<wiWidget*> childrenWidgets;
	bool minimized;
public:
	wiWindow(wiGUI* gui, const std::string& name = "");
	virtual ~wiWindow();

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	void RemoveWidgets(bool alsoDelete = false);

	virtual void Update(wiGUI* gui, float dt ) override;
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
	std::function<void(wiEventArgs args)> onColorChanged;
	XMFLOAT2 hue_picker;
	XMFLOAT2 saturation_picker;
	XMFLOAT3 saturation_picker_barycentric;
	XMFLOAT4 hue_color;
	XMFLOAT4 final_color;
	float angle;
	bool huefocus; // whether the hue is in focus or the saturation
public:
	wiColorPicker(wiGUI* gui, const std::string& name = "");
	virtual ~wiColorPicker();

	virtual void Update(wiGUI* gui, float dt ) override;
	virtual void Render(wiGUI* gui) override;

	XMFLOAT4 GetPickColor();
	void SetPickColor(const XMFLOAT4& value);

	void OnColorChanged(std::function<void(wiEventArgs args)> func);
};

