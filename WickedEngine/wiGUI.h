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
#include "wiLocalization.h"

#include <string>
#include <functional>

namespace wi::gui
{

	struct EventArgs
	{
		XMFLOAT2 clickPos = {};	// mouse click coordinate
		XMFLOAT2 startPos = {};	// mouse start position of operation (drag)
		XMFLOAT2 deltaPos = {}; // mouse delta position of operation since last update (drag)
		XMFLOAT2 endPos = {};	// mouse end position of operation (drag)
		float fValue = 0;		// generic float value of operation
		bool bValue = false;	// generic boolean value of operation
		int iValue = 0;			// generic integer value of operation
		wi::Color color;		// color value of color picker operation
		std::string sValue;		// generic string value of operation
		uint64_t userdata = 0;	// this will provide the userdata value that was set to a widget (or part of a widget)
	};

	enum WIDGETSTATE
	{
		IDLE,			// widget is doing nothing
		FOCUS,			// widget got pointer dragged on or selected
		ACTIVE,			// widget is interacted with right now
		DEACTIVATING,	// widget has last been active but no more interactions are occuring
		WIDGETSTATE_COUNT,
	};

	// These can be used to target a setting for a specific widget control and state:
	enum WIDGET_ID
	{
		// IDs for normal widget states:
		WIDGET_ID_IDLE = IDLE,
		WIDGET_ID_FOCUS = FOCUS,
		WIDGET_ID_ACTIVE = ACTIVE,
		WIDGET_ID_DEACTIVATING = DEACTIVATING,

		// IDs for special widget states:

		// TextInputField:
		WIDGET_ID_TEXTINPUTFIELD_BEGIN, // do not use!
		WIDGET_ID_TEXTINPUTFIELD_IDLE,
		WIDGET_ID_TEXTINPUTFIELD_FOCUS,
		WIDGET_ID_TEXTINPUTFIELD_ACTIVE,
		WIDGET_ID_TEXTINPUTFIELD_DEACTIVATING,
		WIDGET_ID_TEXTINPUTFIELD_END, // do not use!

		// Slider:
		WIDGET_ID_SLIDER_BEGIN, // do not use!
		WIDGET_ID_SLIDER_BASE_IDLE,
		WIDGET_ID_SLIDER_BASE_FOCUS,
		WIDGET_ID_SLIDER_BASE_ACTIVE,
		WIDGET_ID_SLIDER_BASE_DEACTIVATING,
		WIDGET_ID_SLIDER_KNOB_IDLE,
		WIDGET_ID_SLIDER_KNOB_FOCUS,
		WIDGET_ID_SLIDER_KNOB_ACTIVE,
		WIDGET_ID_SLIDER_KNOB_DEACTIVATING,
		WIDGET_ID_SLIDER_END, // do not use!

		// Scrollbar:
		WIDGET_ID_SCROLLBAR_BEGIN, // do not use!
		WIDGET_ID_SCROLLBAR_BASE_IDLE,
		WIDGET_ID_SCROLLBAR_BASE_FOCUS,
		WIDGET_ID_SCROLLBAR_BASE_ACTIVE,
		WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING,
		WIDGET_ID_SCROLLBAR_KNOB_INACTIVE,
		WIDGET_ID_SCROLLBAR_KNOB_HOVER,
		WIDGET_ID_SCROLLBAR_KNOB_GRABBED,
		WIDGET_ID_SCROLLBAR_END, // do not use!

		// Combo box:
		WIDGET_ID_COMBO_DROPDOWN,

		// Window:
		WIDGET_ID_WINDOW_BASE,

		// other user-defined widget states can be specified after this (but in user's own enum):
		//	And you will of course need to handle it yourself in a SetColor() override for example
		WIDGET_ID_USER,
	};

	enum class LocalizationEnabled
	{
		None = 0,
		Text = 1 << 0,
		Tooltip = 1 << 1,
		Items = 1 << 2,		// ComboBox items
		Children = 1 << 3,	// Window children

		All = Text | Tooltip | Items | Children,
	};

	struct Theme
	{
		// Reduced version of wi::image::Params, excluding position, alignment, etc.
		struct Image
		{
			inline static const wi::image::Params params; // prototype for default values
			XMFLOAT4 color = params.color;
			wi::enums::BLENDMODE blendFlag = params.blendFlag;
			wi::image::SAMPLEMODE sampleFlag = params.sampleFlag;
			wi::image::QUALITY quality = params.quality;
			bool background = params.isBackgroundEnabled();
			bool corner_rounding = params.isCornerRoundingEnabled();
			wi::image::Params::Rounding corners_rounding[arraysize(params.corners_rounding)];

			void Apply(wi::image::Params& params) const
			{
				params.color = color;
				params.blendFlag = blendFlag;
				params.sampleFlag = sampleFlag;
				params.quality = quality;
				if (background)
				{
					params.enableBackground();
				}
				else
				{
					params.disableBackground();
				}
				if (corner_rounding)
				{
					params.enableCornerRounding();
				}
				else
				{
					params.disableCornerRounding();
				}
				std::memcpy(params.corners_rounding, corners_rounding, sizeof(corners_rounding));
			}
			void CopyFrom(const wi::image::Params& params)
			{
				color = params.color;
				blendFlag = params.blendFlag;
				sampleFlag = params.sampleFlag;
				quality = params.quality;
				if (params.isBackgroundEnabled())
				{
					background = true;
				}
				else
				{
					background = false;
				}
				if (params.isCornerRoundingEnabled())
				{
					corner_rounding = true;
				}
				else
				{
					corner_rounding = false;
				}
				std::memcpy(corners_rounding, params.corners_rounding, sizeof(corners_rounding));
			}
		} image;

		// Reduced version of wi::font::Params, excluding position, alignment, etc.
		struct Font
		{
			inline static const wi::font::Params params; // prototype for default values
			wi::Color color = params.color;
			wi::Color shadow_color = params.shadowColor;
			int style = params.style;
			float softness = params.softness;
			float bolden = params.bolden;
			float shadow_softness = params.shadow_softness;
			float shadow_bolden = params.shadow_bolden;
			float shadow_offset_x = params.shadow_offset_x;
			float shadow_offset_y = params.shadow_offset_y;

			void Apply(wi::font::Params& params) const
			{
				params.color = color;
				params.shadowColor = shadow_color;
				params.style = style;
				params.softness = softness;
				params.bolden = bolden;
				params.shadow_softness = shadow_softness;
				params.shadow_bolden = shadow_bolden;
				params.shadow_offset_x = shadow_offset_x;
				params.shadow_offset_y = shadow_offset_y;
			}
			void CopyFrom(const wi::font::Params& params)
			{
				color = params.color;
				shadow_color = params.shadowColor;
				style = params.style;
				softness = params.softness;
				bolden = params.bolden;
				shadow_softness = params.shadow_softness;
				shadow_bolden = params.shadow_bolden;
				shadow_offset_x = params.shadow_offset_x;
				shadow_offset_y = params.shadow_offset_y;
			}
		} font;

		wi::Color shadow_color = wi::Color::Shadow(); // shadow color for whole widget

		Image tooltipImage;
		Font tooltipFont;
		wi::Color tooltip_shadow_color = wi::Color::Shadow();
	};

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
		bool HasFocus() const;
		// returns true if text input is happening
		bool IsTyping() const;

		void SetVisible(bool value) { visible = value; }
		bool IsVisible() const { return visible; }

		void SetColor(wi::Color color, int id = -1);
		void SetShadowColor(wi::Color color);
		void SetTheme(const Theme& theme, int id = -1);

		void ExportLocalization(wi::Localization& localization) const;
		void ImportLocalization(const wi::Localization& localization);
	};

	class Widget : public wi::scene::TransformComponent
	{
	private:
		int tooltipTimer = 0;
	protected:
		std::string name;
		bool enabled = true;
		bool visible = true;
		LocalizationEnabled localization_enabled = LocalizationEnabled::All;
		float shadow = 1; // shadow radius
		wi::Color shadow_color = wi::Color::Shadow();
		WIDGETSTATE state = IDLE;
		float tooltip_shadow = 1; // shadow radius
		wi::Color tooltip_shadow_color = wi::Color::Shadow();
		mutable wi::Sprite tooltipSprite;
		mutable wi::SpriteFont tooltipFont;
		mutable wi::SpriteFont scripttipFont;

	public:
		Widget();
		virtual ~Widget() = default;

		const std::string& GetName() const;
		void SetName(const std::string& value);
		std::string GetText() const;
		std::string GetTooltip() const;
		void SetText(const char* value);
		void SetText(const std::string& value);
		void SetText(std::string&& value);
		void SetTooltip(const std::string& value);
		void SetTooltip(std::string&& value);
		void SetScriptTip(const std::string& value);
		void SetScriptTip(std::string&& value);
		void SetPos(const XMFLOAT2& value);
		void SetSize(const XMFLOAT2& value);
		XMFLOAT2 GetPos() const;
		virtual XMFLOAT2 GetSize() const;
		WIDGETSTATE GetState() const;
		virtual void SetEnabled(bool val);
		bool IsEnabled() const;
		virtual void SetVisible(bool val);
		bool IsVisible() const;
		wi::Color GetColor() const;
		float GetShadowRadius() const { return shadow; }
		void SetShadowRadius(float value) { shadow = value; }

		virtual void ResizeLayout() {};
		virtual void Update(const wi::Canvas& canvas, float dt);
		virtual void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const {}
		virtual void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		// last param default: set color for all states
		//	you can specify a WIDGET_ID here, or your own custom ID if you use your own widget type
		virtual void SetColor(wi::Color color, int id = -1);
		virtual void SetShadowColor(wi::Color color);
		virtual void SetImage(wi::Resource textureResource, int id = -1);
		virtual void SetTheme(const Theme& theme, int id = -1);
		virtual const char* GetWidgetTypeName() const { return "Widget"; }

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

		bool IsLocalizationEnabled() const { return localization_enabled != LocalizationEnabled::None; }
		LocalizationEnabled GetLocalizationEnabled() const { return localization_enabled; }
		void SetLocalizationEnabled(LocalizationEnabled value) { localization_enabled = value; }
		void SetLocalizationEnabled(bool value) { localization_enabled = value ? LocalizationEnabled::All : LocalizationEnabled::None; }
		virtual void ExportLocalization(wi::Localization& localization) const;
		virtual void ImportLocalization(const wi::Localization& localization);
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
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Button"; }

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
		float safe_area = 0;
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
		void SetSafeArea(float value) { safe_area = value; }

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
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "ScrollBar"; }

		void SetVertical(bool value) { vertical = value; }
		bool IsVertical() const { return vertical; }
	};

	// Static box that holds text
	class Label : public Widget
	{
	protected:
	public:
		void Create(const std::string& name);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Label"; }

		float scrollbar_width = 18;
		ScrollBar scrollbar;
	};

	// Text input box
	class TextInputField : public Widget
	{
	protected:
		std::function<void(EventArgs args)> onInputAccepted;
		std::function<void(EventArgs args)> onInput;
		static wi::SpriteFont font_input;
		bool cancel_input_enabled = true;

	public:
		void Create(const std::string& name);

		wi::SpriteFont font_description;

		void SetValue(const std::string& newValue);
		void SetValue(int newValue);
		void SetValue(float newValue);
		const std::string GetValue();
		const std::string GetCurrentInputValue();
		void SetDescription(const std::string& desc) { font_description.SetText(desc); }
		const std::string GetDescription() const { return font_description.GetTextA(); }

		// Set whether incomplete input will be removed on lost activation state (default: true)
		void SetCancelInputEnabled(bool value) { cancel_input_enabled = value; }
		bool IsCancelInputEnabled() const { return cancel_input_enabled; }

		// There can only be ONE active text input field, so these methods modify the active one
		static void AddInput(const wchar_t inputChar);
		static void AddInput(const char inputChar);
		static void DeleteFromInput(int direction = -1);
		void SetAsActive();

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "TextInputField"; }

		// Called when input was accepted with ENTER key:
		void OnInputAccepted(std::function<void(EventArgs args)> func);
		// Called when input was updated with new character:
		void OnInput(std::function<void(EventArgs args)> func);
	};

	// Define an interval and slide the control along it
	class Slider : public Widget
	{
	protected:
		std::function<void(EventArgs args)> onSlide;
		float start = 0, end = 1;
		float step = 1000;
		float value = 0;
	public:
		// start : slider minimum value
		// end : slider maximum value
		// defaultValue : slider default Value
		// step : slider step size
		void Create(float start, float end, float defaultValue, float step, const std::string& name);

		wi::Sprite sprites_knob[WIDGETSTATE_COUNT];

		void SetValue(float value);
		void SetValue(int value);
		float GetValue() const;
		void SetRange(float start, float end);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Slider"; }

		void OnSlide(std::function<void(EventArgs args)> func);

		TextInputField valueInputField;
	};

	// Two-state clickable box
	class CheckBox :public Widget
	{
	protected:
		std::function<void(EventArgs args)> onClick;
		bool checked = false;
		std::wstring check_text;
		std::wstring uncheck_text;
	public:
		void Create(const std::string& name);

		void SetCheck(bool value);
		bool GetCheck() const;

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		const char* GetWidgetTypeName() const override { return "CheckBox"; }

		void OnClick(std::function<void(EventArgs args)> func);

		static void SetCheckTextGlobal(const std::string& text);
		void SetCheckText(const std::string& text);
		void SetUnCheckText(const std::string& text);
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

		wi::Color drop_color = wi::Color::Ghost();
		std::wstring invalid_selection_text;

		float GetDropOffset(const wi::Canvas& canvas) const;
		float GetItemOffset(const wi::Canvas& canvas, int index) const;
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
		void SetInvalidSelectionText(const std::string& text);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "ComboBox"; }

		void OnSelect(std::function<void(EventArgs args)> func);

		wi::SpriteFont selected_font;

		void ExportLocalization(wi::Localization& localization) const override;
		void ImportLocalization(const wi::Localization& localization) override;
	};

	// Widget container
	class Window :public Widget
	{
	protected:
		wi::vector<Widget*> widgets;
		bool minimized = false;
		Widget scrollable_area;
		float control_size = 20;
		std::function<void(EventArgs args)> onClose;
		std::function<void(EventArgs args)> onCollapse;
		std::function<void()> onResize;

	public:
		enum class WindowControls
		{
			NONE = 0,
			RESIZE_TOPLEFT = 1 << 0,
			RESIZE_TOPRIGHT = 1 << 1,
			RESIZE_BOTTOMLEFT = 1 << 2,
			RESIZE_BOTTOMRIGHT = 1 << 3,
			MOVE = 1 << 4,
			CLOSE = 1 << 5,
			COLLAPSE = 1 << 6,

			RESIZE = RESIZE_TOPLEFT | RESIZE_TOPRIGHT | RESIZE_BOTTOMLEFT | RESIZE_BOTTOMRIGHT,
			CLOSE_AND_COLLAPSE = CLOSE | COLLAPSE,
			ALL = RESIZE | MOVE | CLOSE | COLLAPSE,
		};
		void Create(const std::string& name, WindowControls window_controls = WindowControls::ALL);

		enum class AttachmentOptions
		{
			NONE = 0,
			SCROLLABLE = 1 << 0,
		};
		void AddWidget(Widget* widget, AttachmentOptions options = AttachmentOptions::SCROLLABLE);
		void RemoveWidget(Widget* widget);
		void RemoveWidgets();

		void ResizeLayout() override;
		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetShadowColor(wi::Color color) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "Window"; }

		void SetVisible(bool value) override;
		void SetEnabled(bool value) override;
		void SetCollapsed(bool value);
		bool IsCollapsed() const;
		void SetMinimized(bool value); // Same as SetCollapsed()
		bool IsMinimized() const; // Same as IsCollapsed()
		void SetControlSize(float value);
		float GetControlSize() const;
		XMFLOAT2 GetSize() const override; // For the window, the returned size can be modified by collapsed state
		XMFLOAT2 GetWidgetAreaSize() const;

		void OnClose(std::function<void(EventArgs args)> func);
		void OnCollapse(std::function<void(EventArgs args)> func);
		void OnResize(std::function<void()> func);

		Button closeButton;
		Button collapseButton;
		Button resizeDragger_UpperLeft;
		Button resizeDragger_UpperRight;
		Button resizeDragger_BottomLeft;
		Button resizeDragger_BottomRight;
		Button moveDragger;
		Label label;
		ScrollBar scrollbar_vertical;
		ScrollBar scrollbar_horizontal;

		void ExportLocalization(wi::Localization& localization) const override;
		void ImportLocalization(const wi::Localization& localization) override;
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

		void FireEvents();
	public:
		void Create(const std::string& name, WindowControls window_controls = WindowControls::ALL);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void ResizeLayout() override;
		const char* GetWidgetTypeName() const override { return "ColorPicker"; }

		wi::Color GetPickColor() const;
		void SetPickColor(wi::Color value);

		void OnColorChanged(std::function<void(EventArgs args)> func);

		TextInputField text_R;
		TextInputField text_G;
		TextInputField text_B;
		TextInputField text_H;
		TextInputField text_S;
		TextInputField text_V;
		TextInputField text_hex;
		Slider alphaSlider;
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
		std::function<void(EventArgs args)> onDelete;
		int item_highlight = -1;
		int opener_highlight = -1;

		wi::primitive::Hitbox2D GetHitbox_ListArea() const;
		wi::primitive::Hitbox2D GetHitbox_Item(int visible_count, int level) const;
		wi::primitive::Hitbox2D GetHitbox_ItemOpener(int visible_count, int level) const;

		wi::vector<Item> items;

		float GetItemOffset(int index) const;
		bool DoesItemHaveChildren(int index) const;
	public:
		void Create(const std::string& name);

		void AddItem(const Item& item);
		void AddItem(const std::string& name);
		void ClearItems();
		bool HasScrollbar() const;

		void ClearSelection();
		void Select(int index);

		int GetItemCount() const { return (int)items.size(); }
		const Item& GetItem(int index) const;

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "TreeList"; }

		void OnSelect(std::function<void(EventArgs args)> func);
		void OnDelete(std::function<void(EventArgs args)> func);

		ScrollBar scrollbar;
	};

}

template<>
struct enable_bitmask_operators<wi::gui::Window::WindowControls> {
	static const bool enable = true;
};

template<>
struct enable_bitmask_operators<wi::gui::Window::AttachmentOptions> {
	static const bool enable = true;
};

template<>
struct enable_bitmask_operators<wi::gui::LocalizationEnabled> {
	static const bool enable = true;
};
