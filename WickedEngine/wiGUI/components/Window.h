#pragma once

/**
 * @file
 * The `wi::gui::Window` widget.
 *
 * A movable, resizable, collapsible container that hosts child widgets, with an
 * optional title bar (label, collapse and close buttons), scrollable content
 * area, and a layout helper for arranging children in rows.
 */

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>

#include <CommonInclude.h>
#include <Utility/DirectXMath/DirectXMath.h>

#include <wiCanvas.h>
#include <wiColor.h>
#include <wiGraphics.h>
#include <wiGraphicsDevice.h>
#include <wiGUI/components/ScrollBar.h>
#include <wiGUI/GUICommon.h>
#include <wiGUI/Widget.h>
#include <wiLocalization.h>
#include <wiResourceManager.h>
#include <wiVector.h>

#include "wiGUI/components/Button.h"
#include "wiGUI/components/Label.h"

namespace wi::gui
{
	/**
	 * Container widget that hosts and arranges child widgets.
	 *
	 * A window owns a list of child @ref Widget pointers and drives their
	 * update/render. It can be moved by its title bar, resized from its edges
	 * and corners, and collapsed to just the title bar. Children may be
	 * attached to a scrollable content area. The nested @ref Layout helper
	 * provides row-based placement used from @ref ResizeLayout. Which
	 * interactions are available is selected at creation via @ref
	 * WindowControls.
	 */
	class Window :public Widget
	{
	protected:
		/** Child widgets hosted by this window (non-owning pointers). */
		wi::vector<Widget*> widgets;

		/** Whether the window is collapsed to just its title bar. */
		bool minimized = false;

		/** Whether the window has a title bar. */
		bool has_titlebar = false;

		/** Whether the background image is right-aligned (aspect-kept). */
		bool right_aligned_image = false;

		/** Invisible area that scrollable children are attached to. */
		Widget scrollable_area;

		/** Height of the title bar / control row, in pixels. */
		float control_size = 20;

		/** Callback invoked when the window is closed. */
		std::function<void(const EventArgs& args)> onClose;

		/** Callback invoked when the window is collapsed/expanded. */
		std::function<void(const EventArgs& args)> onCollapse;

		/** Callback invoked when the window is resized. */
		std::function<void()> onResize;

		/** Width of the edge/corner resize grab areas, in pixels. */
		float resizehitboxwidth = 6;

		/**
		 * Which edge or corner (if any) is currently being resized.
		 */
		enum RESIZE_STATE
		{
			/** Not resizing. */
			RESIZE_STATE_NONE,

			/** Resizing via the left edge. */
			RESIZE_STATE_LEFT,

			/** Resizing via the top edge. */
			RESIZE_STATE_TOP,

			/** Resizing via the right edge. */
			RESIZE_STATE_RIGHT,

			/** Resizing via the bottom edge. */
			RESIZE_STATE_BOTTOM,

			/** Resizing via the top-left corner. */
			RESIZE_STATE_TOPLEFT,

			/** Resizing via the top-right corner. */
			RESIZE_STATE_TOPRIGHT,

			/** Resizing via the bottom-right corner. */
			RESIZE_STATE_BOTTOMRIGHT,

			/** Resizing via the bottom-left corner. */
			RESIZE_STATE_BOTTOMLEFT,
		} resize_state = RESIZE_STATE_NONE;

		/** Pointer position where the resize drag began. */
		XMFLOAT2 resize_begin = XMFLOAT2(0, 0);

		/** Timer driving the resize-handle blink feedback. */
		float resize_blink_timer = 0;

		/**
		 * Row-based layout helper for placing child widgets.
		 *
		 * Tracks a running cursor within the window's widget area and offers
		 * methods to place widgets in rows (full width, right-aligned, equally
		 * split, etc.). Typically driven from @ref Window::ResizeLayout.
		 */
		struct Layout
		{
			/** Spacing between widgets and edges, in pixels. */
			float padding = 2;

			/** Current widget-area width, in pixels. */
			float width = 0;

			/** Current widget-area height, in pixels. */
			float height = 0;

			/** Running vertical cursor, in pixels. */
			float y = 0;

			/** Running horizontal cursor, in pixels. */
			float x = 0;

			/** Left margin used by left-labeled rows, in pixels. */
			float margin_left = 160;

			/**
			 * Resets the layout state from the window's widget area.
			 *
			 * @param[in,out] window - Window providing the widget-area size.
			 */
			void reset(Window& window)
			{
				const XMFLOAT2 widgetareasize = window.GetWidgetAreaSize();

				width = widgetareasize.x;
				height = widgetareasize.y - window.GetControlSize();
				y = padding;
			}

			/**
			 * Advances the vertical cursor to separate widgets visually.
			 *
			 * @param[in] amount - Vertical space to skip, in pixels.
			 */
			void jump(float amount = 20)
			{
				y += amount;
			}

			/**
			 * Places one widget in a row, sized to the remaining width.
			 *
			 * @param[in,out] widget - Widget to position and size.
			 */
			void add(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
				{
					return;
				}

				const XMFLOAT2 size = widget.GetSize();

				x = margin_left;
				y += widget.GetShadowRadius();

				widget.SetPos(XMFLOAT2(x, y));
				widget.SetSize(XMFLOAT2(
					width - x - padding - widget.GetShadowRadius(), size.y
				));

				y += size.y;
				y += widget.GetShadowRadius();
				y += padding;
			}

			/**
			 * Places one widget aligned to the right side of the row.
			 *
			 * @param[in,out] widget - Widget to position.
			 */
			void add_right(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
				{
					return;
				}

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

			/**
			 * Places one widget spanning the full widget-area width.
			 *
			 * @param[in,out] widget - Widget to position and size.
			 */
			void add_fullwidth(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
				{
					return;
				}

				const XMFLOAT2 size = widget.GetSize();

				x = padding;
				x += widget.GetLeftTextWidth();
				x += widget.GetShadowRadius();
				y += widget.GetShadowRadius();

				widget.SetPos(XMFLOAT2(x, y));
				widget.SetSize(XMFLOAT2(
					width - x - padding - widget.GetShadowRadius(), size.y
				));

				y += size.y;
				y += widget.GetShadowRadius();
				y += padding;
			}

			/**
			 * Places one widget full-width, preserving its aspect ratio.
			 *
			 * The aspect ratio is taken from the widget's current sprite
			 * texture when available (cubemaps use a fixed 4:3 cross layout).
			 *
			 * @param[in,out] widget - Widget to position and size.
			 */
			void add_fullwidth_aspect(wi::gui::Widget& widget)
			{
				if (!widget.IsVisible())
				{
					return;
				}

				x = padding;
				x += widget.GetLeftTextWidth();
				x += widget.GetShadowRadius();
				y += widget.GetShadowRadius();

				widget.SetPos(XMFLOAT2(margin_left, y));

				float h_aspect = widget.GetSize().y / widget.GetSize().x;
				const wi::Resource& res =
					widget.sprites[widget.GetState()].textureResource;

				if (res.IsValid())
				{
					const wi::graphics::Texture& tex = res.GetTexture();

					if (has_flag(
						tex.desc.misc_flags,
						wi::graphics::ResourceMiscFlag::TEXTURECUBE
					))
					{
						// Fixed aspect for cubemap cross:
						h_aspect = 3.0f / 4.0f;
					}
					else
					{
						h_aspect =
							(float)tex.desc.height / (float)tex.desc.width;
					}
				}

				const float w = width - x - padding - widget.GetShadowRadius();

				widget.SetSize(XMFLOAT2(w, w * h_aspect));
				widget.SetPos(XMFLOAT2(x, y));

				y += widget.GetSize().y;
				y += widget.GetShadowRadius();
				y += padding;
			}

			/**
			 * Places multiple widgets in a row, right-aligned.
			 *
			 * @param[in,out] widget - First widget to position.
			 * @param[in,out] args - Remaining widgets to position.
			 */
			template<typename... Args>
			void add_right(wi::gui::Widget& widget, Args&&... args)
			{
				add_right(std::forward<Args>(args)...);

				if (!widget.IsVisible())
				{
					return;
				}

				const XMFLOAT2 size = widget.GetSize();

				x -= size.x;
				x -= widget.GetRightTextWidth();
				x -= widget.GetShadowRadius();

				widget.SetPos(XMFLOAT2(
					x, y - size.y - padding - widget.GetShadowRadius()
				));

				if (widget.GetLeftTextWidth() > 0)
				{
					x -= widget.GetLeftTextWidth() + padding * 4;
				}

				x -= padding;
				x -= widget.GetShadowRadius();
			}

			/**
			 * Resizes a widget to fit a target width, minus its left text.
			 *
			 * @param[in] sizx - Target width, in pixels.
			 * @param[in,out] widget - Widget to resize.
			 */
			void helper_fitx(float sizx, wi::gui::Widget& widget)
			{
				float left = widget.GetLeftTextWidth();

				if (left > 0)
				{
					left += padding;
				}

				widget.SetSize(XMFLOAT2(sizx - left, widget.GetSize().y));
			}

			/**
			 * Resizes multiple widgets to a common target width.
			 *
			 * @param[in] sizx - Target width, in pixels.
			 * @param[in,out] widget - First widget to resize.
			 * @param[in,out] args - Remaining widgets to resize.
			 */
			template<typename... Args>
			void helper_fitx(
				float sizx,
				wi::gui::Widget& widget,
				Args&&... args
			)
			{
				helper_fitx(sizx, widget);
				helper_fitx(sizx, std::forward<Args>(args)...);
			}

			/**
			 * Places multiple widgets in a row, equally sized to fit.
			 *
			 * @param[in,out] widget - First widget to position.
			 * @param[in,out] args - Remaining widgets to position.
			 */
			template<typename... Args>
			void add(wi::gui::Widget& widget, Args&&... args)
			{
				constexpr size_t total_count = 1 + sizeof...(Args);
				const float onewidth =
					(width - padding) / total_count
					- (padding * (total_count - 1));

				helper_fitx(onewidth, widget, std::forward<Args>(args)...);
				add_right(widget, std::forward<Args>(args)...);
			}
		} layout;

	public:
		/**
		 * Bit flags selecting which window controls/interactions exist.
		 *
		 * Combine with bitwise operators (see the `enable_bitmask_operators`
		 * specialization below).
		 */
		enum class WindowControls
		{
			/** No controls. */
			NONE = 0,

			/** Resize from the left edge. */
			RESIZE_LEFT = 1 << 0,

			/** Resize from the top edge. */
			RESIZE_TOP = 1 << 1,

			/** Resize from the right edge. */
			RESIZE_RIGHT = 1 << 2,

			/** Resize from the bottom edge. */
			RESIZE_BOTTOM = 1 << 3,

			/** Resize from the top-left corner. */
			RESIZE_TOPLEFT = 1 << 4,

			/** Resize from the top-right corner. */
			RESIZE_TOPRIGHT = 1 << 5,

			/** Resize from the bottom-left corner. */
			RESIZE_BOTTOMLEFT = 1 << 6,

			/** Resize from the bottom-right corner. */
			RESIZE_BOTTOMRIGHT = 1 << 7,

			/** Move the window by dragging the title bar. */
			MOVE = 1 << 8,

			/** Show a close button. */
			CLOSE = 1 << 9,

			/** Show a collapse button. */
			COLLAPSE = 1 << 10,

			/** Hide the title bar. */
			DISABLE_TITLE_BAR = 1 << 11,

			/** Auto-resize the window vertically to fit all its widgets. */
			FIT_ALL_WIDGETS_VERTICAL = 1 << 12,

			/** All resize edges and corners. */
			RESIZE =
				RESIZE_LEFT | RESIZE_TOP | RESIZE_RIGHT | RESIZE_BOTTOM
				| RESIZE_TOPLEFT | RESIZE_TOPRIGHT | RESIZE_BOTTOMLEFT
				| RESIZE_BOTTOMRIGHT,

			/** Close and collapse buttons. */
			CLOSE_AND_COLLAPSE = CLOSE | COLLAPSE,

			/** Resize, move, close and collapse. */
			ALL = RESIZE | MOVE | CLOSE | COLLAPSE,
		};

		/**
		 * Initializes the window and its title-bar controls.
		 *
		 * @param[in] name - Widget name; also used as the title.
		 * @param[in] window_controls - Which controls/interactions to enable.
		 */
		void Create(
			const std::string& name,
			WindowControls window_controls = WindowControls::ALL
		);

		/**
		 * Bit flags controlling how a child widget is attached.
		 *
		 * Combine with bitwise operators (see the `enable_bitmask_operators`
		 * specialization below).
		 */
		enum class AttachmentOptions
		{
			/** No special attachment. */
			NONE = 0,

			/** Attach to the scrollable content area. */
			SCROLLABLE = 1 << 0,
		};

		/**
		 * Adds a child widget to the window.
		 *
		 * @param[in] widget - Widget to add (non-owning).
		 * @param[in] options - Attachment options (scrollable by default).
		 */
		void AddWidget(
			Widget* widget,
			AttachmentOptions options = AttachmentOptions::SCROLLABLE
		);

		/**
		 * Removes a child widget from the window.
		 *
		 * @param[in] widget - Widget to remove.
		 */
		void RemoveWidget(Widget* widget);

		/**
		 * Removes all child widgets.
		 */
		void RemoveWidgets();

		/**
		 * Recomputes the layout of the title bar and child widgets.
		 */
		void ResizeLayout() override;

		/**
		 * Updates the window, its controls and all child widgets.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the window, its title bar and all child widgets.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Draws the tooltips of the window and its child widgets.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void RenderTooltip(
			const wi::Canvas& canvas,
			wi::graphics::CommandList cmd
		) const override;

		/**
		 * Sets the color of the window.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Sets the background image of the window.
		 *
		 * @param[in] resource - Texture resource to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetImage(wi::Resource resource, int id = -1) override;

		/**
		 * Sets the shadow color of the window and its controls.
		 *
		 * @param[in] color - New shadow color.
		 */
		void SetShadowColor(wi::Color color) override;

		/**
		 * Applies a theme to the window and its controls.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("Window").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override
		{
			return "Window";
		}

		/**
		 * Shows or hides the window and its children.
		 *
		 * @param[in] value - true to show.
		 */
		void SetVisible(bool value) override;

		/**
		 * Enables or disables the window and its children.
		 *
		 * @param[in] value - true to enable.
		 */
		void SetEnabled(bool value) override;

		/**
		 * Collapses or expands the window to/from just its title bar.
		 *
		 * @param[in] value - true to collapse.
		 */
		void SetCollapsed(bool value);

		/**
		 * Returns whether the window is collapsed.
		 */
		[[nodiscard]] bool IsCollapsed() const noexcept;

		/**
		 * Collapses or expands the window (alias of @ref SetCollapsed).
		 *
		 * @param[in] value - true to collapse.
		 */
		void SetMinimized(bool value);

		/**
		 * Returns whether the window is collapsed (alias of @ref IsCollapsed).
		 */
		[[nodiscard]] bool IsMinimized() const noexcept;

		/**
		 * Sets the title bar / control row height.
		 *
		 * @param[in] value - Control size in pixels.
		 */
		void SetControlSize(float value);

		/**
		 * Returns the title bar / control row height.
		 */
		[[nodiscard]] float GetControlSize() const noexcept;

		/**
		 * Returns the window size (reflects the collapsed state).
		 */
		[[nodiscard]] XMFLOAT2 GetSize() const override;

		/**
		 * Returns the size of the area available to child widgets.
		 */
		[[nodiscard]] XMFLOAT2 GetWidgetAreaSize() const noexcept;

		/**
		 * Sets the close callback.
		 *
		 * @param[in] func - Callback invoked when the window is closed.
		 */
		void OnClose(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the collapse callback.
		 *
		 * @param[in] func - Callback invoked when collapsed/expanded.
		 */
		void OnCollapse(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the resize callback.
		 *
		 * @param[in] func - Callback invoked when the window is resized.
		 */
		void OnResize(std::function<void()> func);

		/** Title-bar close button. */
		Button closeButton;

		/** Title-bar collapse/expand button. */
		Button collapseButton;

		/** Title-bar drag handle used to move the window. */
		Button moveDragger;

		/** Title-bar label showing the window title. */
		Label label;

		/** Vertical scrollbar for the content area. */
		ScrollBar scrollbar_vertical;

		/** Horizontal scrollbar for the content area. */
		ScrollBar scrollbar_horizontal;

		/** The set of enabled window controls/interactions. */
		WindowControls controls;

		/**
		 * Sets whether the background image is right-aligned (aspect-kept).
		 *
		 * @param[in] value - true to right-align the background image.
		 */
		void SetRightAlignedImage(bool value) noexcept
		{
			right_aligned_image = value;
		}

		/**
		 * Exports the window's and children's localizable strings.
		 *
		 * @param[in,out] localization - Localization to add entries to.
		 */
		void ExportLocalization(wi::Localization& localization) const override;

		/**
		 * Imports localized strings into the window and its children.
		 *
		 * @param[in] localization - Localization to read entries from.
		 */
		void ImportLocalization(const wi::Localization& localization) override;

		/** Optional texture drawn as an overlay on top of the window. */
		wi::graphics::Texture background_overlay;
	};
}

/**
 * Enables bitwise operators for @ref wi::gui::Window::WindowControls.
 */
template<>
struct enable_bitmask_operators<wi::gui::Window::WindowControls>
	: std::true_type {};

/**
 * Enables bitwise operators for @ref wi::gui::Window::AttachmentOptions.
 */
template<>
struct enable_bitmask_operators<wi::gui::Window::AttachmentOptions>
	: std::true_type {};
