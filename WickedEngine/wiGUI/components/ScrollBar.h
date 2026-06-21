#pragma once
#include "wiGUI/Widget.h"

namespace wi::gui
{
	// Generic scroll bar
	class ScrollBar : public Widget
	{
	protected:
		float scrollbar_delta = 0;
		float scrollbar_length = 0;
		float scrollbar_value = 0;
		float scrollbar_granularity = 1;
		float list_length = 0;
		float list_offset = 0;
		float overscroll = 0;
		bool vertical = true;
		float safe_area = 0;
		XMFLOAT2 grab_pos = {};
		float grab_delta = 0;

	public:
		// Set the list's length that will be scrollable and moving
		void SetListLength(float size) { list_length = size; }
		// The scrolling offset that should be applied to the list items
		float GetOffset() const { return list_offset; }
		void SetOffset(float value);
		// This can be called by user for extra scrolling on top of base functionality
		//	scroll sensitivity based on how much content is hidden (higher granularity = more amplification)
		void Scroll(const float amount)
		{
			if (scrollbar_granularity < 1.0f)
			{
				scrollbar_delta -= amount * scrollbar_granularity / (1.0f - scrollbar_granularity);
			}
		}
		// How much the max scrolling will offset the list even further than it would be necessary for fitting
		//	this value is in percent of a full scrollbar's worth of extra offset
		//	0: no extra offset
		//	1: full extra offset
		void SetOverScroll(float amount) { overscroll = amount; }
		// Check whether the scrollbar is required (when the items don't fit and scrolling could be used)
		bool IsScrollbarRequired() const { return scrollbar_granularity < 1; }
		// Check whether the scrollbar is at the beginning
		bool IsScrolledToBegin() const { return scrollbar_delta <= 0; }
		void SetSafeArea(float value) { safe_area = value; }

		enum SCROLLBAR_STATE
		{
			SCROLLBAR_INACTIVE,
			SCROLLBAR_HOVER,
			SCROLLBAR_GRABBED,
			SCROLLBAR_STATE_COUNT,
		} scrollbar_state = SCROLLBAR_INACTIVE;
		wi::Sprite sprites_knob[SCROLLBAR_STATE_COUNT];
		XMFLOAT2 knob_inset_border = {};

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "ScrollBar"; }

		void SetVertical(bool value) { vertical = value; }
		bool IsVertical() const { return vertical; }
	};
}
