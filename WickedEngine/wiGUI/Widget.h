#pragma once
#include "wiGUI/GUICommon.h"

namespace wi::gui
{
	class ComboBox;

	class Widget : public wi::scene::TransformComponent
	{
		friend class ComboBox;
	private:
		int tooltipTimer = 0;
	protected:
		std::string name;
		bool enabled = true;
		bool visible = true;
		LocalizationEnabled localization_enabled = LocalizationEnabled::All;
		float shadow = 1; // shadow radius
		wi::Color shadow_color = wi::Color::Shadow();
		bool shadow_highlight = false;
		XMFLOAT3 shadow_highlight_color = XMFLOAT3(1, 1, 1);
		float shadow_highlight_spread = 1;
		WIDGETSTATE state = IDLE;
		float tooltip_shadow = 1; // shadow radius
		wi::Color tooltip_shadow_color = wi::Color::Shadow();
		mutable wi::Sprite tooltipSprite;
		mutable wi::SpriteFont tooltipFont;
		mutable wi::SpriteFont scripttipFont;
		float angular_highlight_width = 0;
		float angular_highlight_timer = 0;
		XMFLOAT4 angular_highlight_color = XMFLOAT4(1, 1, 1, 1);
		float left_text_width = 0;
		float right_text_width = 0;

	public:
		Widget();
		virtual ~Widget() = default;

		// Delete copy/move to keep internal references stable.
		Widget(const Widget &) = delete;

		const std::string& GetName() const;
		void SetName(const std::string& value);
		std::string GetText() const;
		std::string GetTooltip() const;
		void SetText(const char* value);
		void SetText(const std::string& value);
		void SetText(std::string&& value);
		void SetTooltip(const std::string& value);
		void SetTooltip(std::string&& value);
		void SetScriptTip(const std::string& value);
		void SetScriptTip(std::string&& value);
		void SetPos(const XMFLOAT2& value);
		void SetSize(const XMFLOAT2& value);
		XMFLOAT2 GetPos() const;
		virtual XMFLOAT2 GetSize() const;
		virtual WIDGETSTATE GetState() const;
		virtual void SetEnabled(bool val);
		bool IsEnabled() const;
		virtual void SetVisible(bool val);
		bool IsVisible() const;
		wi::Color GetColor() const;
		float GetShadowRadius() const { return shadow; }
		void SetShadowRadius(float value) { shadow = value; }

		bool IsShadowHighlightEnabled() { return shadow_highlight; }
		void SetShadowHighlightEnabled(bool value) { shadow_highlight = value; }
		XMFLOAT3 GetShadowHighlightColor() const { return shadow_highlight_color; }
		void SetShadowHighlightColor(const XMFLOAT3& value) { shadow_highlight_color = value; }
		float GetShadowHighlightSpread() const { return shadow_highlight_spread; }
		void SetShadowHighlightSpread(float value) { shadow_highlight_spread = value; }

		virtual void ResizeLayout() {};
		virtual void Update(const wi::Canvas& canvas, float dt);
		virtual void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;
		virtual void RenderTooltip(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

		// last param default: set color for all states
		//	you can specify a WIDGET_ID here, or your own custom ID if you use your own widget type
		virtual void SetColor(wi::Color color, int id = -1);
		virtual void SetShadowColor(wi::Color color);
		virtual void SetImage(wi::Resource textureResource, int id = -1);
		virtual void SetTheme(const Theme& theme, int id = -1);
		virtual const char* GetWidgetTypeName() const { return "Widget"; }

		wi::Sprite sprites[WIDGETSTATE_COUNT];
		wi::SpriteFont font;

		XMFLOAT3 translation = XMFLOAT3(0, 0, 0);
		XMFLOAT3 scale = XMFLOAT3(1, 1, 1);

		wi::primitive::Hitbox2D hitBox;
		wi::graphics::Rect scissorRect;

		Widget* parent = nullptr;
		void AttachTo(Widget* parent);
		void Detach();

		virtual void Activate();
		virtual void Deactivate();

		void ApplyScissor(const wi::Canvas& canvas, const wi::graphics::Rect rect, wi::graphics::CommandList cmd, bool constrain_to_parent = true) const;
		wi::primitive::Hitbox2D GetPointerHitbox(bool constrained = true) const;
		XMFLOAT2 GetPointerHighlightPos(const wi::Canvas& canvas) const;

		wi::primitive::Hitbox2D active_area; // Pointer hitbox constrain area
		void HitboxConstrain(wi::primitive::Hitbox2D& hb) const;

		bool priority_change = true;
		uint32_t priority = 0;
		bool force_disable = false;

		bool IsLocalizationEnabled() const { return localization_enabled != LocalizationEnabled::None; }
		LocalizationEnabled GetLocalizationEnabled() const { return localization_enabled; }
		void SetLocalizationEnabled(LocalizationEnabled value) { localization_enabled = value; }
		void SetLocalizationEnabled(bool value) { localization_enabled = value ? LocalizationEnabled::All : LocalizationEnabled::None; }
		virtual void ExportLocalization(wi::Localization& localization) const;
		virtual void ImportLocalization(const wi::Localization& localization);

		void SetAngularHighlightWidth(float value) { angular_highlight_width = value; };
		float GetAngularHighlightWidth() const { return angular_highlight_width; };
		void SetAngularHighlightColor(const XMFLOAT4& value) { angular_highlight_color = value; };
		XMFLOAT4 GetAngularHighlightColor() const { return angular_highlight_color; };

		constexpr float GetLeftTextWidth() const { return left_text_width; }
		constexpr float GetRightTextWidth() const { return right_text_width; }
	};
}
