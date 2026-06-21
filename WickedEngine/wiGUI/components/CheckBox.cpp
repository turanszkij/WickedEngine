#include "wiGUI/components/CheckBox.h"
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
	std::wstring check_text_global;
	void CheckBox::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnClick([](const EventArgs& args) {});
		SetSize(XMFLOAT2(20, 20));

		font.params.h_align = wi::font::WIFALIGN_RIGHT;
		font.params.v_align = wi::font::WIFALIGN_CENTER;
	}
	void CheckBox::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

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

			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();

			// hover the button
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
					Activate();
				}
			}

			if (state == DEACTIVATING)
			{
				if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
				{
					// Keep pressed until mouse is released
					Activate();
				}
				else
				{
					// Deactivation event
					SetCheck(!GetCheck());
					EventArgs args;
					args.clickPos = pointerHitbox.pos;
					args.bValue = GetCheck();
					onClick(args);
				}
			}
		}

		font.params.posY = translation.y + scale.y * 0.5f;

		left_text_width = font.TextWidth();
	}
	void CheckBox::Render(const wi::Canvas& canvas, CommandList cmd) const
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

		font.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		// control
		sprites[state].Draw(cmd);

		// check
		if (GetCheck())
		{
			if (!check_text.empty())
			{
				// render text symbol:
				wi::font::Params params;
				params.posX = translation.x + scale.x * 0.5f;
				params.posY = translation.y + scale.y * 0.5f;
				params.h_align = wi::font::WIFALIGN_CENTER;
				params.v_align = wi::font::WIFALIGN_CENTER;
				params.size = int(scale.y);
				params.scaling = 0.75f;
				params.color = font.params.color;
				wi::font::Draw(check_text, params, cmd);
			}
			else if (!check_text_global.empty())
			{
				// render text symbol:
				wi::font::Params params;
				params.posX = translation.x + scale.x * 0.5f;
				params.posY = translation.y + scale.y * 0.5f;
				params.h_align = wi::font::WIFALIGN_CENTER;
				params.v_align = wi::font::WIFALIGN_CENTER;
				params.size = int(scale.y);
				params.scaling = 0.75f;
				params.color = font.params.color;
				wi::font::Draw(check_text_global, params, cmd);
			}
			else
			{
				// simple square:
				wi::image::Params params(
					translation.x + scale.x * 0.25f,
					translation.y + scale.y * 0.25f,
					scale.x * 0.5f,
					scale.y * 0.5f
				);
				params.color = font.params.color;
				wi::image::Draw(nullptr, params, cmd);
			}
		}
		else if (!uncheck_text.empty())
		{
			wi::font::Params params;
			params.posX = translation.x + scale.x * 0.5f;
			params.posY = translation.y + scale.y * 0.5f;
			params.h_align = wi::font::WIFALIGN_CENTER;
			params.v_align = wi::font::WIFALIGN_CENTER;
			params.size = int(scale.y);
			params.scaling = 0.75f;
			params.color = font.params.color;
			wi::font::Draw(uncheck_text, params, cmd);
		}

	}
	void CheckBox::OnClick(std::function<void(const EventArgs& args)> func)
	{
		onClick = std::move(func);
	}
	void CheckBox::SetCheck(bool value)
	{
		checked = value;
	}
	bool CheckBox::GetCheck() const
	{
		return checked;
	}

	void CheckBox::SetCheckTextGlobal(const std::string& text)
	{
		wi::helper::StringConvert(text, check_text_global);
	}
	void CheckBox::SetCheckText(const std::string& text)
	{
		wi::helper::StringConvert(text, check_text);
	}
	void CheckBox::SetUnCheckText(const std::string& text)
	{
		wi::helper::StringConvert(text, uncheck_text);
	}
}
