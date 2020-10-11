#pragma once
#include "CommonInclude.h"
#include "wiGUI.h"
#include "wiColor.h"
#include "wiGraphicsDevice.h"
#include "wiIntersect.h"
#include "wiScene.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"

#include <string>
#include <list>
#include <functional>

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
	uint64_t userdata;
};

class wiWidget : public wiGUIElement
{
	friend class wiGUI;
public:
	enum WIDGETSTATE
	{
		IDLE,			// widget is doing nothing
		FOCUS,			// widget got pointer dragged on or selected
		ACTIVE,			// widget is interacted with right now
		DEACTIVATING,	// widget has last been active but no more interactions are occuring
		WIDGETSTATE_COUNT,
	};
private:
	int tooltipTimer = 0;
protected:
	std::string name;
	std::string tooltip;
	std::string scriptTip;
	bool enabled = true;
	bool visible = true;
	bool priority_change = true;

	WIDGETSTATE state = IDLE;
	void Activate();
	void Deactivate();

public:
	wiWidget();

	const std::string& GetName() const;
	void SetName(const std::string& value);
	const std::string GetText() const;
	void SetText(const std::string& value);
	void SetTooltip(const std::string& value);
	void SetScriptTip(const std::string& value);
	void SetPos(const XMFLOAT2& value);
	void SetSize(const XMFLOAT2& value);
	WIDGETSTATE GetState() const;
	virtual void SetEnabled(bool val);
	bool IsEnabled() const;
	virtual void SetVisible(bool val);
	bool IsVisible() const;
	// last param default: set color for all states
	void SetColor(wiColor color, WIDGETSTATE state = WIDGETSTATE_COUNT);
	wiColor GetColor() const;

	virtual void Update(wiGUI* gui, float dt);
	virtual void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const = 0;
	virtual void RenderTooltip(const wiGUI* gui, wiGraphics::CommandList cmd) const;

	wiSprite sprites[WIDGETSTATE_COUNT];
	wiSpriteFont font;

	XMFLOAT3 translation = XMFLOAT3(0, 0, 0);
	XMFLOAT3 scale = XMFLOAT3(1, 1, 1);

	Hitbox2D hitBox;

	static void Initialize();
};

// Clickable, draggable box
class wiButton : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onClick;
	std::function<void(wiEventArgs args)> onDragStart;
	std::function<void(wiEventArgs args)> onDrag;
	std::function<void(wiEventArgs args)> onDragEnd;
	XMFLOAT2 dragStart = XMFLOAT2(0, 0);
	XMFLOAT2 prevPos = XMFLOAT2(0, 0);
public:
	void Create(const std::string& name);

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

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
	void Create(const std::string& name);

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;
};

// Text input box
class wiTextInputField : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onInputAccepted;
	static wiSpriteFont font_input;

public:
	void Create(const std::string& name);

	wiSpriteFont font_description;

	void SetValue(const std::string& newValue);
	void SetValue(int newValue);
	void SetValue(float newValue);
	const std::string GetValue();
	void SetDescription(const std::string& desc) { font_description.SetText(desc); }
	const std::string GetDescription() const { return font_description.GetTextA(); }

	// There can only be ONE active text input field, so these methods modify the active one
	static void AddInput(const char inputChar);
	static void DeleteFromInput();

	void Update(wiGUI* gui, float dt) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void OnInputAccepted(std::function<void(wiEventArgs args)> func);
};

// Define an interval and slide the control along it
class wiSlider : public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onSlide;
	float start = 0, end = 1;
	float step = 1000;
	float value = 0;

	wiTextInputField valueInputField;
public:
	// start : slider minimum value
	// end : slider maximum value
	// defaultValue : slider default Value
	// step : slider step size
	void Create(float start, float end, float defaultValue, float step, const std::string& name);
	
	wiSprite sprites_knob[WIDGETSTATE_COUNT];

	void SetValue(float value);
	float GetValue() const;
	void SetRange(float start, float end);

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;
	void RenderTooltip(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void OnSlide(std::function<void(wiEventArgs args)> func);
};

// Two-state clickable box
class wiCheckBox :public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onClick;
	bool checked = false;
public:
	void Create(const std::string& name);

	wiSprite sprites_check[WIDGETSTATE_COUNT];

	void SetCheck(bool value);
	bool GetCheck() const;

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void OnClick(std::function<void(wiEventArgs args)> func);
};

// Drop-down list
class wiComboBox :public wiWidget
{
protected:
	std::function<void(wiEventArgs args)> onSelect;
	int selected = -1;
	int maxVisibleItemCount = 8;
	int firstItemVisible = 0;

	// While the widget is active (rolled down) these are the inner states that control behaviour
	enum COMBOSTATE
	{
		COMBOSTATE_INACTIVE,	// When the list is just being dropped down, or the widget is not active
		COMBOSTATE_HOVER,		// The widget is in drop-down state with the last item hovered highlited
		COMBOSTATE_SELECTING,	// The hovered item is clicked
		COMBOSTATE_SCROLLBAR_HOVER,		// scrollbar is to be selected
		COMBOSTATE_SCROLLBAR_GRABBED,	// scrollbar is moved
		COMBOSTATE_COUNT,
	} combostate = COMBOSTATE_INACTIVE;
	int hovered = -1;

	float scrollbar_delta = 0;

	std::vector<std::string> items;

	float GetItemOffset(int index) const;
public:
	void Create(const std::string& name);

	void AddItem(const std::string& item);
	void RemoveItem(int index);
	void ClearItems();
	void SetMaxVisibleItemCount(int value);
	bool HasScrollbar() const;

	void SetSelected(int index);
	int GetSelected() const;
	std::string GetItemText(int index) const;

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void OnSelect(std::function<void(wiEventArgs args)> func);
};

// Widget container
class wiWindow :public wiWidget
{
protected:
	wiButton closeButton;
	wiButton minimizeButton;
	wiButton resizeDragger_UpperLeft;
	wiButton resizeDragger_BottomRight;
	wiButton moveDragger;
	wiLabel label;
	std::list<wiWidget*> childrenWidgets;
	bool minimized = false;
public:
	void Create(const std::string& name, bool window_controls = true);

	void AddWidget(wiWidget* widget);
	void RemoveWidget(wiWidget* widget);
	void RemoveWidgets();

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;
	void RenderTooltip(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void SetVisible(bool value) override;
	void SetEnabled(bool value) override;
	void SetMinimized(bool value);
	bool IsMinimized() const;
};

// HSV-Color Picker
class wiColorPicker : public wiWindow
{
protected:
	std::function<void(wiEventArgs args)> onColorChanged;
	enum COLORPICKERSTATE
	{
		CPS_IDLE,
		CPS_HUE,
		CPS_SATURATION,
	} colorpickerstate = CPS_IDLE;
	float hue = 0.0f;			// [0, 360] degrees
	float saturation = 0.0f;	// [0, 1]
	float luminance = 1.0f;		// [0, 1]

	wiTextInputField text_R;
	wiTextInputField text_G;
	wiTextInputField text_B;
	wiTextInputField text_H;
	wiTextInputField text_S;
	wiTextInputField text_V;
	wiSlider alphaSlider;

	void FireEvents();
public:
	void Create(const std::string& name, bool window_controls  = true);

	void Update(wiGUI* gui, float dt ) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	wiColor GetPickColor() const;
	void SetPickColor(wiColor value);

	void OnColorChanged(std::function<void(wiEventArgs args)> func);
};

// List of items in a tree (parent-child relationships)
class wiTreeList :public wiWidget
{
public:
	struct Item
	{
		std::string name;
		int level = 0;
		uint64_t userdata = 0;
		bool open = false;
		bool selected = false;
	};
protected:
	std::function<void(wiEventArgs args)> onSelect;
	float list_height = 0;
	float list_offset = 0;
	int item_highlight = -1;
	int opener_highlight = -1;

	enum SCROLLBAR_STATE
	{
		SCROLLBAR_INACTIVE,
		SCROLLBAR_HOVER,
		SCROLLBAR_GRABBED,
		TREESTATE_COUNT,
	} scrollbar_state = SCROLLBAR_INACTIVE;

	float scrollbar_delta = 0;
	float scrollbar_height = 0;
	float scrollbar_value = 0;

	Hitbox2D GetHitbox_ListArea() const;
	Hitbox2D GetHitbox_Item(int visible_count, int level) const;
	Hitbox2D GetHitbox_ItemOpener(int visible_count, int level) const;

	std::vector<Item> items;

	float GetItemOffset(int index) const;
public:
	void Create(const std::string& name);

	void AddItem(const Item& item);
	void ClearItems();
	bool HasScrollbar() const;

	void ClearSelection();
	void Select(int index);

	int GetItemCount() const { return (int)items.size(); }
	const Item& GetItem(int index) const;

	void Update(wiGUI* gui, float dt) override;
	void Render(const wiGUI* gui, wiGraphics::CommandList cmd) const override;

	void OnSelect(std::function<void(wiEventArgs args)> func);
};

