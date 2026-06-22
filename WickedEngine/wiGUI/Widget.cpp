/**
 * @file
 * @brief Implementation of @ref wi::gui::Widget, the GUI widget base class.
 */

#include "wiGUI/Widget.h"
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
	Widget::Widget()
	{
		sprites[IDLE].params.color = wi::Color::Booger();
		sprites[FOCUS].params.color = wi::Color::Gray();
		sprites[ACTIVE].params.color = wi::Color::White();
		sprites[DEACTIVATING].params.color = wi::Color::Gray();
		font.params.shadowColor = wi::Color::Shadow();
		font.params.shadow_bolden = 0.2f;
		font.params.shadow_softness = 0.2f;

		tooltipSprite.params.color = wi::Color(255, 234, 165);
		tooltipFont.params.color = wi::Color(25, 25, 25, 255);

		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
			sprites[i].params.enableBackground();
		}

		active_area.pos.x = 0;
		active_area.pos.y = 0;
		active_area.siz.x = std::numeric_limits<float>::max();
		active_area.siz.y = std::numeric_limits<float>::max();
	}
	void Widget::Update(const wi::Canvas& canvas, float dt)
	{
		if (!IsVisible())
		{
			return;
		}

		if (force_disable)
		{
			state = IDLE;
		}

		UpdateTransform();

		if (parent != nullptr)
		{
			this->UpdateTransform_Parented(*parent);
		}

		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		XMStoreFloat3(&translation, T);
		XMStoreFloat3(&scale, S);

		scale = wi::math::Max(scale, XMFLOAT3(0.001f, 0.001f, 0.001f));

		scissorRect.bottom = (int32_t)std::ceil(translation.y + scale.y);
		scissorRect.left = (int32_t)std::floor(translation.x);
		scissorRect.right = (int32_t)std::ceil(translation.x + scale.x);
		scissorRect.top = (int32_t)std::floor(translation.y);

		// default sprite and font placement:
		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			sprites[i].params.pos.x = translation.x;
			sprites[i].params.pos.y = translation.y;
			sprites[i].params.siz.x = scale.x;
			sprites[i].params.siz.y = scale.y;
			sprites[i].params.fade = enabled ? 0.0f : 0.5f;
		}
		font.params.posX = translation.x;
		font.params.posY = translation.y;

		hitBox = Hitbox2D(XMFLOAT2(translation.x, translation.y), XMFLOAT2(scale.x, scale.y));

		if (!force_disable && GetState() != WIDGETSTATE::ACTIVE && !tooltipFont.text.empty() && GetPointerHitbox().intersects(hitBox))
		{
			tooltipTimer++;
		}
		else
		{
			tooltipTimer = 0;
		}

		XMFLOAT2 highlight_pos = GetPointerHighlightPos(canvas);
		for (auto& x : sprites)
		{
			if (x.params.isHighlightEnabled())
			{
				x.params.highlight_pos = highlight_pos;
			}
		}

		sprites[state].Update(dt);
		font.Update(dt);
		angular_highlight_timer += dt;
	}
	void Widget::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		if (angular_highlight_width > 0)
		{
			wi::image::Params fx;
			fx.color = angular_highlight_color;
			fx.pos.x = translation.x - angular_highlight_width;
			fx.pos.y = translation.y - angular_highlight_width;
			fx.siz.x = scale.x + angular_highlight_width * 2;
			fx.siz.y = scale.y + angular_highlight_width * 2;
			if (sprites[state].params.isCornerRoundingEnabled())
			{
				fx.enableCornerRounding();
				fx.corners_rounding[0] = sprites[state].params.corners_rounding[0];
				fx.corners_rounding[1] = sprites[state].params.corners_rounding[1];
				fx.corners_rounding[2] = sprites[state].params.corners_rounding[2];
				fx.corners_rounding[3] = sprites[state].params.corners_rounding[3];
			}
			fx.angular_softness_outer_angle = XM_PI * 0.6f;
			fx.angular_softness_inner_angle = 0;
			XMStoreFloat2(&fx.angular_softness_direction, XMVector2Normalize(XMVectorSet(std::sin(angular_highlight_timer), std::cos(angular_highlight_timer), 0, 0)));
			fx.enableAngularSoftnessDoubleSided();
			fx.border_soften = 0.1f;
			wi::image::Draw(nullptr, fx, cmd);
		}
	}
	void Widget::RenderTooltip(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		if (tooltipTimer > 25)
		{
			float screenwidth = canvas.GetLogicalWidth();
			float screenheight = canvas.GetLogicalHeight();

			tooltipFont.params.position = {};
			tooltipFont.params.h_align = wi::font::WIFALIGN_LEFT;
			tooltipFont.params.v_align = wi::font::WIFALIGN_TOP;

			static const float _border = 2;
			XMFLOAT2 textSize = tooltipFont.TextSize();
			float textWidth = textSize.x;
			float textHeight = textSize.y;
			const float textHeightWithoutScriptTip = textHeight;

			if (!scripttipFont.text.empty())
			{
				XMFLOAT2 scriptTipSize = scripttipFont.TextSize();
				textWidth = std::max(textWidth, scriptTipSize.x);
				textHeight += scriptTipSize.y;
			}

			XMFLOAT2 pointer = GetPointerHitbox().pos;
			tooltipFont.params.posX = pointer.x;
			tooltipFont.params.posY = pointer.y;

			if (tooltipFont.params.posX + textWidth > screenwidth)
			{
				tooltipFont.params.posX -= tooltipFont.params.posX + textWidth - screenwidth;
			}
			if (tooltipFont.params.posX < 0)
			{
				tooltipFont.params.posX = 0;
			}

			if (tooltipFont.params.posY > screenheight * 0.8f)
			{
				tooltipFont.params.posY -= 30;
			}
			else
			{
				tooltipFont.params.posY += 40;
			}

			tooltipSprite.params.pos.x = tooltipFont.params.posX - _border;
			tooltipSprite.params.pos.y = tooltipFont.params.posY - _border;
			tooltipSprite.params.siz.x = textWidth + _border * 2;
			tooltipSprite.params.siz.y = textHeight + _border * 2;

			if (tooltip_shadow > 0)
			{
				wi::image::Params fx(
					tooltipSprite.params.pos.x - tooltip_shadow,
					tooltipSprite.params.pos.y - tooltip_shadow,
					tooltipSprite.params.siz.x + tooltip_shadow * 2,
					tooltipSprite.params.siz.y + tooltip_shadow * 2,
					tooltip_shadow_color
				);
				wi::image::Draw(nullptr, fx, cmd);
			}

			tooltipSprite.Draw(cmd);

			tooltipFont.Draw(cmd);

			if (!scripttipFont.text.empty())
			{
				scripttipFont.params = tooltipFont.params;
				scripttipFont.params.posY += textHeightWithoutScriptTip;
				scripttipFont.params.color = tooltipFont.params.color;
				scripttipFont.params.color.setA(uint8_t(float(scripttipFont.params.color.getA()) / 255.0f * 0.6f * 255));
				scripttipFont.Draw(cmd);
			}
		}
	}
	const std::string& Widget::GetName() const
	{
		return name;
	}
	void Widget::SetName(const std::string& value)
	{
		if (value.length() <= 0)
		{
			static std::atomic<uint32_t> widgetID{ 0 };
			name = "widget_" + std::to_string(widgetID.fetch_add(1));
		}
		else
		{
			name = value;
		}
	}
	std::string Widget::GetText() const
	{
		return font.GetTextA();
	}
	std::string Widget::GetTooltip() const
	{
		return tooltipFont.GetTextA();
	}
	void Widget::SetText(const char* value)
	{
		font.SetText(value);
	}
	void Widget::SetText(const std::string& value)
	{
		font.SetText(value);
	}
	void Widget::SetText(std::string&& value)
	{
		font.SetText(std::move(value));
	}
	void Widget::SetTooltip(const std::string& value)
	{
		tooltipFont.SetText(value);
	}
	void Widget::SetTooltip(std::string&& value)
	{
		tooltipFont.SetText(std::move(value));
	}
	void Widget::SetScriptTip(const std::string& value)
	{
		scripttipFont.SetText(value);
	}
	void Widget::SetScriptTip(std::string&& value)
	{
		scripttipFont.SetText(std::move(value));
	}
	void Widget::SetPos(const XMFLOAT2& value)
	{
		SetDirty();
		translation_local.x = value.x;
		translation_local.y = value.y;
		UpdateTransform();

		translation = translation_local;
	}
	void Widget::SetSize(const XMFLOAT2& value)
	{
		SetDirty();
		scale_local.x = value.x;
		scale_local.y = value.y;
		scale_local = wi::math::Max(scale_local, XMFLOAT3(0.001f, 0.001f, 0.001f));
		UpdateTransform();

		scale = scale_local;
	}
	XMFLOAT2 Widget::GetPos() const
	{
		return *(XMFLOAT2*)&translation;
	}
	XMFLOAT2 Widget::GetSize() const
	{
		return *(XMFLOAT2*)&scale;
	}
	WIDGETSTATE Widget::GetState() const
	{
		return state;
	}
	void Widget::SetEnabled(bool val)
	{
		if (enabled != val)
		{
			enabled = val;
			if (!enabled)
				state = IDLE;
		}
	}
	bool Widget::IsEnabled() const
	{
		return enabled && visible && !force_disable;
	}
	void Widget::SetVisible(bool val)
	{
		if (visible != val)
		{
			visible = val;
			if (!visible)
				state = IDLE;
		}
	}
	bool Widget::IsVisible() const
	{
		if (parent != nullptr && !parent->IsVisible())
		{
			return false;
		}
		return visible;
	}
	void Widget::Activate()
	{
		priority_change = true;
		state = ACTIVE;
		tooltipTimer = 0;
	}
	void Widget::Deactivate()
	{
		state = DEACTIVATING;
		tooltipTimer = 0;
	}
	wi::Color Widget::GetColor() const
	{
		return wi::Color::fromFloat4(sprites[GetState()].params.color);
	}

	enum LOCALIZATION_ID
	{
		LOCALIZATION_ID_TEXT,
		LOCALIZATION_ID_TOOLTIP,
	};
	void Widget::ExportLocalization(wi::Localization& localization) const
	{
		if (!IsLocalizationEnabled())
			return;

		if (font.GetText().empty() && tooltipFont.GetText().empty())
			return;

		wi::Localization& section = localization.GetSection(GetName());
		section.SetSectionHint(GetWidgetTypeName());

		if (has_flag(localization_enabled, LocalizationEnabled::Text) && !font.GetText().empty())
		{
			section.Add(LOCALIZATION_ID_TEXT, font.GetTextA().c_str(), "text");
		}
		if (has_flag(localization_enabled, LocalizationEnabled::Tooltip) && !tooltipFont.GetText().empty())
		{
			section.Add(LOCALIZATION_ID_TOOLTIP, tooltipFont.GetTextA().c_str(), "tooltip");
		}
	}
	void Widget::ImportLocalization(const wi::Localization& localization)
	{
		const wi::Localization* section = localization.CheckSection(GetName());
		if (section == nullptr)
			return;

		if (has_flag(localization_enabled, LocalizationEnabled::Text))
		{
			const char* localized_text = section->Get(LOCALIZATION_ID_TEXT);
			if (localized_text != nullptr)
			{
				SetText(localized_text);
			}
		}
		if (has_flag(localization_enabled, LocalizationEnabled::Tooltip))
		{
			const char* localized_tooltip = section->Get(LOCALIZATION_ID_TOOLTIP);
			if (localized_tooltip != nullptr)
			{
				SetTooltip(localized_tooltip);
			}
		}
	}

	void Widget::SetColor(wi::Color color, int id)
	{
		if (id < 0)
		{
			for (int i = 0; i < arraysize(sprites); ++i)
			{
				sprites[i].params.color = color;
			}
		}
		else if (id < arraysize(sprites))
		{
			sprites[id].params.color = color;
		}
	}
	void Widget::SetShadowColor(wi::Color color)
	{
		shadow_color = color;
	}
	void Widget::SetImage(wi::Resource textureResource, int id)
	{
		if (id < 0)
		{
			for (int i = 0; i < arraysize(sprites); ++i)
			{
				sprites[i].textureResource = textureResource;
			}
		}
		else if (id < arraysize(sprites))
		{
			sprites[id].textureResource = textureResource;
		}
	}
	void Widget::SetTheme(const Theme& theme, int id)
	{
		if (id < 0)
		{
			for (int i = 0; i < arraysize(sprites); ++i)
			{
				theme.image.Apply(sprites[i].params);
			}
		}
		else if (id < arraysize(sprites))
		{
			theme.image.Apply(sprites[id].params);
		}
		theme.font.Apply(font.params);
		if (theme.shadow >= 0)
		{
			SetShadowRadius(theme.shadow);
		}
		SetShadowColor(theme.shadow_color);
		theme.tooltipFont.Apply(tooltipFont.params);
		theme.tooltipImage.Apply(tooltipSprite.params);
		if (theme.tooltip_shadow >= 0)
		{
			tooltip_shadow = theme.tooltip_shadow;
		}
		tooltip_shadow_color = theme.tooltip_shadow_color;
		shadow_highlight = theme.shadow_highlight;
		shadow_highlight_color = theme.shadow_highlight_color;
		shadow_highlight_spread = theme.shadow_highlight_spread;
		for (auto& x : sprites)
		{
			x.params.border_soften = theme.image.border_soften;
		}
	}

	void Widget::AttachTo(Widget* parent)
	{
		this->parent = parent;

		if (parent != nullptr)
		{
			parent->UpdateTransform();
			XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&parent->world));
			MatrixTransform(B);
		}

		UpdateTransform();

		if (parent != nullptr)
		{
			UpdateTransform_Parented(*parent);
		}

		XMVECTOR S, R, T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		XMStoreFloat3(&translation, T);
		XMStoreFloat3(&scale, S);
	}
	void Widget::Detach()
	{
		if (parent != nullptr)
		{
			parent = nullptr;
			ApplyTransform();
		}
	}
	void Widget::ApplyScissor(const wi::Canvas& canvas, const wi::graphics::Rect rect, CommandList cmd, bool constrain_to_parent) const
	{
		wi::graphics::Rect scissor = rect;

		if (constrain_to_parent && parent != nullptr)
		{
			Widget* recurse_parent = parent;
			while (recurse_parent != nullptr)
			{
				scissor.bottom = std::min(scissor.bottom, recurse_parent->scissorRect.bottom);
				scissor.top = std::max(scissor.top, recurse_parent->scissorRect.top);
				scissor.left = std::max(scissor.left, recurse_parent->scissorRect.left);
				scissor.right = std::min(scissor.right, recurse_parent->scissorRect.right);

				recurse_parent = recurse_parent->parent;
			}
		}

		if (scissor.left > scissor.right)
		{
			scissor.left = scissor.right;
		}
		if (scissor.top > scissor.bottom)
		{
			scissor.top = scissor.bottom;
		}

		GraphicsDevice* device = wi::graphics::GetDevice();
		float scale = canvas.GetDPIScaling();
		scissor.bottom = int32_t((float)scissor.bottom * scale);
		scissor.top = int32_t((float)scissor.top * scale);
		scissor.left = int32_t((float)scissor.left * scale);
		scissor.right = int32_t((float)scissor.right * scale);
		device->BindScissorRects(1, &scissor, cmd);
	}
	Hitbox2D Widget::GetPointerHitbox(bool constrained) const
	{
		XMFLOAT4 pointer = wi::input::GetPointer();
		Hitbox2D hb = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));
		if (constrained)
		{
			HitboxConstrain(hb); // this is used to filter out pointer outside of active_area (outside of scissor basically)
		}
		return hb;
	}
	void Widget::HitboxConstrain(wi::primitive::Hitbox2D& hb) const
	{
		float left = hb.pos.x;
		float right = hb.pos.x + hb.siz.x;
		float top = hb.pos.y;
		float bottom = hb.pos.y + hb.siz.y;

		float area_left = active_area.pos.x;
		float area_right = active_area.pos.x + active_area.siz.x;
		float area_top = active_area.pos.y;
		float area_bottom = active_area.pos.y + active_area.siz.y;

		bottom = std::min(bottom, area_bottom);
		top = std::max(top, area_top);
		left = std::max(left, area_left);
		right = std::min(right, area_right);

		hb.pos.x = left;
		hb.pos.y = top;
		hb.siz.x = std::max(0.0f, right - left);
		hb.siz.y = std::max(0.0f, bottom - top);

		if (parent != nullptr)
		{
			parent->HitboxConstrain(hb);
		}
	}
	XMFLOAT2 Widget::GetPointerHighlightPos(const wi::Canvas& canvas) const
	{
		XMFLOAT4 pointer = wi::input::GetPointer();
		return XMFLOAT2(pointer.x / canvas.GetLogicalWidth(), pointer.y / canvas.GetLogicalHeight());
	}
}
