#pragma once

/**
 * @file
 * The `wi::gui::ComboBox` widget.
 *
 * A drop-down list: a closed box showing the current selection that, when
 * clicked, rolls down a list of selectable items with an optional text filter
 * and a scrollbar when the items exceed the visible count.
 */

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include <wiCanvas.h>
#include <wiColor.h>
#include <wiGraphicsDevice.h>
#include <wiGUI/GUICommon.h>
#include <wiGUI/Widget.h>
#include <wiLocalization.h>
#include <wiSpriteFont.h>
#include <wiVector.h>

#include "wiGUI/components/TextInputField.h"

namespace wi::gui
{
	/**
	 * Drop-down selection list.
	 *
	 * Shows the currently selected item and, when activated, drops down a list
	 * of items to choose from. Items carry a display name and a `uint64_t`
	 * userdata payload. When more items exist than @ref SetMaxVisibleItemCount,
	 * a scrollbar appears; an embedded @ref filter input narrows the visible
	 * items by substring match. Selecting an item raises @ref OnSelect with the
	 * index in `EventArgs::iValue` and the userdata in `EventArgs::userdata`.
	 * The list drops downward, or upward when there is not enough room below.
	 */
	class ComboBox :public Widget
	{
	protected:
		/** Callback invoked when an item is selected. */
		std::function<void(const EventArgs& args)> onSelect;

		/** Index of the selected item, or -1 if none. */
		int selected = -1;

		/** Maximum number of items shown before a scrollbar is needed. */
		int maxVisibleItemCount = 8;

		/** Index of the first visible item (scroll position). */
		int firstItemVisible = 0;

		/** Whether the drop-down arrow indicator is drawn. */
		bool drop_arrow = true;

		/** Fixed drop-down width; 0 takes the width from the base scale. */
		float fixed_drop_width = 0;

		/**
		 * Inner state machine used while the list is rolled down.
		 *
		 * Controls behaviour during the dropped-down (active) phase.
		 */
		enum COMBOSTATE
		{
			/** List just dropped down, or the widget is not active. */
			COMBOSTATE_INACTIVE,

			/** Dropped down with the hovered item highlighted. */
			COMBOSTATE_HOVER,

			/** The hovered item is being clicked. */
			COMBOSTATE_SELECTING,

			/** The scrollbar is about to be selected. */
			COMBOSTATE_SCROLLBAR_HOVER,

			/** The scrollbar is being dragged. */
			COMBOSTATE_SCROLLBAR_GRABBED,

			/** Interacting with the filter sub-widget. */
			COMBOSTATE_FILTER_INTERACT,

			/** Number of states (used for array sizing). */
			COMBOSTATE_COUNT,
		} combostate = COMBOSTATE_INACTIVE;

		/** Index of the currently hovered item, or -1 if none. */
		int hovered = -1;

		/** Scrollbar knob offset, in pixels. */
		float scrollbar_delta = 0;

		/**
		 * A single selectable entry in the list.
		 */
		struct Item
		{
			/** Display name of the item. */
			std::string name;

			/** Arbitrary user payload associated with the item. */
			uint64_t userdata = 0;
		};

		/** The list of selectable items. */
		wi::vector<Item> items;

		/** Background color of the dropped-down list. */
		wi::Color drop_color = wi::Color::Ghost();

		/** Text shown when the selection is invalid (index -1). */
		std::wstring invalid_selection_text;

		/** Embedded input field for filtering the visible items. */
		TextInputField filter;

		/** Current (uppercased) filter substring. */
		std::string filterText;

		/** Number of items currently passing the filter. */
		int filteredItemCount = 0;

		/**
		 * Returns the vertical offset of the dropped-down list.
		 *
		 * Positive drops below the box; negative drops above when there is not
		 * enough room below.
		 *
		 * @param[in] canvas - Canvas providing logical size.
		 *
		 * @return Vertical offset in pixels.
		 */
		[[nodiscard]] float GetDropOffset(const wi::Canvas& canvas) const;

		/**
		 * Returns the horizontal position of the dropped-down list.
		 *
		 * @param[in] canvas - Canvas providing logical size.
		 *
		 * @return X position in pixels.
		 */
		[[nodiscard]] float GetDropX(const wi::Canvas& canvas) const;

		/**
		 * Returns the vertical offset of a given item in the list.
		 *
		 * Accounts for filtering and the current scroll position.
		 *
		 * @param[in] canvas - Canvas providing logical size.
		 * @param[in] index - Item index.
		 *
		 * @return Vertical offset in pixels.
		 */
		[[nodiscard]] float GetItemOffset(
			const wi::Canvas& canvas,
			int index
		) const;
	public:
		/**
		 * Initializes the combo box with default size, text and filter.
		 *
		 * @param[in] name - Widget name; also used as the initial text.
		 */
		void Create(const std::string& name);

		/**
		 * Appends an item to the list.
		 *
		 * @param[in] name - Display name of the item.
		 * @param[in] userdata - Optional user payload.
		 */
		void AddItem(const std::string& name, uint64_t userdata = 0);

		/**
		 * Removes the item at the given index.
		 *
		 * @param[in] index - Index of the item to remove.
		 */
		void RemoveItem(int index);

		/**
		 * Removes all items.
		 */
		void ClearItems();

		/**
		 * Sets the maximum number of items shown before scrolling.
		 *
		 * @param[in] value - Maximum visible item count.
		 */
		void SetMaxVisibleItemCount(int value);

		/**
		 * Returns whether the dropped-down list needs a scrollbar.
		 */
		[[nodiscard]] bool HasScrollbar() const noexcept;

		/**
		 * Selects an item by index and raises @ref OnSelect.
		 *
		 * @param[in] index - Item index, or -1 to clear the selection.
		 */
		void SetSelected(int index);

		/**
		 * Selects an item by index without raising @ref OnSelect.
		 *
		 * @param[in] index - Item index, or -1 to clear the selection.
		 */
		void SetSelectedWithoutCallback(int index);

		/**
		 * Selects the first item with matching userdata; raises @ref OnSelect.
		 *
		 * @param[in] userdata - Userdata to match.
		 */
		void SetSelectedByUserdata(uint64_t userdata);

		/**
		 * Selects by userdata without raising @ref OnSelect.
		 *
		 * @param[in] userdata - Userdata to match.
		 */
		void SetSelectedByUserdataWithoutCallback(uint64_t userdata);

		/**
		 * Returns the selected item index, or -1 if none.
		 */
		[[nodiscard]] int GetSelected() const noexcept;

		/**
		 * Returns the userdata of the selected item.
		 */
		[[nodiscard]] uint64_t GetSelectedUserdata() const noexcept;

		/**
		 * Sets the display name of an item.
		 *
		 * @param[in] index - Item index.
		 * @param[in] text - New display name.
		 */
		void SetItemText(int index, const std::string& text);

		/**
		 * Sets the userdata of an item.
		 *
		 * @param[in] index - Item index.
		 * @param[in] userdata - New user payload.
		 */
		void SetItemUserdata(int index, uint64_t userdata);

		/**
		 * Returns the display name of an item.
		 *
		 * @param[in] index - Item index.
		 *
		 * @return The item's display name.
		 */
		[[nodiscard]] std::string GetItemText(int index) const;

		/**
		 * Returns the userdata of an item.
		 *
		 * @param[in] index - Item index.
		 *
		 * @return The item's user payload.
		 */
		[[nodiscard]] uint64_t GetItemUserData(int index) const noexcept;

		/**
		 * Returns the number of items.
		 */
		[[nodiscard]] size_t GetItemCount() const noexcept {
			return items.size();
		}

		/**
		 * Sets the text shown when no valid item is selected.
		 *
		 * @param[in] text - Placeholder text.
		 */
		void SetInvalidSelectionText(const std::string& text);

		/**
		 * Updates interaction: drop-down, filtering, scrolling and selection.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the box, selection text, and the dropped-down list if active.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Sets the color of the combo box.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the combo box.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("ComboBox").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override {
			return "ComboBox";
		}

		/**
		 * Sets the selection callback.
		 *
		 * @param[in] func - Callback invoked when an item is selected.
		 */
		void OnSelect(std::function<void(const EventArgs& args)> func);

		/** Font used to draw the selected item's text in the closed box. */
		wi::SpriteFont selected_font;

		/**
		 * Exports the items' localizable strings into a localization.
		 *
		 * @param[in,out] localization - Localization to add entries to.
		 */
		void ExportLocalization(wi::Localization& localization) const override;

		/**
		 * Imports localized item strings from a localization.
		 *
		 * @param[in] localization - Localization to read entries from.
		 */
		void ImportLocalization(const wi::Localization& localization) override;

		/**
		 * Enables or disables the drop-down arrow indicator.
		 *
		 * @param[in] value - true to draw the arrow.
		 */
		void SetDropArrowEnabled(bool value) noexcept { drop_arrow = value; }

		/**
		 * Returns whether the drop-down arrow indicator is drawn.
		 */
		[[nodiscard]] bool IsDropArrowEnabled() const noexcept {
			return drop_arrow;
		}

		/**
		 * Sets a fixed width for the dropped-down list.
		 *
		 * @param[in] value - Width in pixels; 0 uses the base scale width.
		 */
		void SetFixedDropWidth(float value) noexcept { fixed_drop_width = value; }

		/**
		 * Returns the fixed drop-down width (0 if not fixed).
		 */
		[[nodiscard]] float GetFixedDropWidth() const noexcept {
			return fixed_drop_width;
		}
	};
}
