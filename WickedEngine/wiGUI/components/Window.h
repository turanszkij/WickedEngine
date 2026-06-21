#pragma once
#include "wiGUI/components/Button.h"
#include "wiGUI/components/Label.h"

namespace wi::gui
{
	// Widget container
	class Window :public Widget
	{
	protected:
		wi::vector<Widget*> widgets;
		bool minimized = false;
		bool has_titlebar = false;
		bool right_aligned_image = false;
		Widget scrollable_area;
		float control_size = 20;
		std::function<void(const EventArgs& args)> onClose;
		std::function<void(const EventArgs& args)> onCollapse;
		std::function<void()> onResize;

		float resizehitboxwidth = 6;
		enum RESIZE_STATE
		{
			RESIZE_STATE_NONE,

			RESIZE_STATE_LEFT,
			RESIZE_STATE_TOP,
			RESIZE_STATE_RIGHT,
			RESIZE_STATE_BOTTOM,

			RESIZE_STATE_TOPLEFT,
			RESIZE_STATE_TOPRIGHT,
			RESIZE_STATE_BOTTOMRIGHT,
			RESIZE_STATE_BOTTOMLEFT,
		} resize_state = RESIZE_STATE_NONE;
		XMFLOAT2 resize_begin = XMFLOAT2(0, 0);
		float resize_blink_timer = 0;

		struct Layout
		{
			float padding = 2;
			float width = 0;
			float height = 0;
			float y = 0;
			float x = 0;
			float margin_left = 160;

			// Reset layout state:
			void reset(Window& window)
			{
				const XMFLOAT2 widgetareasize = window.GetWidgetAreaSize();
				width = widgetareasize.x;
				height = widgetareasize.y - window.GetControlSize();
				y = padding;
			}

			// Jump over an empty space to visually separate widgets a bit:
			void jump(float amount = 20)
			{
				y += amount;
			}

			// Add one widget in a row:
			void add(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
					return;
				const XMFLOAT2 size = widget.GetSize();
				x = margin_left;
				y += widget.GetShadowRadius();
				widget.SetPos(XMFLOAT2(x, y));
				widget.SetSize(XMFLOAT2(width - x - padding - widget.GetShadowRadius(), size.y));
				y += size.y;
				y += widget.GetShadowRadius();
				y += padding;
			}
			// Add one widget to the right side:
			void add_right(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
					return;
				const XMFLOAT2 size = widget.GetSize();
				x = width - padding - size.x;
				x -= widget.GetRightTextWidth();
				x -= widget.GetShadowRadius();
				y += widget.GetShadowRadius();
				widget.SetPos(XMFLOAT2(x, y));
				if (widget.GetLeftTextWidth() > 0)
				{
					x -= widget.GetLeftTextWidth() + padding * 4;
				}
				x -= padding;
				x -= widget.GetShadowRadius();
				y += size.y;
				y += widget.GetShadowRadius();
				y += padding;
			}
			// Add one widget to fill the whole width:
			void add_fullwidth(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
					return;
				const XMFLOAT2 size = widget.GetSize();
				x = padding;
				x += widget.GetLeftTextWidth();
				x += widget.GetShadowRadius();
				y += widget.GetShadowRadius();
				widget.SetPos(XMFLOAT2(x, y));
				widget.SetSize(XMFLOAT2(width - x - padding - widget.GetShadowRadius(), size.y));
				y += size.y;
				y += widget.GetShadowRadius();
				y += padding;
			}
			// Add one widget to fill the whole width and keep aspect ratio:
			void add_fullwidth_aspect(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
					return;
				x = padding;
				x += widget.GetLeftTextWidth();
				x += widget.GetShadowRadius();
				y += widget.GetShadowRadius();
				widget.SetPos(XMFLOAT2(margin_left, y));
				float h_aspect = widget.GetSize().y / widget.GetSize().x;
				const wi::Resource& res = widget.sprites[widget.GetState()].textureResource;
				if (res.IsValid())
				{
					const wi::graphics::Texture& tex = res.GetTexture();
					if (has_flag(tex.desc.misc_flags, wi::graphics::ResourceMiscFlag::TEXTURECUBE))
					{
						// fixed aspect for cubemap cross:
						h_aspect = 3.0f / 4.0f;
					}
					else
					{
						h_aspect = (float)tex.desc.height / (float)tex.desc.width;
					}
				}
				const float w = width - x - padding - widget.GetShadowRadius();
				widget.SetSize(XMFLOAT2(w, w * h_aspect));
				widget.SetPos(XMFLOAT2(x, y));
				y += widget.GetSize().y;
				y += widget.GetShadowRadius();
				y += padding;
			}

			// Add multiple widgets in a row aligned to the right side:
			template<typename... Args>
			void add_right(wi::gui::Widget& widget, Args&&... args)
			{
				add_right(std::forward<Args>(args)...);
				if (!widget.IsVisible())
					return;
				const XMFLOAT2 size = widget.GetSize();
				x -= size.x;
				x -= widget.GetRightTextWidth();
				x -= widget.GetShadowRadius();
				widget.SetPos(XMFLOAT2(x, y - size.y - padding - widget.GetShadowRadius()));
				if (widget.GetLeftTextWidth() > 0)
				{
					x -= widget.GetLeftTextWidth() + padding * 4;
				}
				x -= padding;
				x -= widget.GetShadowRadius();
			}

			void helper_fitx(float sizx, wi::gui::Widget& widget)
			{
				float left = widget.GetLeftTextWidth();
				if (left > 0)
					left += padding;
				widget.SetSize(XMFLOAT2(sizx - left, widget.GetSize().y));
			}
			template<typename... Args>
			void helper_fitx(float sizx, wi::gui::Widget& widget, Args&&... args)
			{
				helper_fitx(sizx, widget);
				helper_fitx(sizx, std::forward<Args>(args)...);
			}

			// Add multiple widgets in a row, equally sized to fit
			template<typename... Args>
			void add(wi::gui::Widget& widget, Args&&... args)
			{
				constexpr size_t total_count = 1 + sizeof...(Args);
				const float onewidth = (width - padding) / total_count - (padding * (total_count - 1));
				helper_fitx(onewidth, widget, std::forward<Args>(args)...);
				add_right(widget, std::forward<Args>(args)...);
			}
		} layout;

	public:
		enum class WindowControls
		{
			NONE = 0,
			RESIZE_LEFT = 1 << 0,
			RESIZE_TOP = 1 << 1,
			RESIZE_RIGHT = 1 << 2,
			RESIZE_BOTTOM = 1 << 3,
			RESIZE_TOPLEFT = 1 << 4,
			RESIZE_TOPRIGHT = 1 << 5,
			RESIZE_BOTTOMLEFT = 1 << 6,
			RESIZE_BOTTOMRIGHT = 1 << 7,
			MOVE = 1 << 8,
			CLOSE = 1 << 9,
			COLLAPSE = 1 << 10,
			DISABLE_TITLE_BAR = 1 << 11,
			FIT_ALL_WIDGETS_VERTICAL = 1 << 12, // auto resize window vertically to fit all widgets that are in it

			RESIZE = RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM | RESIZE_TOPLEFT | RESIZE_TOPRIGHT | RESIZE_BOTTOMLEFT | RESIZE_BOTTOMRIGHT,
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
		void SetImage(wi::Resource resource, int id = -1) override;
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

		void OnClose(std::function<void(const EventArgs& args)> func);
		void OnCollapse(std::function<void(const EventArgs& args)> func);
		void OnResize(std::function<void()> func);

		Button closeButton;
		Button collapseButton;
		Button moveDragger;
		Label label;
		ScrollBar scrollbar_vertical;
		ScrollBar scrollbar_horizontal;
		WindowControls controls;

		// Set whether the background image should be aligned to the right side while keeping aspect ratio
		void SetRightAlignedImage(bool value) { right_aligned_image = value; }

		void ExportLocalization(wi::Localization& localization) const override;
		void ImportLocalization(const wi::Localization& localization) override;

		wi::graphics::Texture background_overlay;
	};
}

template<>
struct enable_bitmask_operators<wi::gui::Window::WindowControls> : std::true_type {};

template<>
struct enable_bitmask_operators<wi::gui::Window::AttachmentOptions> : std::true_type {};
