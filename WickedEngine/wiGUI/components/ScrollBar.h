#pragma once

/**
 * @file
 * The `wi::gui::ScrollBar` widget.
 *
 * A generic vertical or horizontal scroll bar. Given the total length of a
 * scrollable list, it computes a draggable knob and exposes the resulting
 * scroll offset to apply to the list's items.
 */

#include "wiGUI/Widget.h"

namespace wi::gui
{
	/**
	 * Generic vertical or horizontal scroll bar.
	 *
	 * Driven by the scrollable content's total length (@ref SetListLength). It
	 * sizes a knob proportional to how much of the content is visible and lets
	 * the user drag it; the resulting offset to apply to the list is read via
	 * @ref GetOffset. Scrolling is only active when the content does not fit
	 * (@ref IsScrollbarRequired). Orientation defaults to vertical and can be
	 * changed with @ref SetVertical.
	 */
	class ScrollBar : public Widget
	{
	protected:
		/** Knob offset from the bar start, in pixels. */
		float scrollbar_delta = 0;

		/** Knob length along the scroll axis, in pixels. */
		float scrollbar_length = 0;

		/** Normalized scroll position in `[0, 1]`. */
		float scrollbar_value = 0;

		/** Visible-to-total ratio; `< 1` means scrolling is needed. */
		float scrollbar_granularity = 1;

		/** Total length of the scrollable content, in pixels. */
		float list_length = 0;

		/** Resulting offset to apply to the list items (see @ref GetOffset). */
		float list_offset = 0;

		/** Extra over-scroll past the fitting point (fraction of a bar). */
		float overscroll = 0;

		/** Whether the bar is oriented vertically (else horizontally). */
		bool vertical = true;

		/** Length subtracted from the content when sizing the knob. */
		float safe_area = 0;

		/** Pointer position captured when the knob was grabbed. */
		XMFLOAT2 grab_pos = {};

		/** Knob offset captured when the knob was grabbed. */
		float grab_delta = 0;

	public:
		/**
		 * Sets the scrollable content's total length.
		 *
		 * @param[in] size - Length of the scrollable list, in pixels.
		 */
		void SetListLength(float size) noexcept
		{
			list_length = size;
		}

		/**
		 * Returns the scroll offset to apply to the list items.
		 *
		 * @return Offset in pixels (negative scrolls content up/left).
		 */
		[[nodiscard]] float GetOffset() const noexcept
		{
			return list_offset;
		}

		/**
		 * Sets the scroll position from a list offset value.
		 *
		 * @param[in] value - Target offset into the list.
		 */
		void SetOffset(float value);

		/**
		 * Applies extra scrolling on top of the base interaction.
		 *
		 * Scroll sensitivity scales with how much content is hidden (higher
		 * granularity amplifies the scroll). Intended for user-driven extra
		 * scrolling, e.g. the mouse wheel.
		 *
		 * @param[in] amount - Scroll amount to apply.
		 */
		void Scroll(const float amount) noexcept
		{
			if (scrollbar_granularity < 1.0f)
			{
				scrollbar_delta -=
					amount * scrollbar_granularity
					/ (1.0f - scrollbar_granularity);
			}
		}

		/**
		 * Sets how far past the fitting point the list may over-scroll.
		 *
		 * Expressed as a fraction of a full scrollbar's worth of extra offset:
		 * 0 means no extra offset, 1 means a full extra offset.
		 *
		 * @param[in] amount - Over-scroll fraction in `[0, 1]`.
		 */
		void SetOverScroll(float amount) noexcept
		{
			overscroll = amount;
		}

		/**
		 * Returns whether a scrollbar is needed (content does not fit).
		 */
		[[nodiscard]] bool IsScrollbarRequired() const noexcept
		{
			return scrollbar_granularity < 1;
		}

		/**
		 * Returns whether the bar is scrolled to the beginning.
		 */
		[[nodiscard]] bool IsScrolledToBegin() const noexcept
		{
			return scrollbar_delta <= 0;
		}

		/**
		 * Sets the safe area subtracted from the content length.
		 *
		 * @param[in] value - Length to reserve, in pixels.
		 */
		void SetSafeArea(float value) noexcept
		{
			safe_area = value;
		}

		/**
		 * Visual/interaction state of the scrollbar knob.
		 */
		enum SCROLLBAR_STATE
		{
			/** Knob is idle. */
			SCROLLBAR_INACTIVE,

			/** Pointer is hovering the knob. */
			SCROLLBAR_HOVER,

			/** Knob is being dragged. */
			SCROLLBAR_GRABBED,

			/** Number of states (used for array sizing). */
			SCROLLBAR_STATE_COUNT,
		} scrollbar_state = SCROLLBAR_INACTIVE;

		/** Knob sprite for each @ref SCROLLBAR_STATE. */
		wi::Sprite sprites_knob[SCROLLBAR_STATE_COUNT];

		/** Inset of the knob within the bar, in pixels (x, y). */
		XMFLOAT2 knob_inset_border = {};

		/**
		 * Updates the knob geometry and the drag interaction.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the bar background and the knob (when scrolling is required).
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Sets the color of the bar base or knob states.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; the `WIDGET_ID_SCROLLBAR_*`
		 *                 IDs address the base/knob states. -1 applies to all.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the bar base or knob states.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("ScrollBar").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override
		{
			return "ScrollBar";
		}

		/**
		 * Sets the bar orientation.
		 *
		 * @param[in] value - true for vertical, false for horizontal.
		 */
		void SetVertical(bool value) noexcept
		{
			vertical = value;
		}

		/**
		 * Returns whether the bar is oriented vertically.
		 */
		[[nodiscard]] bool IsVertical() const noexcept
		{
			return vertical;
		}
	};
}
