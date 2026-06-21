#include "wiGUI/components/ScrollBar.h"
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
	void ScrollBar::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		float scrollbar_begin;
		float scrollbar_end;
		float scrollbar_size;

		if (vertical)
		{
			scrollbar_begin = translation.y;
			scrollbar_end = scrollbar_begin + scale.y;
			scrollbar_size = scrollbar_end - scrollbar_begin;
			scrollbar_granularity = std::min(1.0f, scrollbar_size / std::max(1.0f, list_length - safe_area));
			scrollbar_length = std::max(scale.x * 2, scrollbar_size * scrollbar_granularity);
			scrollbar_length = std::min(scrollbar_length, scale.y);
		}
		else
		{
			scrollbar_begin = translation.x;
			scrollbar_end = scrollbar_begin + scale.x;
			scrollbar_size = scrollbar_end - scrollbar_begin;
			scrollbar_granularity = std::min(1.0f, scrollbar_size / std::max(1.0f, list_length - safe_area));
			scrollbar_length = std::max(scale.y * 2, scrollbar_size * scrollbar_granularity);
			scrollbar_length = std::min(scrollbar_length, scale.x);
		}
		scrollbar_length = std::max(0.0f, scrollbar_length);

		if (IsEnabled() && dt > 0)
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

			Hitbox2D pointerHitbox = GetPointerHitbox();

			if (state == IDLE && hitBox.intersects(pointerHitbox))
			{
				state = FOCUS;
			}

			bool clicked = false;
			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				clicked = true;
			}

			bool click_down = false;
			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				click_down = true;
				if (state == FOCUS || state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();
				}
			}

			if (!click_down)
			{
				scrollbar_state = SCROLLBAR_INACTIVE;
			}

			if (IsScrollbarRequired() && hitBox.intersects(pointerHitbox))
			{
				if (clicked)
				{
					scrollbar_state = SCROLLBAR_GRABBED;
					grab_pos = pointerHitbox.pos;
					grab_pos.x = wi::math::Clamp(grab_pos.x, scrollbar_begin + scrollbar_delta, scrollbar_begin + scrollbar_delta + scrollbar_length);
					grab_pos.y = wi::math::Clamp(grab_pos.y, scrollbar_begin + scrollbar_delta, scrollbar_begin + scrollbar_delta + scrollbar_length);
					grab_delta = scrollbar_delta;
				}
				else if (!click_down)
				{
					scrollbar_state = SCROLLBAR_HOVER;
					state = FOCUS;
				}
			}

			if (scrollbar_state == SCROLLBAR_GRABBED)
			{
				Activate();
				if (vertical)
				{
					scrollbar_delta = grab_delta + pointerHitbox.pos.y - grab_pos.y;
				}
				else
				{
					scrollbar_delta = grab_delta + pointerHitbox.pos.x - grab_pos.x;
				}
			}
		}

		scrollbar_delta = wi::math::Clamp(scrollbar_delta, 0, scrollbar_size - scrollbar_length);
		if (scrollbar_begin < scrollbar_end - scrollbar_length)
		{
			scrollbar_value = wi::math::InverseLerp(scrollbar_begin, scrollbar_end - scrollbar_length, scrollbar_begin + scrollbar_delta);
		}
		else
		{
			scrollbar_value = 0;
		}

		list_offset = -scrollbar_value * (list_length - scrollbar_size * (1.0f - overscroll));

		if (vertical)
		{
			for (int i = 0; i < arraysize(sprites_knob); ++i)
			{
				sprites_knob[i].params.pos.x = translation.x + knob_inset_border.x;
				sprites_knob[i].params.pos.y = translation.y + knob_inset_border.y + scrollbar_delta;
				sprites_knob[i].params.siz.x = std::max(0.0f, scale.x - knob_inset_border.x * 2);
				sprites_knob[i].params.siz.y = std::max(0.0f, scrollbar_length - knob_inset_border.y * 2);
			}
		}
		else
		{
			for (int i = 0; i < arraysize(sprites_knob); ++i)
			{
				sprites_knob[i].params.pos.x = translation.x + knob_inset_border.x + scrollbar_delta;
				sprites_knob[i].params.pos.y = translation.y + knob_inset_border.y;
				sprites_knob[i].params.siz.x = std::max(0.0f, scrollbar_length - knob_inset_border.x * 2);
				sprites_knob[i].params.siz.y = std::max(0.0f, scale.y - knob_inset_border.y * 2);
			}
		}

		if (!IsScrollbarRequired())
		{
			state = IDLE;
			list_offset = 0;
		}
	}
	void ScrollBar::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		Widget::Render(canvas, cmd);
		if (!IsVisible())
		{
			return;
		}
		if (!IsScrollbarRequired())
			return;

		// scrollbar background
		wi::image::Params fx = sprites[state].params;
		fx.pos = XMFLOAT3(translation.x, translation.y, 0);
		fx.siz = XMFLOAT2(scale.x, scale.y);
		fx.color = sprites[IDLE].params.color;
		wi::image::Draw(nullptr, fx, cmd);


		// shadow:
		if (shadow > 0)
		{
			wi::image::Params fx = sprites_knob[scrollbar_state].params;
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

		// scrollbar knob
		sprites_knob[scrollbar_state].Draw(cmd);

		//Rect scissorRect;
		//scissorRect.bottom = (int32_t)(canvas.GetPhysicalHeight());
		//scissorRect.left = (int32_t)(0);
		//scissorRect.right = (int32_t)(canvas.GetPhysicalWidth());
		//scissorRect.top = (int32_t)(0);
		//GraphicsDevice* device = wi::graphics::GetDevice();
		//device->BindScissorRects(1, &scissorRect, cmd);
		//wi::image::Draw(nullptr, wi::image::Params(hitBox.pos.x, hitBox.pos.y, hitBox.siz.x, hitBox.siz.y, wi::Color(255,0,0,100)), cmd);

	}
	void ScrollBar::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);

		if (id > WIDGET_ID_SCROLLBAR_BEGIN && id < WIDGET_ID_SCROLLBAR_END)
		{
			if (id >= WIDGET_ID_SCROLLBAR_KNOB_INACTIVE)
			{
				sprites_knob[id - WIDGET_ID_SCROLLBAR_KNOB_INACTIVE].params.color = color;
			}
			else if (id >= WIDGET_ID_SCROLLBAR_BASE_IDLE)
			{
				sprites[id - WIDGET_ID_SCROLLBAR_BASE_IDLE].params.color = color;
			}
		}
	}
	void ScrollBar::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);

		if (id > WIDGET_ID_SCROLLBAR_BEGIN && id < WIDGET_ID_SCROLLBAR_END)
		{
			if (id >= WIDGET_ID_SCROLLBAR_KNOB_INACTIVE)
			{
				theme.image.Apply(sprites_knob[id - WIDGET_ID_SCROLLBAR_KNOB_INACTIVE].params);
			}
			else if (id >= WIDGET_ID_SCROLLBAR_BASE_IDLE)
			{
				theme.image.Apply(sprites[id - WIDGET_ID_SCROLLBAR_BASE_IDLE].params);
			}
		}
	}
	void ScrollBar::SetOffset(float value)
	{
		float scrollbar_begin;
		float scrollbar_end;
		float scrollbar_size;

		if (vertical)
		{
			scrollbar_begin = translation.y;
			scrollbar_end = scrollbar_begin + scale.y;
			scrollbar_size = scrollbar_end - scrollbar_begin;
		}
		else
		{
			scrollbar_begin = translation.x;
			scrollbar_end = scrollbar_begin + scale.x;
			scrollbar_size = scrollbar_end - scrollbar_begin;
		}

		scrollbar_delta = lerp(0.0f, scrollbar_size - scrollbar_length, value / list_length);
	}
}
