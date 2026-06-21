#pragma once
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

#include <string>
#include <functional>

namespace wi::gui
{
	struct EventArgs
	{
		XMFLOAT2 clickPos = {};	// mouse click coordinate
		XMFLOAT2 startPos = {};	// mouse start position of operation (drag)
		XMFLOAT2 deltaPos = {}; // mouse delta position of operation since last update (drag)
		XMFLOAT2 endPos = {};	// mouse end position of operation (drag)
		float fValue = 0;		// generic float value of operation
		bool bValue = false;	// generic boolean value of operation
		int iValue = 0;			// generic integer value of operation
		int iValue2 = 0;		// secondary generic integer value of operation
		wi::Color color;		// color value of color picker operation
		std::string sValue;		// generic string value of operation
		uint64_t userdata = 0;	// this will provide the userdata value that was set to a widget (or part of a widget)
	};

	enum WIDGETSTATE
	{
		IDLE,			// widget is doing nothing
		FOCUS,			// widget got pointer dragged on or selected
		ACTIVE,			// widget is interacted with right now
		DEACTIVATING,	// widget has last been active but no more interactions are occuring
		WIDGETSTATE_COUNT,
	};

	// These can be used to target a setting for a specific widget control and state:
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

		// other user-defined widget states can be specified after this (but in user's own enum):
		//	And you will of course need to handle it yourself in a SetColor() override for example
		WIDGET_ID_USER,
	};

	enum class LocalizationEnabled
	{
		None = 0,
		Text = 1 << 0,
		Tooltip = 1 << 1,
		Items = 1 << 2,		// ComboBox items
		Children = 1 << 3,	// Window children

		All = Text | Tooltip | Items | Children,
	};

	struct Theme
	{
		// Reduced version of wi::image::Params, excluding position, alignment, etc.
		struct Image
		{
			inline static const wi::image::Params params; // prototype for default values
			XMFLOAT4 color = params.color;
			wi::enums::BLENDMODE blendFlag = params.blendFlag;
			wi::image::SAMPLEMODE sampleFlag = params.sampleFlag;
			wi::image::QUALITY quality = params.quality;
			bool background = params.isBackgroundEnabled();
			bool corner_rounding = params.isCornerRoundingEnabled();
			wi::image::Params::Rounding corners_rounding[arraysize(params.corners_rounding)];
			bool highlight = false;
			XMFLOAT3 highlight_color = XMFLOAT3(1, 1, 1);
			float highlight_spread = 1;
			float border_soften = 0;

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
				std::memcpy(params.corners_rounding, corners_rounding, sizeof(corners_rounding));
			}
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
				std::memcpy(corners_rounding, params.corners_rounding, sizeof(corners_rounding));
			}
		} image;

		// Reduced version of wi::font::Params, excluding position, alignment, etc.
		struct Font
		{
			inline static const wi::font::Params params; // prototype for default values
			wi::Color color = params.color;
			wi::Color shadow_color = params.shadowColor;
			int style = params.style;
			float softness = params.softness;
			float bolden = params.bolden;
			float shadow_softness = params.shadow_softness;
			float shadow_bolden = params.shadow_bolden;
			float shadow_offset_x = params.shadow_offset_x;
			float shadow_offset_y = params.shadow_offset_y;

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

		float shadow = -1; // shadow radius, if less than 0, it won't be used to override
		wi::Color shadow_color = wi::Color::Shadow(); // shadow color for whole widget
		bool shadow_highlight = false;
		XMFLOAT3 shadow_highlight_color = XMFLOAT3(1, 1, 1);
		float shadow_highlight_spread = 1;

		Image tooltipImage;
		Font tooltipFont;
		float tooltip_shadow = -1; // shadow radius, if less than 0, it won't be used to override
		wi::Color tooltip_shadow_color = wi::Color::Shadow();
	};
}

template<>
struct enable_bitmask_operators<wi::gui::LocalizationEnabled> : std::true_type {};
