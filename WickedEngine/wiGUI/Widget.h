#pragma once
/**
 * @file
 * Base class for all `wi::gui` widgets.
 *
 * @ref wi::gui::Widget provides the machinery every widget shares: a transform
 * (it derives from `wi::scene::TransformComponent`), per-state background
 * sprites, a text label, tooltip/script-tip rendering, shadow and highlight
 * styling, parent/child attachment, and the Update / Render lifecycle. Concrete
 * widgets (Button, Slider, Window, ...) override the virtual hooks.
 */

#include "wiGUI/GUICommon.h"

namespace wi::gui
{
	class ComboBox;

	/**
	 * Base class for all GUI widgets.
	 *
	 * A widget is a `wi::scene::TransformComponent` with an interaction
	 * @ref WIDGETSTATE, one background sprite per state, a text label and
	 * optional tooltip. Widgets form a hierarchy through @ref parent /
	 * @ref AttachTo, and are driven each frame by @ref Update followed by
	 * @ref Render. Styling is applied via @ref SetColor / @ref SetImage /
	 * @ref SetTheme.
	 *
	 * @note Copy (and implicitly move) is disabled: widgets are referenced by
	 *       pointer (parent/child, manager lists), so they must not be
	 *       relocated.
	 */
	class Widget : public wi::scene::TransformComponent
	{
		/**
		 * Grants ComboBox access to widget internals.
		 *
		 * ComboBox drives the internals of its embedded sub-widgets (e.g. its
		 * filter input field) directly.
		 */
		friend class ComboBox;
	private:
		/**
		 * Frames the pointer has hovered while a tooltip is pending.
		 *
		 * Incremented in @ref Update while hovered; once it exceeds a threshold
		 * the tooltip is drawn by @ref RenderTooltip. Reset to 0 otherwise.
		 */
		int tooltipTimer = 0;
	protected:
		/** Unique widget name / identifier (auto-generated if set empty). */
		std::string name;

		/** Whether the widget accepts interaction. */
		bool enabled = true;

		/** Whether the widget is drawn and interactive. */
		bool visible = true;

		/** Which parts of the widget are localized. */
		LocalizationEnabled localization_enabled = LocalizationEnabled::All;

		/** Shadow radius (0 disables the shadow). */
		float shadow = 1;

		/** Shadow color. */
		wi::Color shadow_color = wi::Color::Shadow();

		/** Whether the shadow uses the pointer highlight effect. */
		bool shadow_highlight = false;

		/** Color of the shadow highlight (RGB). */
		XMFLOAT3 shadow_highlight_color = XMFLOAT3(1, 1, 1);

		/** Spread/falloff of the shadow highlight. */
		float shadow_highlight_spread = 1;

		/** Current interaction state. */
		WIDGETSTATE state = IDLE;

		/** Tooltip shadow radius. */
		float tooltip_shadow = 1;

		/** Tooltip shadow color. */
		wi::Color tooltip_shadow_color = wi::Color::Shadow();

		/**
		 * Background sprite drawn behind the tooltip text (built during
		 * render).
		 */
		mutable wi::Sprite tooltipSprite;

		/** Tooltip text (built during render). */
		mutable wi::SpriteFont tooltipFont;

		/**
		 * Secondary "script tip" text drawn under the tooltip (built during
		 * render).
		 */
		mutable wi::SpriteFont scripttipFont;

		/** Width of the animated angular highlight border (0 disables it). */
		float angular_highlight_width = 0;

		/** Animation timer driving the angular highlight sweep. */
		float angular_highlight_timer = 0;

		/** Color of the angular highlight (RGBA). */
		XMFLOAT4 angular_highlight_color = XMFLOAT4(1, 1, 1, 1);

		/**
		 * Measured width of text rendered to the left of the widget (layout).
		 */
		float left_text_width = 0;

		/**
		 * Measured width of text rendered to the right of the widget (layout).
		 */
		float right_text_width = 0;

	public:
		/**
		 * Constructs a widget with default per-state colors and styling.
		 */
		Widget();

		/**
		 * Virtual destructor for safe polymorphic deletion.
		 */
		virtual ~Widget() = default;

		/**
		 * Copy is deleted to keep internal references stable.
		 *
		 * Widgets are tracked by pointer (parent/child links, manager lists),
		 * so they must not be copied or relocated. This also suppresses the
		 * implicit move operations.
		 */
		Widget(const Widget &) = delete;

		/**
		 * Returns the widget's name.
		 */
		const std::string& GetName() const;

		/**
		 * Sets the widget's name.
		 *
		 * @param[in] value - New name; if empty, a unique name is generated.
		 */
		void SetName(const std::string& value);

		/**
		 * Returns the main text as an ASCII string.
		 */
		std::string GetText() const;

		/**
		 * Returns the tooltip text as an ASCII string.
		 */
		std::string GetTooltip() const;

		/**
		 * Sets the main text.
		 *
		 * @param[in] value - Null-terminated text.
		 */
		void SetText(const char* value);

		/**
		 * Sets the main text.
		 *
		 * @param[in] value - Text to copy.
		 */
		void SetText(const std::string& value);

		/**
		 * Sets the main text.
		 *
		 * @param[in] value - Text to move from.
		 */
		void SetText(std::string&& value);

		/**
		 * Sets the tooltip text.
		 *
		 * @param[in] value - Text to copy.
		 */
		void SetTooltip(const std::string& value);

		/**
		 * Sets the tooltip text.
		 *
		 * @param[in] value - Text to move from.
		 */
		void SetTooltip(std::string&& value);

		/**
		 * Sets the script-tip text.
		 *
		 * @param[in] value - Text to copy.
		 */
		void SetScriptTip(const std::string& value);

		/**
		 * Sets the script-tip text.
		 *
		 * @param[in] value - Text to move from.
		 */
		void SetScriptTip(std::string&& value);

		/**
		 * Sets the widget's local position.
		 *
		 * @param[in] value - New local (x, y) position.
		 */
		void SetPos(const XMFLOAT2& value);

		/**
		 * Sets the widget's local size.
		 *
		 * @param[in] value - New (width, height); clamped to a small minimum.
		 */
		void SetSize(const XMFLOAT2& value);

		/**
		 * Returns the current (x, y) position.
		 */
		XMFLOAT2 GetPos() const;

		/**
		 * Returns the current (width, height).
		 */
		virtual XMFLOAT2 GetSize() const;

		/**
		 * Returns the current interaction state.
		 */
		virtual WIDGETSTATE GetState() const;

		/**
		 * Enables or disables interaction.
		 *
		 * @param[in] val - true to enable; disabling forces the state to IDLE.
		 */
		virtual void SetEnabled(bool val);

		/**
		 * Returns whether the widget is currently interactive.
		 */
		bool IsEnabled() const;

		/**
		 * Shows or hides the widget.
		 *
		 * @param[in] val - true to show; hiding forces the state to IDLE.
		 */
		virtual void SetVisible(bool val);

		/**
		 * Returns whether the widget is visible (false if a parent is hidden).
		 */
		bool IsVisible() const;

		/**
		 * Returns the color of the current state's sprite.
		 */
		wi::Color GetColor() const;

		/**
		 * Returns the shadow radius.
		 */
		float GetShadowRadius() const { return shadow; }

		/**
		 * Sets the shadow radius.
		 *
		 * @param[in] value - New radius.
		 */
		void SetShadowRadius(float value) { shadow = value; }

		/**
		 * Returns whether the shadow highlight effect is enabled.
		 */
		bool IsShadowHighlightEnabled() { return shadow_highlight; }

		/**
		 * Enables or disables the shadow highlight effect.
		 *
		 * @param[in] value - true to enable the effect.
		 */
		void SetShadowHighlightEnabled(bool value) {
			shadow_highlight = value;
		}

		/**
		 * Returns the shadow highlight color (RGB).
		 */
		XMFLOAT3 GetShadowHighlightColor() const {
			return shadow_highlight_color;
		}

		/**
		 * Sets the shadow highlight color (RGB).
		 *
		 * @param[in] value - New highlight color.
		 */
		void SetShadowHighlightColor(const XMFLOAT3& value) {
			shadow_highlight_color = value;
		}

		/**
		 * Returns the shadow highlight spread/falloff.
		 */
		float GetShadowHighlightSpread() const {
			return shadow_highlight_spread;
		}

		/**
		 * Sets the shadow highlight spread/falloff.
		 *
		 * @param[in] value - New spread value.
		 */
		void SetShadowHighlightSpread(float value) {
			shadow_highlight_spread = value;
		}

		/**
		 * Recomputes child layout; no-op for the base widget.
		 */
		virtual void ResizeLayout() {};

		/**
		 * Per-frame update: transform, hitbox, sprite/font placement, tooltip
		 * timer and highlight animation.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		virtual void Update(const wi::Canvas& canvas, float dt);

		/**
		 * Draws the widget. The base draws the angular highlight effect.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		virtual void Render(
			const wi::Canvas& canvas,
			wi::graphics::CommandList cmd
		) const;

		/**
		 * Draws the tooltip if the widget has been hovered long enough.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		virtual void RenderTooltip(
			const wi::Canvas& canvas,
			wi::graphics::CommandList cmd
		) const;

		/**
		 * Sets a sprite color.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID (or custom ID); -1 sets all
		 *                 states. Subclasses may handle extended IDs.
		 */
		virtual void SetColor(wi::Color color, int id = -1);

		/**
		 * Sets the shadow color.
		 *
		 * @param[in] color - New shadow color.
		 */
		virtual void SetShadowColor(wi::Color color);

		/**
		 * Sets a sprite texture.
		 *
		 * @param[in] textureResource - Texture resource to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 sets all states.
		 */
		virtual void SetImage(wi::Resource textureResource, int id = -1);

		/**
		 * Applies a theme to the widget.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		virtual void SetTheme(const Theme& theme, int id = -1);

		/**
		 * Returns the widget's type name (for debugging / localization).
		 */
		virtual const char* GetWidgetTypeName() const { return "Widget"; }

		/** Background sprite for each @ref WIDGETSTATE. */
		wi::Sprite sprites[WIDGETSTATE_COUNT];

		/** Main text label. */
		wi::SpriteFont font;

		/**
		 * Resolved world position, decomposed from the transform each
		 * @ref Update.
		 */
		XMFLOAT3 translation = XMFLOAT3(0, 0, 0);

		/**
		 * Resolved world scale (size), decomposed from the transform each
		 * @ref Update.
		 */
		XMFLOAT3 scale = XMFLOAT3(1, 1, 1);

		/** Interaction rectangle tested against the pointer. */
		wi::primitive::Hitbox2D hitBox;

		/** Clipping rectangle for this widget's draws. */
		wi::graphics::Rect scissorRect;

		/** Parent widget in the hierarchy, or nullptr if unparented. */
		Widget* parent = nullptr;

		/**
		 * Attaches this widget to a parent, preserving world transform.
		 *
		 * @param[in] parent - New parent widget (may be nullptr).
		 */
		void AttachTo(Widget* parent);

		/**
		 * Detaches from the current parent, baking in the world transform.
		 */
		void Detach();

		/**
		 * Enters the ACTIVE state and raises interaction priority.
		 */
		virtual void Activate();

		/**
		 * Enters the DEACTIVATING state (interaction ending).
		 */
		virtual void Deactivate();

		/**
		 * Binds a scissor rectangle for subsequent draws.
		 *
		 * @param[in] canvas - Canvas providing DPI scaling.
		 * @param[in] rect - Rectangle to bind (logical coordinates).
		 * @param[in] cmd - Command list to record into.
		 * @param[in] constrain_to_parent - If true, intersect with all ancestor
		 *                                  scissor rects.
		 */
		void ApplyScissor(
			const wi::Canvas& canvas,
			const wi::graphics::Rect rect,
			wi::graphics::CommandList cmd,
			bool constrain_to_parent = true
		) const;

		/**
		 * Returns the pointer's hitbox.
		 *
		 * @param[in] constrained - If true, clip to @ref active_area and
		 *                          ancestors.
		 *
		 * @return A 1x1 hitbox at the pointer position (possibly clipped).
		 */
		wi::primitive::Hitbox2D GetPointerHitbox(bool constrained = true) const;

		/**
		 * Returns the pointer position normalized to [0, 1] over the canvas.
		 *
		 * @param[in] canvas - Canvas providing logical size.
		 *
		 * @return Normalized (x, y) used to drive highlight shaders.
		 */
		XMFLOAT2 GetPointerHighlightPos(const wi::Canvas& canvas) const;

		/**
		 * Area the pointer hitbox is constrained to (scissor-like clip
		 * region).
		 */
		wi::primitive::Hitbox2D active_area;

		/**
		 * Clips a hitbox to @ref active_area, recursing into ancestors.
		 *
		 * @param[in,out] hb - Hitbox to constrain in place.
		 */
		void HitboxConstrain(wi::primitive::Hitbox2D& hb) const;

		/** Set when this widget's interaction priority changed this frame. */
		bool priority_change = true;

		/** Sort priority among sibling widgets (lower draws/updates first). */
		uint32_t priority = 0;

		/** When true, the widget is forced disabled for the current frame. */
		bool force_disable = false;

		/**
		 * Returns whether any localization is enabled for this widget.
		 */
		bool IsLocalizationEnabled() const {
			return localization_enabled != LocalizationEnabled::None;
		}

		/**
		 * Returns the localization flags.
		 */
		LocalizationEnabled GetLocalizationEnabled() const {
			return localization_enabled;
		}

		/**
		 * Sets the localization flags.
		 *
		 * @param[in] value - New localization flags.
		 */
		void SetLocalizationEnabled(LocalizationEnabled value) {
			localization_enabled = value;
		}

		/**
		 * Enables all or no localization.
		 *
		 * @param[in] value - true selects All, false selects None.
		 */
		void SetLocalizationEnabled(bool value) {
			localization_enabled =
				value ? LocalizationEnabled::All : LocalizationEnabled::None;
		}

		/**
		 * Writes this widget's localizable strings into a localization.
		 *
		 * @param[in,out] localization - Localization to add entries to.
		 */
		virtual void ExportLocalization(wi::Localization& localization) const;

		/**
		 * Applies localized strings from a localization to this widget.
		 *
		 * @param[in] localization - Localization to read entries from.
		 */
		virtual void ImportLocalization(const wi::Localization& localization);

		/**
		 * Sets the angular highlight border width (0 disables it).
		 *
		 * @param[in] value - New border width.
		 */
		void SetAngularHighlightWidth(float value) {
			angular_highlight_width = value;
		};

		/**
		 * Returns the angular highlight border width.
		 */
		float GetAngularHighlightWidth() const {
			return angular_highlight_width;
		};

		/**
		 * Sets the angular highlight color (RGBA).
		 *
		 * @param[in] value - New highlight color.
		 */
		void SetAngularHighlightColor(const XMFLOAT4& value) {
			angular_highlight_color = value;
		};

		/**
		 * Returns the angular highlight color (RGBA).
		 */
		XMFLOAT4 GetAngularHighlightColor() const {
			return angular_highlight_color;
		};

		/**
		 * Returns the measured width of text rendered to the widget's left.
		 */
		constexpr float GetLeftTextWidth() const { return left_text_width; }

		/**
		 * Returns the measured width of text rendered to the widget's right.
		 */
		constexpr float GetRightTextWidth() const { return right_text_width; }
	};
}
