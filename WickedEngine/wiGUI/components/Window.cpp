#include "wiGUI/components/Window.h"
#include "wiGUI/GUIInternal.h"

#include "wiInput.h"
#include "wiPrimitive.h"
#include "wiProfiler.h"
#include "wiRenderer.h"
#include "wiTimer.h"
#include "wiEventHandler.h"
#include "wiFont.h"
#include "wiImage.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"
#include "wiHelper.h"

#include <sstream>
#include <iomanip> // setprecision
#include <utility>

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	void Window::Create(const std::string& name, WindowControls window_controls)
	{
		SetColor(wi::Color::Ghost());

		SetName(name);
		SetText(name);
		SetSize(XMFLOAT2(640, 480));

		controls = window_controls;

		for (int i = IDLE + 1; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.color = sprites[IDLE].params.color;
		}

		if (!has_flag(window_controls, WindowControls::DISABLE_TITLE_BAR))
		{
			// Add title bar controls
			if (has_flag(window_controls, WindowControls::MOVE))
			{
				// Add a grabber onto the title bar
				moveDragger.Create(name);
				moveDragger.SetLocalizationEnabled(LocalizationEnabled::None);
				moveDragger.SetShadowRadius(0);
				moveDragger.SetText(name);
				moveDragger.font.params.h_align = wi::font::WIFALIGN_LEFT;
				moveDragger.OnDrag([this](const EventArgs& args) {
					auto saved_parent = this->parent;
					this->Detach();
					this->Translate(XMFLOAT3(args.deltaPos.x, args.deltaPos.y, 0));
					this->AttachTo(saved_parent);
					});
				AddWidget(&moveDragger, AttachmentOptions::NONE);
				has_titlebar = true;
			}

			if (has_flag(window_controls, WindowControls::CLOSE))
			{
				// Add close button to the top left corner
				closeButton.Create(name + "_close_button");
				closeButton.SetLocalizationEnabled(LocalizationEnabled::None);
				closeButton.SetShadowRadius(0);
				closeButton.SetText("x");
				closeButton.OnClick([this](const EventArgs& args) {
					this->SetVisible(false);
					if (onClose)
					{
						onClose(std::move(args));
					}
					});
				closeButton.SetTooltip("Close window");
				AddWidget(&closeButton, AttachmentOptions::NONE);
				has_titlebar = true;
			}

			if (has_flag(window_controls, WindowControls::COLLAPSE))
			{
				// Add minimize button to the top left corner
				collapseButton.Create(name + "_collapse_button");
				collapseButton.SetLocalizationEnabled(LocalizationEnabled::None);
				collapseButton.SetShadowRadius(0);
				collapseButton.SetText("-");
				collapseButton.OnClick([this](const EventArgs& args) {
					this->SetMinimized(!this->IsMinimized());
					if (onCollapse)
					{
						onCollapse({});
					}
					});
				collapseButton.SetTooltip("Collapse/Expand window");
				AddWidget(&collapseButton, AttachmentOptions::NONE);
				has_titlebar = true;
			}

			if (!has_flag(window_controls, WindowControls::MOVE) && !name.empty())
			{
				// Simple title bar
				label.Create(name);
				label.SetLocalizationEnabled(LocalizationEnabled::None);
				label.SetShadowRadius(0);
				label.SetText(name);
				label.font.params.h_align = wi::font::WIFALIGN_LEFT;
				label.scrollbar.SetEnabled(false);
				label.SetWrapEnabled(false);
				AddWidget(&label, AttachmentOptions::NONE);
				has_titlebar = true;
			}
		}

		scrollbar_horizontal.SetVertical(false);
		scrollbar_horizontal.SetColor(wi::Color(80, 80, 80, 100), wi::gui::IDLE);
		scrollbar_horizontal.sprites_knob[ScrollBar::SCROLLBAR_INACTIVE].params.color = wi::Color(140, 140, 140, 140);
		scrollbar_horizontal.sprites_knob[ScrollBar::SCROLLBAR_HOVER].params.color = wi::Color(180, 180, 180, 180);
		scrollbar_horizontal.sprites_knob[ScrollBar::SCROLLBAR_GRABBED].params.color = wi::Color::White();
		scrollbar_horizontal.knob_inset_border = XMFLOAT2(2, 4);
		scrollbar_horizontal.SetOverScroll(0.1f);
		AddWidget(&scrollbar_horizontal);

		scrollbar_vertical.SetVertical(true);
		scrollbar_vertical.SetColor(wi::Color(80, 80, 80, 100), wi::gui::IDLE);
		scrollbar_vertical.sprites_knob[ScrollBar::SCROLLBAR_INACTIVE].params.color = wi::Color(140, 140, 140, 140);
		scrollbar_vertical.sprites_knob[ScrollBar::SCROLLBAR_HOVER].params.color = wi::Color(180, 180, 180, 180);
		scrollbar_vertical.sprites_knob[ScrollBar::SCROLLBAR_GRABBED].params.color = wi::Color::White();
		scrollbar_vertical.knob_inset_border = XMFLOAT2(4, 2);
		scrollbar_vertical.SetOverScroll(0.1f);
		AddWidget(&scrollbar_vertical);

		scrollable_area.ClearTransform();


		SetEnabled(true);
		SetVisible(true);
		SetMinimized(false);
	}
	void Window::AddWidget(Widget* widget, AttachmentOptions options)
	{
		widget->SetEnabled(this->IsEnabled());
		if (IsVisible() && !IsMinimized())
		{
			widget->SetVisible(true);
		}
		else
		{
			widget->SetVisible(false);
		}
		if (has_flag(options, AttachmentOptions::SCROLLABLE))
		{
			widget->AttachTo(&scrollable_area);
		}
		else
		{
			widget->AttachTo(this);
		}

		widgets.push_back(widget);
	}
	void Window::RemoveWidget(Widget* widget)
	{
		for (auto& x : widgets)
		{
			if (x == widget)
			{
				x = widgets.back();
				widgets.pop_back();
				break;
			}
		}
	}
	void Window::RemoveWidgets()
	{
		widgets.clear();
	}
	void Window::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Hitbox2D pointerHitbox = GetPointerHitbox();

		// Resizer updates:
		if (IsEnabled())
		{
			float vscale = IsCollapsed() ? control_size : scale.y;
			Hitbox2D lefthitbox;
			Hitbox2D righthitbox;
			Hitbox2D tophitbox;
			Hitbox2D bottomhitbox;
			Hitbox2D toplefthitbox;
			Hitbox2D toprighthitbox;
			Hitbox2D bottomrighthitbox;
			Hitbox2D bottomlefthitbox;
			if (has_flag(controls, WindowControls::RESIZE_LEFT))
			{
				lefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y), XMFLOAT2(resizehitboxwidth, vscale));
			}
			if (has_flag(controls, WindowControls::RESIZE_RIGHT))
			{
				righthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x, translation.y), XMFLOAT2(resizehitboxwidth, vscale));
			}
			if (!IsCollapsed())
			{
				if (has_flag(controls, WindowControls::RESIZE_TOP))
				{
					tophitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y - resizehitboxwidth), XMFLOAT2(scale.x, resizehitboxwidth));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOM))
				{
					bottomhitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y + vscale), XMFLOAT2(scale.x, resizehitboxwidth));
				}
				if (has_flag(controls, WindowControls::RESIZE_TOPLEFT))
				{
					toplefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_TOPRIGHT))
				{
					toprighthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x - resizehitboxwidth, translation.y - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOMRIGHT))
				{
					bottomrighthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x - resizehitboxwidth, translation.y + vscale - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOMLEFT))
				{
					bottomlefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y + vscale - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
			}

			if (resize_state == RESIZE_STATE_NONE && (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanStarting()))
			{
				if (pointerHitbox.intersects(toplefthitbox))
				{
					resize_state = RESIZE_STATE_TOPLEFT;
					Activate();
				}
				else if (pointerHitbox.intersects(toprighthitbox))
				{
					resize_state = RESIZE_STATE_TOPRIGHT;
					Activate();
				}
				else if (pointerHitbox.intersects(bottomrighthitbox))
				{
					resize_state = RESIZE_STATE_BOTTOMRIGHT;
					Activate();
				}
				else if (pointerHitbox.intersects(bottomlefthitbox))
				{
					resize_state = RESIZE_STATE_BOTTOMLEFT;
					Activate();
				}
				else if (pointerHitbox.intersects(lefthitbox))
				{
					resize_state = RESIZE_STATE_LEFT;
					Activate();
				}
				else if (pointerHitbox.intersects(righthitbox))
				{
					resize_state = RESIZE_STATE_RIGHT;
					Activate();
				}
				else if (pointerHitbox.intersects(tophitbox))
				{
					resize_state = RESIZE_STATE_TOP;
					Activate();
				}
				else if (pointerHitbox.intersects(bottomhitbox))
				{
					resize_state = RESIZE_STATE_BOTTOM;
					Activate();
				}
				resize_begin = pointerHitbox.pos;
			}
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (resize_state != RESIZE_STATE_NONE)
				{
					auto saved_parent = this->parent;
					this->Detach();
					float deltaX = pointerHitbox.pos.x - resize_begin.x;
					float deltaY = pointerHitbox.pos.y - resize_begin.y;

					switch (resize_state)
					{
					case wi::gui::Window::RESIZE_STATE_LEFT:
						this->Translate(XMFLOAT3(deltaX, 0, 0));
						this->Scale(XMFLOAT3((scale.x - deltaX) / scale.x, 1, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_TOP:
						this->Translate(XMFLOAT3(0, deltaY, 0));
						this->Scale(XMFLOAT3(1, (scale.y - deltaY) / scale.y, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_RIGHT:
						this->Scale(XMFLOAT3((scale.x + deltaX) / scale.x, 1, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_BOTTOM:
						this->Scale(XMFLOAT3(1, (scale.y + deltaY) / scale.y, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_TOPLEFT:
						this->Translate(XMFLOAT3(deltaX, deltaY, 0));
						this->Scale(XMFLOAT3((scale.x - deltaX) / scale.x, (scale.y - deltaY) / scale.y, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_TOPRIGHT:
						this->Translate(XMFLOAT3(0, deltaY, 0));
						this->Scale(XMFLOAT3((scale.x + deltaX) / scale.x, (scale.y - deltaY) / scale.y, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_BOTTOMRIGHT:
						this->Scale(XMFLOAT3((scale.x + deltaX) / scale.x, (scale.y + deltaY) / scale.y, 1));
						break;
					case wi::gui::Window::RESIZE_STATE_BOTTOMLEFT:
						this->Translate(XMFLOAT3(deltaX, 0, 0));
						this->Scale(XMFLOAT3((scale.x - deltaX) / scale.x, (scale.y + deltaY) / scale.y, 1));
						break;
					default:
						break;
					}

					this->scale_local = wi::math::Max(this->scale_local, XMFLOAT3(control_size * 3, control_size * 2, 1)); // don't allow resize to negative or too small
					this->AttachTo(saved_parent);
					resize_begin = pointerHitbox.pos;
				}
			}
			else
			{
				resize_state = RESIZE_STATE_NONE;
			}

			if (
				resize_state != RESIZE_STATE_NONE ||
				pointerHitbox.intersects(toplefthitbox) ||
				pointerHitbox.intersects(toprighthitbox) ||
				pointerHitbox.intersects(bottomlefthitbox) ||
				pointerHitbox.intersects(bottomrighthitbox) ||
				pointerHitbox.intersects(lefthitbox) ||
				pointerHitbox.intersects(righthitbox) ||
				pointerHitbox.intersects(tophitbox) ||
				pointerHitbox.intersects(bottomhitbox)
				)
			{
				resize_blink_timer += dt;
			}
			else
			{
				resize_blink_timer = 0;
			}
		}

		// Corner rounding update for control widgets:
		for (int i = 0; i < arraysize(moveDragger.sprites); ++i)
		{
			moveDragger.sprites[i].params.disableCornerRounding();
			label.sprites[i].params.disableCornerRounding();
			collapseButton.sprites[i].params.disableCornerRounding();
			closeButton.sprites[i].params.disableCornerRounding();
			scrollbar_horizontal.sprites[i].params.disableCornerRounding();
			scrollbar_vertical.sprites[i].params.disableCornerRounding();

			if (sprites[state].params.isCornerRoundingEnabled())
			{
				// Left side:
				if (collapseButton.parent)
				{
					collapseButton.sprites[i].params.enableCornerRounding();
					collapseButton.sprites[i].params.corners_rounding[0].radius = sprites[state].params.corners_rounding[0].radius;
					if (IsCollapsed())
					{
						collapseButton.sprites[i].params.corners_rounding[2].radius = sprites[state].params.corners_rounding[2].radius;
					}
					else
					{
						collapseButton.sprites[i].params.corners_rounding[2].radius = 0;
					}
				}
				else if (moveDragger.parent)
				{
					moveDragger.sprites[i].params.enableCornerRounding();
					moveDragger.sprites[i].params.corners_rounding[0].radius = sprites[state].params.corners_rounding[0].radius;
					if (IsCollapsed())
					{
						moveDragger.sprites[i].params.corners_rounding[2].radius = sprites[state].params.corners_rounding[2].radius;
					}
					else
					{
						moveDragger.sprites[i].params.corners_rounding[2].radius = 0;
					}
				}
				else if (label.parent)
				{
					label.sprites[i].params.enableCornerRounding();
					label.sprites[i].params.corners_rounding[0].radius = sprites[state].params.corners_rounding[0].radius;
					if (IsCollapsed())
					{
						label.sprites[i].params.corners_rounding[2].radius = sprites[state].params.corners_rounding[2].radius;
					}
					else
					{
						label.sprites[i].params.corners_rounding[2].radius = 0;
					}
				}

				// Right side:
				if (closeButton.parent)
				{
					closeButton.sprites[i].params.enableCornerRounding();
					closeButton.sprites[i].params.corners_rounding[1].radius = sprites[state].params.corners_rounding[1].radius;
					if (IsCollapsed())
					{
						closeButton.sprites[i].params.corners_rounding[3].radius = sprites[state].params.corners_rounding[3].radius;
					}
					else
					{
						closeButton.sprites[i].params.corners_rounding[3].radius = 0;
					}
				}
				else if (moveDragger.parent)
				{
					moveDragger.sprites[i].params.enableCornerRounding();
					moveDragger.sprites[i].params.corners_rounding[1].radius = sprites[state].params.corners_rounding[1].radius;
					if (IsCollapsed())
					{
						moveDragger.sprites[i].params.corners_rounding[3].radius = sprites[state].params.corners_rounding[3].radius;
					}
					else
					{
						moveDragger.sprites[i].params.corners_rounding[3].radius = 0;
					}
				}
				else if (label.parent)
				{
					label.sprites[i].params.enableCornerRounding();
					label.sprites[i].params.corners_rounding[1].radius = sprites[state].params.corners_rounding[1].radius;
					if (IsCollapsed())
					{
						label.sprites[i].params.corners_rounding[3].radius = sprites[state].params.corners_rounding[3].radius;
					}
					else
					{
						label.sprites[i].params.corners_rounding[3].radius = 0;
					}
				}

				scrollbar_horizontal.sprites[i].params.enableCornerRounding();
				scrollbar_horizontal.sprites[i].params.corners_rounding[3].radius = sprites[state].params.corners_rounding[3].radius;
				scrollbar_vertical.sprites[i].params.enableCornerRounding();
				scrollbar_vertical.sprites[i].params.corners_rounding[3].radius = sprites[state].params.corners_rounding[3].radius;
			}
		}

		moveDragger.force_disable = force_disable;
		scrollbar_horizontal.force_disable = force_disable;
		scrollbar_vertical.force_disable = force_disable;

		moveDragger.Update(canvas, dt);

		// Don't allow moving outside of screen:
		if (parent == nullptr)
		{
			translation_local.x = wi::math::Clamp(translation_local.x, 0, canvas.GetLogicalWidth() - scale_local.x);
			translation_local.y = wi::math::Clamp(translation_local.y, 0, canvas.GetLogicalHeight() - control_size);
			SetDirty();
		}

		Widget::Update(canvas, dt);

		ResizeLayout();

		uint32_t priority = 0;

		scrollable_area.scissorRect = scissorRect;

		scrollable_area.Detach();
		scrollable_area.ClearTransform();
		scrollable_area.Translate(translation);

		float scroll_length_horizontal = 0;
		float scroll_length_vertical = 0;
		for (auto& widget : widgets)
		{
			if (!widget->IsVisible())
				continue;
			if (widget->parent == &scrollable_area)
			{
				XMFLOAT2 size = widget->GetSize();
				scroll_length_horizontal = std::max(scroll_length_horizontal, widget->translation_local.x + size.x);
				scroll_length_vertical = std::max(scroll_length_vertical, widget->translation_local.y + size.y);
			}
		}

		if (has_flag(controls, WindowControls::FIT_ALL_WIDGETS_VERTICAL))
		{
			if (!IsCollapsed())
			{
				// it will be dynamically sized to fit all widgets:
				auto saved_parent = this->parent;
				this->Detach();
				scale_local.y = control_size + 1 + scroll_length_vertical + 4; // some padding at the bottom
				this->AttachTo(saved_parent);
			}
		}
		else
		{
			// Compute scrollable area:
			if (scrollbar_horizontal.parent != nullptr || scrollbar_vertical.parent != nullptr)
			{
				scrollbar_horizontal.SetListLength(scroll_length_horizontal);
				scrollbar_vertical.SetListLength(scroll_length_vertical);
				scrollbar_horizontal.Update(canvas, 0);
				scrollbar_vertical.Update(canvas, 0);
				scrollable_area.Translate(XMFLOAT3(scrollbar_horizontal.GetOffset(), 1 + scrollbar_vertical.GetOffset(), 0));
				scrollable_area.scissorRect.left += 1;
				if (scrollbar_horizontal.parent != nullptr && scrollbar_horizontal.IsScrollbarRequired())
				{
					scrollable_area.scissorRect.bottom -= (int32_t)control_size + 1;
				}
				if (scrollbar_vertical.parent != nullptr && scrollbar_vertical.IsScrollbarRequired())
				{
					scrollable_area.scissorRect.right -= (int32_t)control_size + 1;
				}
				scrollable_area.active_area.pos.x = float(scrollable_area.scissorRect.left);
				scrollable_area.active_area.pos.y = float(scrollable_area.scissorRect.top);
				scrollable_area.active_area.siz.x = float(scrollable_area.scissorRect.right) - float(scrollable_area.scissorRect.left);
				scrollable_area.active_area.siz.y = float(scrollable_area.scissorRect.bottom) - float(scrollable_area.scissorRect.top);
			}
		}

		if (has_titlebar)
		{
			scrollable_area.Translate(XMFLOAT3(0, control_size, 0));
			scrollable_area.scissorRect.top += (int32_t)control_size;
		}

		scrollable_area.AttachTo(this);

		bool local_force_disable = this->force_disable;

		for (size_t i = 0; i < widgets.size(); ++i)
		{
			Widget* widget = widgets[i]; // re index in loop, because widgets can be realloced while updating!
			widget->force_disable = local_force_disable;
			widget->Update(canvas, dt);
			widget->force_disable = false;

			if (widget->GetState() > IDLE)
			{
				local_force_disable = true;
			}

			if (widget->priority_change)
			{
				this->priority_change = true;
				widget->priority_change = false;
				widget->priority = priority++;
			}
			else
			{
				widget->priority = ~0u;
			}
		}

		if (local_force_disable && state == IDLE)
		{
			state = FOCUS;
		}

		if (priority > 0)
		{
			// Sort only if there are priority changes
			//	Use std::stable_sort instead of std::sort to preserve UI element order with equal priorities
			std::stable_sort(widgets.begin(), widgets.end(), [](const Widget* a, const Widget* b) {
				return a->priority < b->priority;
			});
		}

		if (!IsMinimized() && IsVisible())
		{
			float wheel_delta = wi::input::GetPointer().z * 60.0f;
			if (!local_force_disable)
			{
				wheel_delta -= wi::input::GetTouchPan().y;
			}
			if (wheel_delta != 0.0f && scroll_allowed && scrollbar_vertical.IsScrollbarRequired() && pointerHitbox.intersects(hitBox)) // when window is in focus, but other widgets aren't
			{
				const bool at_begin = (wheel_delta > 0 && scrollbar_vertical.IsScrolledToBegin());
				if (!at_begin)
				{
					scroll_allowed = false;
					// This is outside scrollbar code, because it can also be scrolled if parent widget is only in focus
					scrollbar_vertical.Scroll(wheel_delta);
				}
			}
		}

		if (IsMinimized())
		{
			hitBox.siz.y = control_size;
		}

		if (IsEnabled() && !IsMinimized() && dt > 0)
		{
			if (state == FOCUS)
			{
				state = IDLE;
			}
			if (state == DEACTIVATING)
			{
				state = IDLE;
			}
			if (state == ACTIVE)
			{
				Deactivate();
			}


			bool clicked = false;
			if (pointerHitbox.intersects(hitBox))
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == FOCUS)
				{
					// activate
					clicked = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			{
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}

			if (clicked)
			{
				Activate();
			}
		}
		else
		{
			state = IDLE;
		}

		if (state == IDLE && resize_blink_timer > 0)
			state = FOCUS;
	}
	void Window::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}

		GetDevice()->EventBegin(name.c_str(), cmd);

		wi::Color color = GetColor();

		// shadow:
		if (shadow > 0)
		{
			wi::image::Params fx = sprites[state].params;
			fx.gradient = wi::image::Params::Gradient::None;
			fx.pos.x -= shadow;
			fx.pos.y -= shadow;
			fx.siz.x += shadow * 2;
			fx.siz.y += shadow * 2;
			fx.color = shadow_color;
			if (IsMinimized())
			{
				fx.siz.y = control_size + shadow * 2;
			}
			if (fx.isCornerRoundingEnabled())
			{
				for (auto& corner_rounding : fx.corners_rounding)
				{
					if (corner_rounding.radius > 0)
					{
						corner_rounding.radius += shadow;
					}
				}
			}
			if (shadow_highlight)
			{
				fx.enableHighlight();
				fx.highlight_pos = GetPointerHighlightPos(canvas);
				fx.highlight_color = shadow_highlight_color;
				fx.highlight_spread = shadow_highlight_spread;
			}
			else
			{
				fx.disableHighlight();
			}
			wi::image::Draw(nullptr, fx, cmd);
		}

		// resize indicator:
		{
			// hitboxes are recomputed because window transform might have changed since update!!
			float vscale = IsCollapsed() ? control_size : scale.y;
			Hitbox2D lefthitbox;
			Hitbox2D righthitbox;
			Hitbox2D tophitbox;
			Hitbox2D bottomhitbox;
			Hitbox2D toplefthitbox;
			Hitbox2D toprighthitbox;
			Hitbox2D bottomrighthitbox;
			Hitbox2D bottomlefthitbox;
			if (has_flag(controls, WindowControls::RESIZE_LEFT))
			{
				lefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y), XMFLOAT2(resizehitboxwidth, vscale));
			}
			if (has_flag(controls, WindowControls::RESIZE_RIGHT))
			{
				righthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x, translation.y), XMFLOAT2(resizehitboxwidth, vscale));
			}
			if (!IsCollapsed())
			{
				if (has_flag(controls, WindowControls::RESIZE_TOP))
				{
					tophitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y - resizehitboxwidth), XMFLOAT2(scale.x, resizehitboxwidth));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOM))
				{
					bottomhitbox = Hitbox2D(XMFLOAT2(translation.x, translation.y + vscale), XMFLOAT2(scale.x, resizehitboxwidth));
				}
				if (has_flag(controls, WindowControls::RESIZE_TOPLEFT))
				{
					toplefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_TOPRIGHT))
				{
					toprighthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x - resizehitboxwidth, translation.y - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOMRIGHT))
				{
					bottomrighthitbox = Hitbox2D(XMFLOAT2(translation.x + scale.x - resizehitboxwidth, translation.y + vscale - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
				if (has_flag(controls, WindowControls::RESIZE_BOTTOMLEFT))
				{
					bottomlefthitbox = Hitbox2D(XMFLOAT2(translation.x - resizehitboxwidth, translation.y + vscale - resizehitboxwidth), XMFLOAT2(resizehitboxwidth * 2, resizehitboxwidth * 2));
				}
			}

			const Hitbox2D pointerHitbox = GetPointerHitbox();

			wi::image::Params fx = sprites[state].params;
			fx.blendFlag = wi::enums::BLENDMODE_ALPHA;
			fx.pos.x -= resizehitboxwidth;
			fx.pos.y -= resizehitboxwidth;
			fx.siz.x += resizehitboxwidth * 2;
			fx.siz.y += resizehitboxwidth * 2;
			fx.color = resize_state == RESIZE_STATE_NONE ? sprites[FOCUS].params.color : sprites[ACTIVE].params.color;
			if (IsMinimized())
			{
				fx.siz.y = control_size + resizehitboxwidth * 2;
			}
			if (fx.isCornerRoundingEnabled())
			{
				for (auto& corner_rounding : fx.corners_rounding)
				{
					if (corner_rounding.radius > 0)
					{
						corner_rounding.radius += resizehitboxwidth;
					}
				}
			}
			//fx.border_soften = 0.01f;

			if (resize_state == RESIZE_STATE_TOPLEFT || pointerHitbox.intersects(toplefthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.03f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.025f, std::abs(std::sin(resize_blink_timer * 4)));
				XMStoreFloat2(&fx.angular_softness_direction, XMVector2Normalize(XMVectorSet(-1, -1, 0, 0)));
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NWSE);
			}
			else if (resize_state == RESIZE_STATE_TOPRIGHT || pointerHitbox.intersects(toprighthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.03f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.025f, std::abs(std::sin(resize_blink_timer * 4)));
				XMStoreFloat2(&fx.angular_softness_direction, XMVector2Normalize(XMVectorSet(1, -1, 0, 0)));
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NESW);
			}
			else if (resize_state == RESIZE_STATE_BOTTOMRIGHT || pointerHitbox.intersects(bottomrighthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.03f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.025f, std::abs(std::sin(resize_blink_timer * 4)));
				XMStoreFloat2(&fx.angular_softness_direction, XMVector2Normalize(XMVectorSet(1, 1, 0, 0)));
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NWSE);
			}
			else if (resize_state == RESIZE_STATE_BOTTOMLEFT || pointerHitbox.intersects(bottomlefthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.03f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.025f, std::abs(std::sin(resize_blink_timer * 4)));
				XMStoreFloat2(&fx.angular_softness_direction, XMVector2Normalize(XMVectorSet(-1, 1, 0, 0)));
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NESW);
			}
			else if (resize_state == RESIZE_STATE_LEFT || pointerHitbox.intersects(lefthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.25f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.24f, std::abs(std::sin(resize_blink_timer * 4)));
				fx.angular_softness_direction = XMFLOAT2(-1, 0);
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_EW);
			}
			else if (resize_state == RESIZE_STATE_RIGHT || pointerHitbox.intersects(righthitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.25f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.24f, std::abs(std::sin(resize_blink_timer * 4)));
				fx.angular_softness_direction = XMFLOAT2(1, 0);
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_EW);
			}
			else if (resize_state == RESIZE_STATE_TOP || pointerHitbox.intersects(tophitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.25f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.24f, std::abs(std::sin(resize_blink_timer * 4)));
				fx.angular_softness_direction = XMFLOAT2(0, -1);
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NS);
			}
			else if (resize_state == RESIZE_STATE_BOTTOM || pointerHitbox.intersects(bottomhitbox))
			{
				fx.angular_softness_outer_angle = XM_PI * 0.25f;
				fx.angular_softness_inner_angle = XM_PI * wi::math::Lerp(0.0f, 0.24f, std::abs(std::sin(resize_blink_timer * 4)));
				fx.angular_softness_direction = XMFLOAT2(0, 1);
				wi::image::Draw(nullptr, fx, cmd);
				wi::input::SetCursor(wi::input::CURSOR_RESIZE_NS);
			}
		}

		// base:
		if (!IsCollapsed())
		{
			wi::image::Params params = sprites[IDLE].params;
			const wi::Resource& res = sprites[IDLE].textureResource;
			if (res.IsValid())
			{
				params.sampleFlag = wi::image::SAMPLEMODE_WRAP;
				const Texture& tex = res.GetTexture();
				const float widget_aspect = scale_local.x / scale_local.y;
				const float image_aspect = float(tex.desc.width) / float(tex.desc.height);
				if (widget_aspect > image_aspect)
				{
					// display aspect is wider than image:
					params.texMulAdd.y *= image_aspect / widget_aspect;
				}
				else
				{
					// image aspect is wider or equal to display
					params.texMulAdd.x *= widget_aspect / image_aspect;
					if (right_aligned_image)
					{
						params.texMulAdd.z = 1.0f - params.texMulAdd.x;
					}
				}
			}
			wi::image::Draw(sprites[IDLE].GetTexture(), params, cmd);

			if (background_overlay.IsValid())
			{
				params.blendFlag = wi::enums::BLENDMODE_ADDITIVE;
				params.color.w = 0;
				params.setBackgroundMap(&background_overlay);
				wi::image::Draw(nullptr, params, cmd);
			}
		}

		for (size_t i = 0; i < widgets.size(); ++i)
		{
			const Widget* widget = widgets[widgets.size() - i - 1];
			if (widget->parent == nullptr)
			{
				ApplyScissor(canvas, scissorRect, cmd);
			}
			else
			{
				ApplyScissor(canvas, widget->parent->scissorRect, cmd);
			}
			widget->Render(canvas, cmd);
		}

		//Rect scissorRect;
		//scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
		//scissorRect.left = (int32_t)(0);
		//scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
		//scissorRect.top = (int32_t)(0);
		//GraphicsDevice* device = wi::graphics::GetDevice();
		//device->BindScissorRects(1, &scissorRect, cmd);
		//wi::image::Draw(nullptr, wi::image::Params(scrollable_area.active_area.pos.x, scrollable_area.active_area.pos.y, scrollable_area.active_area.siz.x, scrollable_area.active_area.siz.y, wi::Color(255,0,255,100)), cmd);
		//Hitbox2D p = scrollable_area.GetPointerHitbox();
		//wi::image::Draw(nullptr, wi::image::Params(p.pos.x, p.pos.y, p.siz.x * 10, p.siz.y * 10, wi::Color(255,0,0,100)), cmd);
		//if (!IsCollapsed())
		//{
		//	wi::image::Draw(nullptr, wi::image::Params(scrollable_area.translation.x, scrollable_area.translation.y, scale.x, 10, wi::Color(255,0,255,100)), cmd);
		//}

		GetDevice()->EventEnd(cmd);
	}
	void Window::RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
	{
		// Window base tooltip is not rendered
		for (auto& x : widgets)
		{
			x->RenderTooltip(canvas, cmd);
		}
	}
	void Window::SetVisible(bool value)
	{
		Widget::SetVisible(value);
		bool minimized = IsMinimized();
		for (auto& x : widgets)
		{
			if (
				x == &closeButton ||
				x == &collapseButton ||
				x == &moveDragger ||
				x == &label
				)
			{
				x->SetVisible(value);
			}
			else
			{
				x->SetVisible(!minimized);
			}
		}
	}
	void Window::SetEnabled(bool value)
	{
		Widget::SetEnabled(value);
		for (auto& x : widgets)
		{
			if (x == &moveDragger)
				continue;
			if (x == &collapseButton)
				continue;
			if (x == &closeButton)
				continue;
			if (x == &scrollbar_horizontal)
				continue;
			if (x == &scrollbar_vertical)
				continue;
			x->SetEnabled(value);
		}
	}
	void Window::SetCollapsed(bool value)
	{
		SetMinimized(value);
	}
	bool Window::IsCollapsed() const
	{
		return IsMinimized();
	}
	void Window::SetMinimized(bool value)
	{
		minimized = value;

		for (auto& x : widgets)
		{
			if (
				x == &closeButton ||
				x == &collapseButton ||
				x == &moveDragger ||
				x == &label
				)
			{
				continue;
			}
			x->SetVisible(!value);
		}

		scrollable_area.SetVisible(!value);

		if (IsMinimized())
		{
			collapseButton.SetText("»");
			collapseButton.font.params.rotation = XM_PIDIV2;
		}
		else
		{
			collapseButton.SetText("-");
			collapseButton.font.params.rotation = 0;
		}
	}
	bool Window::IsMinimized() const
	{
		return minimized;
	}
	void Window::SetControlSize(float value)
	{
		control_size = value;
	}
	float Window::GetControlSize() const
	{
		return control_size;
	}
	XMFLOAT2 Window::GetSize() const
	{
		XMFLOAT2 size = Widget::GetSize();
		if (IsCollapsed())
		{
			return XMFLOAT2(size.x, control_size);
		}
		return size;
	}
	XMFLOAT2 Window::GetWidgetAreaSize() const
	{
		XMFLOAT2 size = GetSize();
		if (scrollbar_horizontal.IsScrollbarRequired())
		{
			size.y -= control_size;
		}
		if (scrollbar_vertical.IsScrollbarRequired())
		{
			size.x -= control_size;
		}
		return size;
	}
	void Window::OnClose(std::function<void(const EventArgs& args)> func)
	{
		onClose = std::move(func);
	}
	void Window::OnCollapse(std::function<void(const EventArgs& args)> func)
	{
		onCollapse = std::move(func);
	}
	void Window::OnResize(std::function<void()> func)
	{
		onResize = std::move(func);
	}
	void Window::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);
		for (auto& widget : widgets)
		{
			widget->SetColor(color, id);
		}

		if (id == WIDGET_ID_WINDOW_BASE)
		{
			sprites[IDLE].params.color = color;
		}
	}
	void Window::SetImage(wi::Resource resource, int id)
	{
		Widget::SetImage(resource, id);
		for (auto& widget : widgets)
		{
			widget->SetImage(resource, id);
		}

		if (id == WIDGET_ID_WINDOW_BASE)
		{
			sprites[IDLE].textureResource = resource;
		}
	}
	void Window::SetShadowColor(wi::Color color)
	{
		Widget::SetShadowColor(color);
		for (auto& widget : widgets)
		{
			widget->SetShadowColor(color);
		}
	}
	void Window::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		for (auto& widget : widgets)
		{
			widget->SetTheme(theme, id);
		}
	}
	void Window::ResizeLayout()
	{
		Widget::ResizeLayout();
		layout.reset(*this);
		for (auto& widget : widgets)
		{
			widget->ResizeLayout();
		}

		if (moveDragger.parent != nullptr)
		{
			moveDragger.Detach();
			float rem = 0;
			if (closeButton.parent != nullptr)
			{
				rem++;
			}
			if (collapseButton.parent != nullptr)
			{
				rem++;
			}
			moveDragger.SetSize(XMFLOAT2(scale.x - control_size * rem, control_size));
			float offset = 0;
			if (collapseButton.parent != nullptr)
			{
				offset++;
			}
			moveDragger.SetPos(XMFLOAT2(translation.x + control_size * offset, translation.y));
			moveDragger.AttachTo(this);
		}
		if (closeButton.parent != nullptr)
		{
			closeButton.Detach();
			closeButton.SetSize(XMFLOAT2(control_size, control_size));
			closeButton.SetPos(XMFLOAT2(translation.x + scale.x - control_size, translation.y));
			closeButton.AttachTo(this);
		}
		if (collapseButton.parent != nullptr)
		{
			collapseButton.Detach();
			collapseButton.SetSize(XMFLOAT2(control_size, control_size));
			collapseButton.SetPos(XMFLOAT2(translation.x, translation.y));
			collapseButton.AttachTo(this);
		}
		if (label.parent != nullptr)
		{
			label.font.params = font.params;
			label.Detach();
			XMFLOAT2 label_size = XMFLOAT2(scale.x, control_size);
			XMFLOAT2 label_pos = XMFLOAT2(translation.x, translation.y);
			if (closeButton.parent != nullptr)
			{
				label_size.x -= control_size;
			}
			if (collapseButton.parent != nullptr)
			{
				label_size.x -= control_size;
				label_pos.x += control_size;
			}
			label.SetSize(label_size);
			label.SetPos(label_pos);
			label.AttachTo(this);
		}
		if (scrollbar_horizontal.parent != nullptr)
		{
			scrollbar_horizontal.Detach();
			scrollbar_horizontal.SetSize(XMFLOAT2(GetWidgetAreaSize().x - control_size, control_size));
			scrollbar_horizontal.SetPos(XMFLOAT2(translation.x + control_size, translation.y + scale.y - control_size));
			scrollbar_horizontal.AttachTo(this);
			scrollbar_horizontal.SetSafeArea(control_size * 2);
		}
		if (scrollbar_vertical.parent != nullptr)
		{
			scrollbar_vertical.Detach();
			scrollbar_vertical.SetSize(XMFLOAT2(control_size, GetWidgetAreaSize().y - control_size));
			scrollbar_vertical.SetPos(XMFLOAT2(translation.x + scale.x - control_size, translation.y + control_size));
			scrollbar_vertical.AttachTo(this);
		}

		if (onResize)
		{
			onResize();
		}
	}
	void Window::ExportLocalization(wi::Localization& localization) const
	{
		Widget::ExportLocalization(localization);
		if (has_flag(localization_enabled, LocalizationEnabled::Children))
		{
			wi::Localization& section = localization.GetSection(GetName());
			for (auto& widget : widgets)
			{
				widget->ExportLocalization(section);
			}
		}
	}
	void Window::ImportLocalization(const wi::Localization& localization)
	{
		Widget::ImportLocalization(localization);
		if (has_flag(localization_enabled, LocalizationEnabled::Children))
		{
			const wi::Localization* section = localization.CheckSection(GetName());
			if (section == nullptr)
				return;
			for (auto& widget : widgets)
			{
				widget->ImportLocalization(*section);
			}
		}
		label.SetText(GetText());
		moveDragger.SetText(GetText());
	}
}
