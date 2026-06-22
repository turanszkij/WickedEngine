/**
 * @file
 * Implementation of the @ref wi::gui::Button widget.
 */

#include "wiGUI/components/Button.h"
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
	void Button::Create(const std::string& name)
	{
		SetName(name);
		SetText(name);
		OnClick([](const EventArgs& args) {});
		OnRightClick([](const EventArgs& args) {});
		OnDragStart([](const EventArgs& args) {});
		OnDrag([](const EventArgs& args) {});
		OnDragEnd([](const EventArgs& args) {});
		SetSize(XMFLOAT2(100, 20));

		font.params.h_align = wi::font::WIFALIGN_CENTER;
		font.params.v_align = wi::font::WIFALIGN_CENTER;

		font_description.params = font.params;
		font_description.params.h_align = wi::font::WIFALIGN_RIGHT;
	}
	void Button::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		Widget::Update(canvas, dt);

		if (IsEnabled() && dt > 0)
		{
			hitBox.pos.x = translation.x;
			hitBox.pos.y = translation.y;
			hitBox.siz.x = scale.x;
			hitBox.siz.y = scale.y;

			Hitbox2D pointerHitbox = GetPointerHitbox();

			if (state == FOCUS)
			{
				state = IDLE;
			}
			if (state == DEACTIVATING)
			{
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				onDragEnd(args);

				if (pointerHitbox.intersects(hitBox) && !disableClicking)
				{
					// Click occurs when the button is released within the bounds
					onClick(args);
				}

				state = IDLE;
			}
			if (state == ACTIVE)
			{
				Deactivate();
			}

			bool leftButtonClicked = false;
			bool rightButtonClicked = false;
			// hover the button
			if (pointerHitbox.intersects(hitBox))
			{
				if (state == IDLE)
				{
					state = FOCUS;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanStarting())
			{
				if (state == FOCUS)
				{
					// activate
					leftButtonClicked = true;
				}
			}

			if (wi::input::Press(wi::input::MOUSE_BUTTON_RIGHT))
			{
				if (state == FOCUS)
				{
					// right-click
					rightButtonClicked = true;
				}
			}

			if (wi::input::Down(wi::input::MOUSE_BUTTON_LEFT) || wi::input::IsTouchPanning())
			{
				if (state == DEACTIVATING)
				{
					// Keep pressed until mouse is released
					Activate();

					EventArgs args;
					args.clickPos = pointerHitbox.pos;
					XMFLOAT3 posDelta;
					posDelta.x = pointerHitbox.pos.x - prevPos.x;
					posDelta.y = pointerHitbox.pos.y - prevPos.y;
					posDelta.z = 0;
					args.deltaPos = XMFLOAT2(posDelta.x, posDelta.y);
					onDrag(args);
				}
			}

			if (leftButtonClicked)
			{
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				dragStart = args.clickPos;
				args.startPos = dragStart;
				onDragStart(args);
				Activate();
			}

			if (rightButtonClicked)
			{
				EventArgs args;
				args.clickPos = pointerHitbox.pos;
				onRightClick(args);
				Activate();
			}

			prevPos.x = pointerHitbox.pos.x;
			prevPos.y = pointerHitbox.pos.y;
		}

		switch (font.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font.params.posX = translation.x + 2;
			break;
		case wi::font::WIFALIGN_RIGHT:
			font.params.posX = translation.x + scale.x - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font.params.posY = translation.y + 2;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font.params.posY = translation.y + scale.y - 2;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font.params.posY = translation.y + scale.y * 0.5f;
			break;
		}

		left_text_width = 0;
		right_text_width = 0;

		font_description.params.posX = translation.x;
		font_description.params.posY = translation.y + scale.y * 0.5f;
		switch (font_description.params.h_align)
		{
		case wi::font::WIFALIGN_LEFT:
			font_description.params.posX = translation.x + scale.x + shadow;
			right_text_width = font_description.TextWidth();
			break;
		case wi::font::WIFALIGN_RIGHT:
			font_description.params.posX = translation.x - shadow;
			left_text_width = font_description.TextWidth();
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font_description.params.posX = translation.x + scale.x * 0.5f;
			break;
		}
		switch (font_description.params.v_align)
		{
		case wi::font::WIFALIGN_TOP:
			font_description.params.posY = translation.y + scale.y + shadow;
			break;
		case wi::font::WIFALIGN_BOTTOM:
			font_description.params.posY = translation.y - shadow;
			break;
		case wi::font::WIFALIGN_CENTER:
		default:
			font_description.params.posY = translation.y + scale.y * 0.5f;
			break;
		}

		if (state <= FOCUS)
		{
			disableClicking = false;
		}
	}
	void Button::Render(const wi::Canvas& canvas, CommandList cmd) const
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

		font_description.Draw(cmd);

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[state].Draw(cmd);
		font.Draw(cmd);
	}
	void Button::OnClick(std::function<void(const EventArgs& args)> func)
	{
		onClick = std::move(func);
	}
	void Button::OnRightClick(std::function<void(const EventArgs& args)> func)
	{
		onRightClick = std::move(func);
	}
	void Button::OnDragStart(std::function<void(const EventArgs& args)> func)
	{
		onDragStart = std::move(func);
	}
	void Button::OnDrag(std::function<void(const EventArgs& args)> func)
	{
		onDrag = std::move(func);
	}
	void Button::OnDragEnd(std::function<void(const EventArgs& args)> func)
	{
		onDragEnd = std::move(func);
	}
	void Button::SetTheme(const Theme& theme, int id)
	{
		Widget::SetTheme(theme, id);
		theme.font.Apply(font_description.params);
	}
}
