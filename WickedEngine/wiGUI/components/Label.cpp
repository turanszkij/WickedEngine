#include "wiGUI/components/Label.h"
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
	void Label::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		SetSize(XMFLOAT2(100, 20));
		SetColor(GetColor()); // all states use same color by default

		scrollbar.SetColor(wi::Color(80, 80, 80, 100), wi::gui::IDLE);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_INACTIVE].params.color = wi::Color(140, 140, 140, 140);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_HOVER].params.color = wi::Color(180, 180, 180, 180);
		scrollbar.sprites_knob[ScrollBar::SCROLLBAR_GRABBED].params.color = wi::Color::White();
		scrollbar.SetOverScroll(0.25f);
		scrollbar.knob_inset_border = XMFLOAT2(4, 2);
	}
	void Label::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}
		Widget::Update(canvas, dt);

		if (wrap_enabled)
		{
			font.params.h_wrap = scale.x - margin_left - margin_right;
		}
		else
		{
			font.params.h_wrap = -1;
		}

		switch (font.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font.params.posX = translation.x + 2 + margin_left;
			break;
		case wi::font::WIFALIGN_RIGHT:
			font.params.posX = translation.x + scale.x - 2 - margin_right;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font.params.posY = translation.y + 2 + margin_top;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font.params.posY = translation.y + scale.y - 2 - margin_bottom;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posY = translation.y + scale.y * 0.5f;
			break;
		}

		const float textheight = font.TextHeight();

		if (fittext_enabled)
		{
			SetSize(XMFLOAT2(GetSize().x, textheight + 4 + margin_top + margin_bottom));
		}

		if (scrollbar.IsEnabled())
		{
			scrollbar.SetListLength(textheight);
			scrollbar.ClearTransform();
			scrollbar.SetPos(XMFLOAT2(translation.x + scale.x - scrollbar_width, translation.y));
			scrollbar.SetSize(XMFLOAT2(scrollbar_width, scale.y));
			scrollbar.Update(canvas, dt);
		}

		if (IsEnabled() && dt > 0)
		{
			if (state == ACTIVE)
			{
				Deactivate();
			}

			const Hitbox2D pointerHitbox = GetPointerHitbox();
			if (scroll_allowed && scrollbar.IsEnabled() && scrollbar.IsScrollbarRequired() && pointerHitbox.intersects(hitBox))
			{
				const float wheel_delta = wi::input::GetPointer().z;
				const bool at_begin = (wheel_delta > 0 && scrollbar.IsScrolledToBegin());
				if (wheel_delta != 0.0f && !at_begin)
				{
					scroll_allowed = false;
					// This is outside scrollbar code, because it can also be scrolled if parent widget is only in focus
					scrollbar.Scroll(wheel_delta * 60.0f);
				}
				state = FOCUS;
			}
			else
			{
				state = IDLE;
			}

			if (pointerHitbox.intersects(hitBox) && wi::input::Press(wi::input::MOUSE_BUTTON_LEFT))
			{
				Activate();
			}
		}

		if (scrollbar.IsEnabled())
		{
			if (scrollbar.IsScrollbarRequired())
			{
				font.params.h_wrap = scale.x - scrollbar_width;
			}
			font.params.posY += scrollbar.GetOffset();
		}
	}
	void Label::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}

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

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[IDLE].Draw(cmd);
		font.Draw(cmd);

		if (scrollbar.IsEnabled())
		{
			scrollbar.Render(canvas, cmd);
		}
	}
	void Label::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);
		scrollbar.SetColor(color, id);
	}
	void Label::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		scrollbar.SetTheme(theme, id);
	}
}
