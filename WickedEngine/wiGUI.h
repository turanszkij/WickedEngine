#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"
#include "wiCanvas.h"
#include "wiVector.h"
#include "wiColor.h"
#include "wiScene.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"

#include <string>
#include <functional>

namespace wi::gui
{
	class Widget;

	class GUI
	{
	private:
		wi::vector<Widget*> widgets;
		bool focus = false;
		bool visible = true;
	public:

		void Update(const wi::Canvas& canvas, float dt);
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		void AddWidget(Widget* widget);
		void RemoveWidget(Widget* widget);
		Widget* GetWidget(const std::string& name);

		// returns true if any gui element has the focus
		bool HasFocus();

		void SetVisible(bool value) { visible = value; }
		bool IsVisible() { return visible; }
	};

	struct EventArgs
	{
		XMFLOAT2 clickPos;
		XMFLOAT2 startPos;
		XMFLOAT2 deltaPos;
		XMFLOAT2 endPos;
		float fValue;
		bool bValue;
		int iValue;
		wi::Color color;
		std::string sValue;
		uint64_t userdata;
	};

	enum WIDGETSTATE
	{
		IDLE,			// widget is doing nothing
		FOCUS,			// widget got pointer dragged on or selected
		ACTIVE,			// widget is interacted with right now
		DEACTIVATING,	// widget has last been active but no more interactions are occuring
		WIDGETSTATE_COUNT,
	};

	class Widget : public wi::scene::TransformComponent
	{
	private:
		int tooltipTimer = 0;
	protected:
		std::string name;
		std::string tooltip;
		std::string scriptTip;
		bool enabled = true;
		bool visible = true;
		WIDGETSTATE state = IDLE;

	public:
		Widget();
		virtual ~Widget() = default;

		const std::string& GetName() const;
		void SetName(const std::string& value);
		const std::string GetText() const;
		void SetText(const std::string& value);
		void SetText(std::string&& value);
		void SetTooltip(const std::string& value);
		void SetTooltip(std::string&& value);
		void SetScriptTip(const std::string& value);
		void SetScriptTip(std::string&& value);
		void SetPos(const XMFLOAT2& value);
		void SetSize(const XMFLOAT2& value);
		WIDGETSTATE GetState() const;
		virtual void SetEnabled(bool val);
		bool IsEnabled() const;
		virtual void SetVisible(bool val);
		bool IsVisible() const;
		// last param default: set color for all states
		void SetColor(wi::Color color, WIDGETSTATE state = WIDGETSTATE_COUNT);
		wi::Color GetColor() const;
		// last param default: set color for all states
		void SetImage(wi::Resource textureResource, WIDGETSTATE state = WIDGETSTATE_COUNT);

		virtual void Update(const wi::Canvas& canvas, float dt);
		virtual void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const {}
		virtual void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		wi::Sprite sprites[WIDGETSTATE_COUNT];
		wi::SpriteFont font;

		XMFLOAT3 translation = XMFLOAT3(0, 0, 0);
		XMFLOAT3 scale = XMFLOAT3(1, 1, 1);

		wi::primitive::Hitbox2D hitBox;
		wi::graphics::Rect scissorRect;

		Widget* parent = nullptr;
		void AttachTo(Widget* parent);
		void Detach();

		void Activate();
		void Deactivate();

		void ApplyScissor(const wi::Canvas& canvas, const wi::graphics::Rect rect, wi::graphics::CommandList cmd, bool constrain_to_parent = true) const;
		wi::primitive::Hitbox2D GetPointerHitbox(bool constrained = true) const;

		wi::primitive::Hitbox2D active_area; // Pointer hitbox constrain area
		void HitboxConstrain(wi::primitive::Hitbox2D& hb) const;

		bool priority_change = true;
		uint32_t priority = 0;
		bool force_disable = false;
	};

	// Clickable, draggable box
	class Button : public Widget
	{
	protected:
		std::function<void(EventArgs args)> onClick;
		std::function<void(EventArgs args)> onDragStart;
		std::function<void(EventArgs args)> onDrag;
		std::function<void(EventArgs args)> onDragEnd;
		XMFLOAT2 dragStart = XMFLOAT2(0, 0);
		XMFLOAT2 prevPos = XMFLOAT2(0, 0);
	public:
		void Create(const std::string& name);

		wi::SpriteFont font_description;
		void SetDescription(const std::string& desc) { font_description.SetText(desc); }

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnClick(std::function<void(EventArgs args)> func);
		void OnDragStart(std::function<void(EventArgs args)> func);
		void OnDrag(std::function<void(EventArgs args)> func);
		void OnDragEnd(std::function<void(EventArgs args)> func);
	};

	// Generic scroll bar
	class ScrollBar : public Widget
	{
	protected:
		float scrollbar_delta = 0;
		float scrollbar_length = 0;
		float scrollbar_value = 0;
		float scrollbar_granularity = 1;
		float list_length = 0;
		float list_offset = 0;
		float overscroll = 0;
		bool vertical = true;
		XMFLOAT2 grab_pos = {};
		float grab_delta = 0;

	public:
		// Set the list's length that will be scrollable and moving
		void SetListLength(float size) { list_length = size; }
		// The scrolling offset that should be applied to the list items
		float GetOffset() const { return list_offset; }
		// This can be called by user for extra scrolling on top of base functionality
		void Scroll(float amount) { scrollbar_delta -= amount; }
		// How much the max scrolling will offset the list even further than it would be necessary for fitting
		//	this value is in percent of a full scrollbar's worth of extra offset
		//	0: no extra offset
		//	1: full extra offset
		void SetOverScroll(float amount) { overscroll = amount; }
		// Check whether the scrollbar is required (when the items don't fit and scrolling could be used)
		bool IsScrollbarRequired() const { return scrollbar_granularity < 0.999f; }

		enum SCROLLBAR_STATE
		{
			SCROLLBAR_INACTIVE,
			SCROLLBAR_HOVER,
			SCROLLBAR_GRABBED,
			SCROLLBAR_STATE_COUNT,
		} scrollbar_state = SCROLLBAR_INACTIVE;
		wi::Sprite sprites_knob[SCROLLBAR_STATE_COUNT];
		XMFLOAT2 knob_inset_border = {};

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
	};

	// Static box that holds text
	class Label : public Widget
	{
	protected:
		ScrollBar scrollbar;
		float scrollbar_width = 18;
	public:
		void Create(const std::string& name);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
	};

	// Text input box
	class TextInputField : public Widget
	{
	protected:
		std::function<void(EventArgs args)> onInputAccepted;
		static wi::SpriteFont font_input;

	public:
		void Create(const std::string& name);

		wi::SpriteFont font_description;

		void SetValue(const std::string& newValue);
		void SetValue(int newValue);
		void SetValue(float newValue);
		const std::string GetValue();
		void SetDescription(const std::string& desc) { font_description.SetText(desc); }
		const std::string GetDescription() const { return font_description.GetTextA(); }

		// There can only be ONE active text input field, so these methods modify the active one
		static void AddInput(const char inputChar);
		static void DeleteFromInput();

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnInputAccepted(std::function<void(EventArgs args)> func);
	};

	// Define an interval and slide the control along it
	class Slider : public Widget
	{
	protected:
		std::function<void(EventArgs args)> onSlide;
		float start = 0, end = 1;
		float step = 1000;
		float value = 0;

		TextInputField valueInputField;
	public:
		// start : slider minimum value
		// end : slider maximum value
		// defaultValue : slider default Value
		// step : slider step size
		void Create(float start, float end, float defaultValue, float step, const std::string& name);

		wi::Sprite sprites_knob[WIDGETSTATE_COUNT];

		void SetValue(float value);
		float GetValue() const;
		void SetRange(float start, float end);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnSlide(std::function<void(EventArgs args)> func);
	};

	// Two-state clickable box
	class CheckBox :public Widget
	{
	protected:
		std::function<void(EventArgs args)> onClick;
		bool checked = false;
	public:
		void Create(const std::string& name);

		wi::Sprite sprites_check[WIDGETSTATE_COUNT];

		void SetCheck(bool value);
		bool GetCheck() const;

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnClick(std::function<void(EventArgs args)> func);
	};

	// Drop-down list
	class ComboBox :public Widget
	{
	protected:
		std::function<void(EventArgs args)> onSelect;
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

		struct Item
		{
			std::string name;
			uint64_t userdata = 0;
		};
		wi::vector<Item> items;

		float GetItemOffset(int index) const;
	public:
		void Create(const std::string& name);

		void AddItem(const std::string& name, uint64_t userdata = 0);
		void RemoveItem(int index);
		void ClearItems();
		void SetMaxVisibleItemCount(int value);
		bool HasScrollbar() const;

		void SetSelected(int index);
		void SetSelectedWithoutCallback(int index); // SetSelected() but the OnSelect callback will not be executed
		void SetSelectedByUserdata(uint64_t userdata);
		void SetSelectedByUserdataWithoutCallback(uint64_t userdata); // SetSelectedByUserdata() but the OnSelect callback will not be executed
		int GetSelected() const;
		void SetItemText(int index, const std::string& text);
		void SetItemUserdata(int index, uint64_t userdata);
		std::string GetItemText(int index) const;
		uint64_t GetItemUserData(int index) const;
		size_t GetItemCount() const { return items.size(); }

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnSelect(std::function<void(EventArgs args)> func);
	};

	// Widget container
	class Window :public Widget
	{
	protected:
		Button closeButton;
		Button collapseButton;
		Button resizeDragger_UpperLeft;
		Button resizeDragger_BottomRight;
		Button moveDragger;
		Label label;
		ScrollBar scrollbar_vertical;
		ScrollBar scrollbar_horizontal;
		wi::vector<Widget*> widgets;
		bool minimized = false;
		Widget scrollable_area;

	public:
		enum class WindowControls
		{
			NONE = 0,
			RESIZE_TOPLEFT = 1 << 0,
			RESIZE_BOTTOMRIGHT = 1 << 1,
			MOVE = 1 << 2,
			CLOSE = 1 << 3,
			COLLAPSE = 1 << 4,

			RESIZE = RESIZE_TOPLEFT | RESIZE_BOTTOMRIGHT,
			CLOSE_AND_COLLAPSE = CLOSE | COLLAPSE,
			ALL = RESIZE | MOVE | CLOSE | COLLAPSE
		};
		void Create(const std::string& name, WindowControls window_controls = WindowControls::ALL);

		void AddWidget(Widget* widget, bool scrollable = true);
		void RemoveWidget(Widget* widget);
		void RemoveWidgets();

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void SetVisible(bool value) override;
		void SetEnabled(bool value) override;
		void SetCollapsed(bool value);
		bool IsCollapsed() const;
		void SetMinimized(bool value); // Same as SetCollapsed()
		bool IsMinimized() const; // Same as IsCollapsed()
	};

	// HSV-Color Picker
	class ColorPicker : public Window
	{
	protected:
		std::function<void(EventArgs args)> onColorChanged;
		enum COLORPICKERSTATE
		{
			CPS_IDLE,
			CPS_HUE,
			CPS_SATURATION,
		} colorpickerstate = CPS_IDLE;
		float hue = 0.0f;			// [0, 360] degrees
		float saturation = 0.0f;	// [0, 1]
		float luminance = 1.0f;		// [0, 1]

		TextInputField text_R;
		TextInputField text_G;
		TextInputField text_B;
		TextInputField text_H;
		TextInputField text_S;
		TextInputField text_V;
		Slider alphaSlider;

		void FireEvents();
	public:
		void Create(const std::string& name, WindowControls window_controls = WindowControls::ALL);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		wi::Color GetPickColor() const;
		void SetPickColor(wi::Color value);

		void OnColorChanged(std::function<void(EventArgs args)> func);
	};

	// List of items in a tree (parent-child relationships)
	class TreeList : public Widget
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
		std::function<void(EventArgs args)> onSelect;
		int item_highlight = -1;
		int opener_highlight = -1;

		ScrollBar scrollbar;

		wi::primitive::Hitbox2D GetHitbox_ListArea() const;
		wi::primitive::Hitbox2D GetHitbox_Item(int visible_count, int level) const;
		wi::primitive::Hitbox2D GetHitbox_ItemOpener(int visible_count, int level) const;

		wi::vector<Item> items;

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

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;

		void OnSelect(std::function<void(EventArgs args)> func);
	};

}

template<>
struct enable_bitmask_operators<wi::gui::Window::WindowControls> {
	static const bool enable = true;
};
