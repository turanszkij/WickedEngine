/**
 * @file
 * Implementation of the @ref wi::gui::Image widget.
 */

#include "wiGUI/components/Image.h"
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
	void Image::Create(const std::string& name)
	{
		SetName(name);
		SetSize(XMFLOAT2(100, 100));
		SetEnabled(false);
	}
	void Image::Render(const wi::Canvas& canvas, CommandList cmd) const
	{
		if (!IsVisible())
		{
			return;
		}

		if (shadow > 0)
		{
			wi::image::Params fx = sprites[IDLE].params;
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
					corner_rounding.radius += shadow;
				}
			}
			if (shadow_highlight)
			{
				fx.enableHighlight();
				fx.highlight_color = shadow_highlight_color;
				fx.highlight_spread = shadow_highlight_spread;
			}
			wi::image::Draw(nullptr, fx, cmd);
		}

		ApplyScissor(canvas, scissorRect, cmd);

		sprites[IDLE].Draw(cmd);
	}
}
