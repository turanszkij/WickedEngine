#include "wiGUI/components/Slider.h"
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
	void Slider::Create(float start, float end, float defaultValue, float step, const std::string& name)
	{
		this->start = start;
		this->end = end;
		this->value = defaultValue;
		this->step = std::max(step, 1.0f);

		SetName(name);
		SetText(name);
		OnSlide([](const EventArgs& args) {});
		SetSize(XMFLOAT2(200, 20));

		valueInputField.Create(name + "_endInputField");
		valueInputField.SetLocalizationEnabled(LocalizationEnabled::None);
		valueInputField.SetShadowRadius(0);
		valueInputField.SetTooltip("Enter number to modify value even outside slider limits. Arithmetic expressions are supported (e.g. 10 * 2, 1 + 0.5). Other inputs:\n - reset : reset slider to initial state.\n - FLT_MAX : float max value\n - -FLT_MAX : negative float max value.");
		valueInputField.SetValue(end);
		valueInputField.OnInputAccepted([this, start, end, defaultValue](EventArgs args) {
			if (args.sValue.compare("reset") == 0)
			{
				this->value = defaultValue;
				this->start = start;
				this->end = end;
				args.fValue = this->value;
			}
			else if (args.sValue.compare("FLT_MAX") == 0)
			{
				this->value = FLT_MAX;
				args.fValue = this->value;
			}
			else if (args.sValue.compare("-FLT_MAX") == 0)
			{
				this->value = -FLT_MAX;
				args.fValue = this->value;
			}
			else
			{
				this->value = args.fValue;
				this->start = std::min(this->start, args.fValue);
				this->end = std::max(this->end, args.fValue);
			}
			onSlide(args);
		});

		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites_knob[i].params = sprites[i].params;
		}

		sprites[IDLE].params.color = wi::Color(60, 60, 60, 200);
		sprites[FOCUS].params.color = wi::Color(50, 50, 50, 200);
		sprites[ACTIVE].params.color = wi::Color(50, 50, 50, 200);
		sprites[DEACTIVATING].params.color = wi::Color(60, 60, 60, 200);

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	void Slider::SetValue(float value)
	{
		this->value = value;
	}
	void Slider::SetValue(int value)
	{
		this->value = float(value);
	}
	float Slider::GetValue() const
	{
		return value;
	}
	void Slider::SetRange(float start, float end)
	{
		this->start = start;
		this->end = end;
		this->value = wi::math::Clamp(this->value, start, end);
	}
	void Slider::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		valueInputField.Detach();
		if (state != ACTIVE)
		{
			// only set input field size when slider is not dragged because now it will modify slider active size too, and slider size shouldn't be modified while dragging!
			valueInputField.SetSize(XMFLOAT2(std::max(scale.y, wi::font::TextWidth(valueInputField.GetCurrentInputValue(), valueInputField.font.params) + 4), scale.y));
			valueInputField.SetPos(XMFLOAT2(translation.x + scale.x - valueInputField.GetSize().x, translation.y));
		}
		valueInputField.AttachTo(this);

		hitBox.siz.x = scale.x - valueInputField.GetSize().x - 1;

		scissorRect.bottom = (int32_t)std::ceil(translation.y + scale.y);
		scissorRect.left = (int32_t)std::floor(translation.x);
		scissorRect.right = (int32_t)std::ceil(translation.x + scale.x);
		scissorRect.top = (int32_t)std::floor(translation.y);

		for (int i = 0; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites_knob[i].params.siz.x = 16.0f;
		}
		valueInputField.SetEnabled(enabled);
		valueInputField.force_disable = force_disable;
		valueInputField.Update(canvas, dt);

		if (IsEnabled() && dt > 0)
		{
			bool dragged = false;

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
				if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
				{
					if (state == ACTIVE)
					{
						// continue drag if already grabbed whether it is intersecting or not
						dragged = true;
					}
				}
				else
				{
					Deactivate();
				}
			}

			Hitbox2D pointerHitbox = GetPointerHitbox();

			if (pointerHitbox.intersects(hitBox))
			{
				// hover the slider
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (state == FOCUS)
				{
					// activate
					dragged = true;
				}
			}


			if (dragged)
			{
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				value = wi::math::InverseLerp(translation.x, translation.x + hitBox.siz.x, args.clickPos.x);
				value = wi::math::Clamp(value, 0, 1);
				value *= step;
				value = std::floor(value);
				value /= step;
				value = wi::math::Lerp(start, end, value);
				args.fValue = value;
				args.iValue = (int)value;
				onSlide(args);
				Activate();
			}
		}

		valueInputField.SetValue(value);

		font.params.posY = translation.y + scale.y * 0.5f;

		const float knobWidth = sprites_knob[state].params.siz.x;
		const float knobWidthHalf = knobWidth * 0.5f;
		sprites_knob[state].params.pos.x = wi::math::Lerp(translation.x + knobWidthHalf + 2, translation.x + hitBox.siz.x - knobWidthHalf - 2, wi::math::Clamp(wi::math::InverseLerp(start, end, value), 0, 1));
		sprites_knob[state].params.pos.y = translation.y + 2;
		sprites_knob[state].params.siz.y = scale.y - 4;
		sprites_knob[state].params.pivot = XMFLOAT2(0.5f, 0);
		sprites_knob[state].params.fade = sprites[state].params.fade;

		sprites[state].params.siz.x = hitBox.siz.x;

		left_text_width = font.TextWidth();
	}
	void Slider::Render(const wi::Canvas& canvas, CommandList cmd) const
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
			fx.siz.x = scale.x;
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

		font.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		// base
		sprites[state].Draw(cmd);

		// knob
		sprites_knob[state].Draw(cmd);

		// input field
		valueInputField.Render(canvas, cmd);
	}
	void Slider::RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
	{
		Widget::RenderTooltip(canvas, cmd);
		valueInputField.RenderTooltip(canvas, cmd);
	}
	void Slider::OnSlide(std::function<void(const EventArgs& args)> func)
	{
		onSlide = std::move(func);
	}
	void Slider::SetColor(wi::Color color, int id)
	{
		Widget::SetColor(color, id);
		valueInputField.SetColor(color, id);

		if (id > WIDGET_ID_SLIDER_BEGIN && id < WIDGET_ID_SLIDER_END)
		{
			if (id >= WIDGET_ID_SLIDER_KNOB_IDLE)
			{
				sprites_knob[id - WIDGET_ID_SLIDER_KNOB_IDLE].params.color = color;
			}
			else if (id >= WIDGET_ID_SLIDER_BASE_IDLE)
			{
				sprites[id - WIDGET_ID_SLIDER_BASE_IDLE].params.color = color;
			}
		}
	}
	void Slider::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		valueInputField.SetTheme(theme, id);

		if (id > WIDGET_ID_SLIDER_BEGIN && id < WIDGET_ID_SLIDER_END)
		{
			if (id >= WIDGET_ID_SLIDER_KNOB_IDLE)
			{
				theme.image.Apply(sprites_knob[id - WIDGET_ID_SLIDER_KNOB_IDLE].params);
			}
			else if (id >= WIDGET_ID_SLIDER_BASE_IDLE)
			{
				theme.image.Apply(sprites[id - WIDGET_ID_SLIDER_BASE_IDLE].params);
			}
		}
	}
}
