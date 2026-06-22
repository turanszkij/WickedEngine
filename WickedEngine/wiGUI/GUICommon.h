#pragma once

/**
 * @file
   * Shared, dependency-light types for the `wi::gui` module.
 *
 * These are the small building blocks used across every GUI widget:
 * - @ref wi::gui::EventArgs — the generic payload passed to event callbacks.
 * - @ref wi::gui::WIDGETSTATE — the per-widget interaction state machine.
 * - @ref wi::gui::WIDGET_ID — addresses a specific control/state for styling.
 * - @ref wi::gui::LocalizationEnabled — per-widget localization opt-in flags.
 * - @ref wi::gui::Theme — a reusable styling descriptor for widgets.
 */

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiPrimitive.h"
#include "wiCanvas.h"
#include "wiVector.h"
#include "wiColor.h"
#include "wiScene.h"
#include "wiSprite.h"
#include "wiSpriteFont.h"
#include "wiLocalization.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <functional>

namespace wi::gui
{
	/**
	 * @brief Generic payload passed to widget event callbacks.
	 *
	 * One structure is reused for every widget event (click, drag, slide,
	 * selection, color pick, ...). Only the fields meaningful to a given event
	 * are populated; the rest keep their defaults. See each widget's `On*`
	 * methods for which fields they fill in.
	 */
	struct EventArgs
	{
		/** Pointer coordinate at the click. */
		XMFLOAT2 clickPos = {};

		/** Pointer position where a drag began. */
		XMFLOAT2 startPos = {};

		/** Pointer movement since the last update (drag). */
		XMFLOAT2 deltaPos = {};

		/** Pointer position where a drag ended. */
		XMFLOAT2 endPos = {};

		/** Generic float result (e.g. slider value). */
		float fValue = 0;

		/** Generic boolean result (e.g. checkbox state). */
		bool bValue = false;

		/** Generic integer result (e.g. selected index). */
		int iValue = 0;

		/** Secondary integer result (e.g. reorder target). */
		int iValue2 = 0;

		/** Color result of a color-picker operation. */
		wi::Color color;

		/** Generic string result (e.g. text / item name). */
		std::string sValue;

		/** User data set on the widget (or part of one). */
		uint64_t userdata = 0;
	};

	/**
	 * @brief Interaction state of a widget.
	 *
	 * Selects which sprite is drawn and how input is interpreted. A typical
	 * press/release cycle moves the widget IDLE -> FOCUS -> ACTIVE ->
	 * DEACTIVATING -> IDLE.
	 */
	enum WIDGETSTATE
	{
		/** Widget is doing nothing. */
		IDLE,

		/** Pointer is dragged onto / hovering the widget. */
		FOCUS,

		/** Widget is being interacted with right now. */
		ACTIVE,

		/** Was active; interaction is ending. */
		DEACTIVATING,

		/** Number of states (used for array sizing). */
		WIDGETSTATE_COUNT,
	};

	/**
	 * @brief Identifies a specific widget control and state to style.
	 *
	 * Passed to the various `SetColor()` / `SetImage()` / `SetTheme()`
	 * overloads to target one sub-element of a compound widget (e.g. a slider's
	 * knob vs. its base) in a particular @ref WIDGETSTATE. The `*_BEGIN` /
	 * `*_END` sentinels bound each widget's range and must not be used as
	 * values.
	 *
	 * @note Custom widget types may define their own IDs starting at
	 *       @ref WIDGET_ID_USER and handle them in a `SetColor()` override.
	 */
	enum WIDGET_ID
	{
		// IDs for normal widget states:
		WIDGET_ID_IDLE = IDLE,
		WIDGET_ID_FOCUS = FOCUS,
		WIDGET_ID_ACTIVE = ACTIVE,
		WIDGET_ID_DEACTIVATING = DEACTIVATING,

		// IDs for special widget states:

		// TextInputField:
		WIDGET_ID_TEXTINPUTFIELD_BEGIN, // do not use!
		WIDGET_ID_TEXTINPUTFIELD_IDLE,
		WIDGET_ID_TEXTINPUTFIELD_FOCUS,
		WIDGET_ID_TEXTINPUTFIELD_ACTIVE,
		WIDGET_ID_TEXTINPUTFIELD_DEACTIVATING,
		WIDGET_ID_TEXTINPUTFIELD_END, // do not use!

		// Slider:
		WIDGET_ID_SLIDER_BEGIN, // do not use!
		WIDGET_ID_SLIDER_BASE_IDLE,
		WIDGET_ID_SLIDER_BASE_FOCUS,
		WIDGET_ID_SLIDER_BASE_ACTIVE,
		WIDGET_ID_SLIDER_BASE_DEACTIVATING,
		WIDGET_ID_SLIDER_KNOB_IDLE,
		WIDGET_ID_SLIDER_KNOB_FOCUS,
		WIDGET_ID_SLIDER_KNOB_ACTIVE,
		WIDGET_ID_SLIDER_KNOB_DEACTIVATING,
		WIDGET_ID_SLIDER_END, // do not use!

		// Scrollbar:
		WIDGET_ID_SCROLLBAR_BEGIN, // do not use!
		WIDGET_ID_SCROLLBAR_BASE_IDLE,
		WIDGET_ID_SCROLLBAR_BASE_FOCUS,
		WIDGET_ID_SCROLLBAR_BASE_ACTIVE,
		WIDGET_ID_SCROLLBAR_BASE_DEACTIVATING,
		WIDGET_ID_SCROLLBAR_KNOB_INACTIVE,
		WIDGET_ID_SCROLLBAR_KNOB_HOVER,
		WIDGET_ID_SCROLLBAR_KNOB_GRABBED,
		WIDGET_ID_SCROLLBAR_END, // do not use!

		// Combo box:
		WIDGET_ID_COMBO_DROPDOWN,

		// Window:
		WIDGET_ID_WINDOW_BASE,

		// Colorpicker:
		WIDGET_ID_COLORPICKER_BASE,

		// other user-defined widget states can be specified after this (but in
		//  user's own enum): And you will of course need to handle it yourself
		//  in a SetColor() override for example
		WIDGET_ID_USER,
	};

	/**
	 * @brief Bit flags selecting which parts of a widget are localized.
	 *
	 * Controls what `ExportLocalization()` / `ImportLocalization()` touch.
	 * Combine with bitwise operators (see the `enable_bitmask_operators`
	 * specialization below).
	 */
	enum class LocalizationEnabled
	{
		/** Nothing is localized. */
		None = 0,

		/** The widget's main text. */
		Text = 1 << 0,

		/** The widget's tooltip text. */
		Tooltip = 1 << 1,

		/** ComboBox items. */
		Items = 1 << 2,

		/** Window children. */
		Children = 1 << 3,

		/** Everything localizable. */
		All = Text | Tooltip | Items | Children,
	};

	/**
	 * @brief Reusable styling descriptor applied to widgets.
	 *
	 * Bundles the image and font styling plus shadow/tooltip settings that a
	 * widget's `SetTheme()` reads. The nested @ref Image and @ref Font are
	 * reduced views of `wi::image::Params` / `wi::font::Params` that omit
	 * per-instance layout (position, alignment, size).
	 */
	struct Theme
	{
		/**
		 * @brief Themeable subset of `wi::image::Params`.
		 *
		 * Excludes position, alignment, size and other per-instance layout;
		 * holds only the visual style fields a theme should drive.
		 */
		struct Image
		{
			/** Default-value prototype the fields below initialize from. */
			inline static const wi::image::Params params;

			/** Tint color (RGBA). */
			XMFLOAT4 color = params.color;

			/** Blend mode used when drawing. */
			wi::enums::BLENDMODE blendFlag = params.blendFlag;

			/** Texture sampling/addressing mode. */
			wi::image::SAMPLEMODE sampleFlag = params.sampleFlag;

			/** Sampling quality (filtering). */
			wi::image::QUALITY quality = params.quality;

			/** Whether the background fill is drawn. */
			bool background = params.isBackgroundEnabled();

			/** Whether corners are rounded. */
			bool corner_rounding = params.isCornerRoundingEnabled();

			/** Per-corner rounding parameters. */
			wi::image::Params::Rounding corners_rounding[arraysize(
				params.corners_rounding
			)];

			/** Whether the pointer highlight effect is enabled. */
			bool highlight = false;

			/** Color of the highlight effect (RGB). */
			XMFLOAT3 highlight_color = XMFLOAT3(1, 1, 1);

			/** Spread/falloff of the highlight effect. */
			float highlight_spread = 1;

			/** Edge softening amount (0 = hard edges). */
			float border_soften = 0;

			/**
			 * Writes this theme's image style into render parameters.
			 *
			 * @param[in,out] params - Image parameters to update in place;
			 *                          layout fields are left untouched.
			 */
			void Apply(wi::image::Params& params) const
			{
				params.color = color;
				params.blendFlag = blendFlag;
				params.sampleFlag = sampleFlag;
				params.quality = quality;

				if (background)
				{
					params.enableBackground();
				}
				else
				{
					params.disableBackground();
				}
				if (corner_rounding)
				{
					params.enableCornerRounding();
				}
				else
				{
					params.disableCornerRounding();
				}
				if (highlight)
				{
					params.enableHighlight();
				}
				else
				{
					params.disableHighlight();
				}

				params.highlight_color = highlight_color;
				params.highlight_spread = highlight_spread;
				std::memcpy(
					params.corners_rounding,
					corners_rounding,
					sizeof(corners_rounding)
				);
			}

			/**
			 * Captures image style from existing render parameters.
			 *
			 * @param[in] params - Image parameters to copy the style from.
			 */
			void CopyFrom(const wi::image::Params& params)
			{
				color = params.color;
				blendFlag = params.blendFlag;
				sampleFlag = params.sampleFlag;
				quality = params.quality;
				background = params.isBackgroundEnabled();
				corner_rounding = params.isCornerRoundingEnabled();
				highlight = params.isHighlightEnabled();
				highlight_color = params.highlight_color;
				highlight_spread = params.highlight_spread;
				std::memcpy(
					corners_rounding,
					params.corners_rounding,
					sizeof(corners_rounding)
				);
			}
		} image;

		/**
		 * @brief Themeable subset of `wi::font::Params`.
		 *
		 * Excludes position, alignment and other per-instance layout; holds
		 * only the visual style fields a theme should drive.
		 */
		struct Font
		{
			/** Default-value prototype the fields below initialize from. */
			inline static const wi::font::Params params;

			/** Glyph color. */
			wi::Color color = params.color;

			/** Drop-shadow color. */
			wi::Color shadow_color = params.shadowColor;

			/** Font style index. */
			int style = params.style;

			/** Glyph edge softening. */
			float softness = params.softness;

			/** Glyph thickening amount. */
			float bolden = params.bolden;

			/** Shadow edge softening. */
			float shadow_softness = params.shadow_softness;

			/** Shadow thickening amount. */
			float shadow_bolden = params.shadow_bolden;

			/** Shadow horizontal offset. */
			float shadow_offset_x = params.shadow_offset_x;

			/** Shadow vertical offset. */
			float shadow_offset_y = params.shadow_offset_y;

			/**
			 * Writes this theme's font style into render parameters.
			 *
			 * @param[in,out] params - Font parameters to update in place;
			 *                          layout fields are left untouched.
			 */
			void Apply(wi::font::Params& params) const
			{
				params.color = color;
				params.shadowColor = shadow_color;
				params.style = style;
				params.softness = softness;
				params.bolden = bolden;
				params.shadow_softness = shadow_softness;
				params.shadow_bolden = shadow_bolden;
				params.shadow_offset_x = shadow_offset_x;
				params.shadow_offset_y = shadow_offset_y;
			}

			/**
			 * Captures font style from existing render parameters.
			 *
			 * @param[in] params - Font parameters to copy the style from.
			 */
			void CopyFrom(const wi::font::Params& params)
			{
				color = params.color;
				shadow_color = params.shadowColor;
				style = params.style;
				softness = params.softness;
				bolden = params.bolden;
				shadow_softness = params.shadow_softness;
				shadow_bolden = params.shadow_bolden;
				shadow_offset_x = params.shadow_offset_x;
				shadow_offset_y = params.shadow_offset_y;
			}
		} font;

		/**
		 * Widget shadow radius; if < 0, the widget's own value is not
		 * overridden.
		 */
		float shadow = -1;

		/** Shadow color for the whole widget. */
		wi::Color shadow_color = wi::Color::Shadow();

		/** Whether the shadow uses the pointer highlight effect. */
		bool shadow_highlight = false;

		/** Color of the shadow highlight (RGB). */
		XMFLOAT3 shadow_highlight_color = XMFLOAT3(1, 1, 1);

		/** Spread/falloff of the shadow highlight. */
		float shadow_highlight_spread = 1;

		/** Image style for the widget's tooltip. */
		Image tooltipImage;

		/** Font style for the widget's tooltip. */
		Font tooltipFont;

		/** Tooltip shadow radius; if < 0, not overridden. */
		float tooltip_shadow = -1;

		/** Tooltip shadow color. */
		wi::Color tooltip_shadow_color = wi::Color::Shadow();
	};
}

/**
 * Enables bitwise operators for @ref wi::gui::LocalizationEnabled.
 */
template<>
struct enable_bitmask_operators<wi::gui::LocalizationEnabled> : std::true_type {};
