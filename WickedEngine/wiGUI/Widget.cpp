/**
 * @file
 * Implementation of @ref wi::gui::Widget, the GUI widget base class.
 */

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <CommonInclude.h>
#include <Utility/DirectXMath/DirectXMath.h>

#include <wiCanvas.h>
#include <wiColor.h>
#include <wiGraphicsDevice.h>
#include <wiGUI/GUICommon.h>
#include <wiLocalization.h>
#include <wiMath.h>
#include <wiResourceManager.h>

#include "wiFont.h"
#include "wiImage.h"
#include "wiInput.h"
#include "wiPrimitive.h"

#include "wiGUI/Widget.h"

using namespace wi::graphics;
using namespace wi::primitive;

namespace wi::gui
{
	Widget::Widget()
	{
		sprites[IDLE].params.color 		   = wi::Color::Booger();
		sprites[FOCUS].params.color 	   = wi::Color::Gray();
		sprites[ACTIVE].params.color 	   = wi::Color::White();
		sprites[DEACTIVATING].params.color = wi::Color::Gray();

		font.params.shadowColor 	= wi::Color::Shadow();
		font.params.shadow_bolden   = 0.2f;
		font.params.shadow_softness = 0.2f;

		tooltipSprite.params.color = wi::Color(255, 234, 165);
		tooltipFont.params.color   = wi::Color( 25,  25,  25, 255);

		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			auto& sprite = sprites[i];

			sprite.params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
			sprite.params.enableBackground();
		}

		active_area.pos.x = 0;
		active_area.pos.y = 0;
		active_area.siz.x = std::numeric_limits<float>::max();
		active_area.siz.y = std::numeric_limits<float>::max();
	}

	void Widget::Update(const wi::Canvas& canvas, const float dt)
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

		XMVECTOR S;
		XMVECTOR R;
		XMVECTOR T;
		XMMatrixDecompose(&S, &R, &T, XMLoadFloat4x4(&world));
		XMStoreFloat3(&translation, T);
		XMStoreFloat3(&scale, S);

		scale = wi::math::Max(scale, XMFLOAT3(0.001f, 0.001f, 0.001f));

		scissorRect.bottom = static_cast<int32_t>(std::ceil(translation.y + scale.y));
		scissorRect.left   = static_cast<int32_t>(std::floor(translation.x));
		scissorRect.right  = static_cast<int32_t>(std::ceil(translation.x + scale.x));
		scissorRect.top    = static_cast<int32_t>(std::floor(translation.y));

		// default sprite and font placement:
		for (int i = IDLE; i < WIDGETSTATE_COUNT; ++i)
		{
			auto& sprite = sprites[i];

			sprite.params.pos.x = translation.x;
			sprite.params.pos.y = translation.y;
			sprite.params.siz.x = scale.x;
			sprite.params.siz.y = scale.y;
			sprite.params.fade  = enabled ? 0.0f : 0.5f;
		}

		font.params.posX = translation.x;
		font.params.posY = translation.y;

		hitBox = Hitbox2D(
			XMFLOAT2(translation.x, translation.y),
			XMFLOAT2(scale.x      , scale.y      )
		);

		if (!force_disable
			&& GetState() != WIDGETSTATE::ACTIVE
			&& !tooltipFont.text.empty()
			&& GetPointerHitbox().intersects(hitBox)
		) {
			tooltipTimer++;
		}
		else
		{
			tooltipTimer = 0;
		}

		const XMFLOAT2 highlight_pos = GetPointerHighlightPos(canvas);
		for (auto& sprite : sprites)
		{
			if (sprite.params.isHighlightEnabled())
			{
				sprite.params.highlight_pos = highlight_pos;
			}
		}

		sprites[state].Update(dt);
		font.Update(dt);
		angular_highlight_timer += dt;
	}

	void Widget::Render(
		const wi::Canvas& canvas,
		const wi::graphics::CommandList cmd
	) const
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
				
				for (int i = 0; i < 4; ++i)
				{
					fx.corners_rounding[i]
						= sprites[state].params.corners_rounding[i];
				}
			}

			fx.angular_softness_outer_angle = XM_PI * 0.6f;
			fx.angular_softness_inner_angle = 0;

			XMStoreFloat2(
				&fx.angular_softness_direction,
				XMVector2Normalize(
					XMVectorSet(
						std::sin(angular_highlight_timer),
						std::cos(angular_highlight_timer),
						0,
						0
					)
				)
			);

			fx.enableAngularSoftnessDoubleSided();
			fx.border_soften = 0.1f;
			wi::image::Draw(nullptr, fx, cmd);
		}
	}

	void Widget::RenderTooltip(
		const wi::Canvas& canvas,
		const CommandList cmd
	) const
	{
		if (!IsVisible())
		{
			return;
		}

		if (tooltipTimer > 25)
		{
			const float screenWidth  = canvas.GetLogicalWidth();
			const float screenHeight = canvas.GetLogicalHeight();

			tooltipFont.params.position = {};
			tooltipFont.params.h_align  = wi::font::WIFALIGN_LEFT;
			tooltipFont.params.v_align  = wi::font::WIFALIGN_TOP;

			static const float _border = 2;
			const XMFLOAT2 textSize = tooltipFont.TextSize();

			float textWidth  = textSize.x;
			float textHeight = textSize.y;

			const float textHeightWithoutScriptTip = textHeight;

			if (!scripttipFont.text.empty())
			{
				const XMFLOAT2 scriptTipSize = scripttipFont.TextSize();

				textWidth = std::max(textWidth, scriptTipSize.x);
				textHeight += scriptTipSize.y;
			}

			const XMFLOAT2 pointer = GetPointerHitbox().pos;

			tooltipFont.params.posX = pointer.x;
			tooltipFont.params.posY = pointer.y;

			if (tooltipFont.params.posX + textWidth > screenWidth)
			{
				tooltipFont.params.posX -= tooltipFont.params.posX
										+ textWidth - screenWidth;
			}

			tooltipFont.params.posX = std::max<float>(tooltipFont.params.posX, 0);

			tooltipFont.params.posY +=
    			(tooltipFont.params.posY > screenHeight * 0.8f) ? -30 : 40;

			tooltipSprite.params.pos.x = tooltipFont.params.posX - _border;
			tooltipSprite.params.pos.y = tooltipFont.params.posY - _border;
			tooltipSprite.params.siz.x = textWidth  + _border * 2;
			tooltipSprite.params.siz.y = textHeight + _border * 2;

			if (tooltip_shadow > 0)
			{
				const wi::image::Params fx(
					tooltipSprite.params.pos.x - tooltip_shadow,
					tooltipSprite.params.pos.y - tooltip_shadow,
					tooltipSprite.params.siz.x  + tooltip_shadow * 2,
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
				scripttipFont.params.color.setA(static_cast<uint8_t>(static_cast<float>(
					scripttipFont.params.color.getA()) / 255.0f * 0.6f * 255
				));
				scripttipFont.Draw(cmd);
			}
		}
	}

	const std::string& Widget::GetName() const noexcept
	{
		return name;
	}

	void Widget::SetName(const std::string& value)
	{
		if (value.empty())
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

	void Widget::SetText(const char* const value)
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
		scale_local = wi::math::Max(
			scale_local,
			XMFLOAT3(0.001f, 0.001f, 0.001f)
		);
		UpdateTransform();

		scale = scale_local;
	}

	XMFLOAT2 Widget::GetPos() const noexcept
	{
		return {translation.x, translation.y};
	}

	XMFLOAT2 Widget::GetSize() const
	{
		return {scale.x, scale.y};
	}

	WIDGETSTATE Widget::GetState() const
	{
		return state;
	}

	void Widget::SetEnabled(const bool val)
	{
		if (enabled == val) {
			return;
		}

		enabled = val;

		if (!enabled) {
			state = IDLE;
		}
	}

	bool Widget::IsEnabled() const noexcept
	{
		return enabled && visible && !force_disable;
	}

	void Widget::SetVisible(const bool val)
	{
		if (visible == val) {
			return;
		}

		visible = val;

		if (!visible) {
			state = IDLE;
		}
	}

	bool Widget::IsVisible() const noexcept
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

	wi::Color Widget::GetColor() const noexcept
	{
		return wi::Color::fromFloat4(sprites[GetState()].params.color);
	}

	/**
	 * Identifies a widget's localizable strings within a localization.
	 *
	 * Used as the entry index by @ref Widget::ExportLocalization /
	 * @ref Widget::ImportLocalization.
	 */
	enum LOCALIZATION_ID
	{
		/** The widget's main text. */
		LOCALIZATION_ID_TEXT,

		/** The widget's tooltip text. */
		LOCALIZATION_ID_TOOLTIP,
	};

	void Widget::ExportLocalization(wi::Localization& localization) const
	{
		if (!IsLocalizationEnabled()) {
			return;
		}

		if (font.GetText().empty() && tooltipFont.GetText().empty()) {
			return;
		}

		wi::Localization& section = localization.GetSection(GetName());
		section.SetSectionHint(GetWidgetTypeName());

		if (has_flag(localization_enabled, LocalizationEnabled::Text)
			&& !font.GetText().empty()
		) {
			section.Add(LOCALIZATION_ID_TEXT, font.GetTextA().c_str(), "text");
		}

		if (has_flag(localization_enabled, LocalizationEnabled::Tooltip)
			&& !tooltipFont.GetText().empty()
		) {
			section.Add(LOCALIZATION_ID_TOOLTIP, tooltipFont.GetTextA().c_str(), "tooltip");
		}
	}

	void Widget::ImportLocalization(const wi::Localization& localization)
	{
		const wi::Localization* section = localization.CheckSection(GetName());

		if (section == nullptr) {
			return;
		}

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

	void Widget::SetColor(const wi::Color color, const int id)
	{
		if (id < 0)
		{
			for (auto & sprite : sprites)
			{
				sprite.params.color = color;
			}
		}
		else if (id < arraysize(sprites))
		{
			sprites[id].params.color = color;
		}
	}

	void Widget::SetShadowColor(const wi::Color color)
	{
		shadow_color = color;
	}

	void Widget::SetImage(wi::Resource textureResource, const int id)
	{
		if (id < 0)
		{
			for (auto & sprite : sprites)
			{
				sprite.textureResource = textureResource;
			}
		}
		else if (id < arraysize(sprites))
		{
			sprites[id].textureResource = textureResource;
		}
	}

	void Widget::SetTheme(const Theme& theme, const int id)
	{
		if (id < 0)
		{
			for (auto & sprite : sprites)
			{
				theme.image.Apply(sprite.params);
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

		tooltip_shadow_color    = theme.tooltip_shadow_color;
		shadow_highlight        = theme.shadow_highlight;
		shadow_highlight_color  = theme.shadow_highlight_color;
		shadow_highlight_spread = theme.shadow_highlight_spread;

		for (auto& sprite : sprites)
		{
			sprite.params.border_soften = theme.image.border_soften;
		}
	}

	void Widget::AttachTo(Widget* const parent)
	{
		this->parent = parent;

		if (parent != nullptr)
		{
			parent->UpdateTransform();
			const XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&parent->world));
			MatrixTransform(B);
		}

		UpdateTransform();

		if (parent != nullptr)
		{
			UpdateTransform_Parented(*parent);
		}

		XMVECTOR S;
		XMVECTOR R;
		XMVECTOR T;
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

	void Widget::ApplyScissor(
		const wi::Canvas& canvas,
		const wi::graphics::Rect rect,
		const CommandList cmd,
		const bool constrain_to_parent
	) const
	{
		wi::graphics::Rect scissor = rect;

		if (constrain_to_parent && parent != nullptr)
		{
			const Widget* recurse_parent = parent;

			while (recurse_parent != nullptr)
			{
				scissor.bottom = std::min(scissor.bottom, recurse_parent->scissorRect.bottom);
				scissor.top    = std::max(scissor.top,    recurse_parent->scissorRect.top);
				scissor.left   = std::max(scissor.left,   recurse_parent->scissorRect.left);
				scissor.right  = std::min(scissor.right,  recurse_parent->scissorRect.right);

				recurse_parent = recurse_parent->parent;
			}
		}

		scissor.left = std::min(scissor.left, scissor.right);
		scissor.top  = std::min(scissor.top,  scissor.bottom);

		GraphicsDevice* device = wi::graphics::GetDevice();
		const float scale = canvas.GetDPIScaling();

		scissor.bottom = static_cast<int32_t>(static_cast<float>(scissor.bottom) * scale);
		scissor.top    = static_cast<int32_t>(static_cast<float>(scissor.top)    * scale);
		scissor.left   = static_cast<int32_t>(static_cast<float>(scissor.left)   * scale);
		scissor.right  = static_cast<int32_t>(static_cast<float>(scissor.right)  * scale);

		device->BindScissorRects(1, &scissor, cmd);
	}

	Hitbox2D Widget::GetPointerHitbox(const bool constrained) const noexcept
	{
		const XMFLOAT4 pointer = wi::input::GetPointer();
		Hitbox2D hb = Hitbox2D(XMFLOAT2(pointer.x, pointer.y), XMFLOAT2(1, 1));

		if (constrained)
		{
			// This is used to filter out pointer outside of active_area
			// (outside of scissor basically).
			HitboxConstrain(hb);
		}

		return hb;
	}

	void Widget::HitboxConstrain(wi::primitive::Hitbox2D& hb) const
	{
		float left   = hb.pos.x;
		float right  = hb.pos.x + hb.siz.x;
		float top    = hb.pos.y;
		float bottom = hb.pos.y + hb.siz.y;

		const float area_left   = active_area.pos.x;
		const float area_right  = active_area.pos.x + active_area.siz.x;
		const float area_top    = active_area.pos.y;
		const float area_bottom = active_area.pos.y + active_area.siz.y;

		bottom = std::min(bottom, area_bottom);
		top    = std::max(top,    area_top);
		left   = std::max(left,   area_left);
		right  = std::min(right,  area_right);

		hb.pos.x = left;
		hb.pos.y = top;
		hb.siz.x = std::max(0.0f, right - left);
		hb.siz.y = std::max(0.0f, bottom - top);

		if (parent != nullptr)
		{
			parent->HitboxConstrain(hb);
		}
	}

	XMFLOAT2 Widget::GetPointerHighlightPos(
		const wi::Canvas& canvas
	) const noexcept
	{
		const XMFLOAT4 pointer = wi::input::GetPointer();

		return {
			pointer.x / canvas.GetLogicalWidth(),
			pointer.y / canvas.GetLogicalHeight()
		};
	}
}
