#pragma once

/**
 * @file
 * The `wi::gui::TreeList` widget.
 *
 * A scrollable list of items arranged in a tree by indentation level, with
 * collapsible parents, selection, bottom-edge resizing, and optional
 * drag-and-drop reordering / re-parenting.
 */

#include <cstdint>
#include <functional>
#include <string>

#include <Utility/DirectXMath/DirectXMath.h>

#include <wiCanvas.h>
#include <wiColor.h>
#include <wiGraphicsDevice.h>
#include <wiGUI/GUICommon.h>
#include <wiGUI/Widget.h>
#include <wiPrimitive.h>
#include <wiVector.h>

#include "wiGUI/components/ScrollBar.h"

namespace wi::gui
{
	/**
	 * List of items in a tree (parent-child relationships).
	 *
	 * Items are stored in a flat list and nested by their @ref Item::level;
	 * a parent's children are the following items with a higher level, and a
	 * parent can be collapsed (@ref Item::open). The list scrolls via the
	 * embedded @ref scrollbar, supports single/multi selection, can be resized
	 * by dragging its bottom edge, and optionally supports drag-and-drop to
	 * reorder or re-parent items. Interactions raise @ref OnSelect,
	 * @ref OnDelete, @ref OnDoubleClick and @ref OnReorder.
	 */
	class TreeList : public Widget
	{
	public:
		/**
		 * A single entry in the tree.
		 */
		struct Item
		{
			/** Display name of the item. */
			std::string name;

			/** Nesting depth (0 = root); deeper items are children. */
			int level = 0;

			/** Arbitrary user payload associated with the item. */
			uint64_t userdata = 0;

			/** Whether this item's children are expanded. */
			bool open = false;

			/** Whether this item is selected. */
			bool selected = false;
		};
	protected:
		/** Callback invoked when the selection changes. */
		std::function<void(const EventArgs& args)> onSelect;

		/** Callback invoked when an item is deleted. */
		std::function<void(const EventArgs& args)> onDelete;

		/** Callback invoked when an item is double-clicked. */
		std::function<void(const EventArgs& args)> onDoubleClick;

		/** Index of the currently highlighted item, or -1 if none. */
		int item_highlight = -1;

		/** Index of the item whose opener is highlighted, or -1 if none. */
		int opener_highlight = -1;

		/**
		 * Returns the hitbox of the scrollable list area.
		 *
		 * @return Hitbox covering the list region (excluding the header).
		 */
		[[nodiscard]] wi::primitive::Hitbox2D GetHitbox_ListArea() const
			noexcept;

		/**
		 * Returns the hitbox of a visible item row.
		 *
		 * @param[in] visible_count - Row index among the visible items.
		 * @param[in] level - Nesting level of the item.
		 *
		 * @return Hitbox of the item row.
		 */
		[[nodiscard]] wi::primitive::Hitbox2D GetHitbox_Item(
			int visible_count,
			int level
		) const noexcept;

		/**
		 * Returns the hitbox of a visible item's collapse/expand opener.
		 *
		 * @param[in] visible_count - Row index among the visible items.
		 * @param[in] level - Nesting level of the item.
		 *
		 * @return Hitbox of the item's opener control.
		 */
		[[nodiscard]] wi::primitive::Hitbox2D GetHitbox_ItemOpener(
			int visible_count,
			int level
		) const noexcept;

		/** The flat list of items (nested by @ref Item::level). */
		wi::vector<Item> items;

		/**
		 * Returns the vertical offset of an item, accounting for scrolling.
		 *
		 * @param[in] index - Item index.
		 *
		 * @return Vertical offset in pixels.
		 */
		[[nodiscard]] float GetItemOffset(int index) const noexcept;

		/**
		 * Returns whether the item at the given index has children.
		 *
		 * @param[in] index - Item index.
		 *
		 * @return true if a following item has a deeper level.
		 */
		[[nodiscard]] bool DoesItemHaveChildren(int index) const noexcept;

		/** Width of the bottom-edge resize grab area, in pixels. */
		float resizehitboxwidth = 6;

		/**
		 * Which edge (if any) is currently being resized.
		 */
		enum RESIZE_STATE
		{
			/** Not resizing. */
			RESIZE_STATE_NONE,

			/** Resizing via the bottom edge. */
			RESIZE_STATE_BOTTOM,
		} resize_state = RESIZE_STATE_NONE;

		/** Pointer position where the resize drag began. */
		XMFLOAT2 resize_begin = XMFLOAT2(0, 0);

		/** Timer driving the resize-handle blink feedback. */
		float resize_blink_timer = 0;

		// Drag-and-drop reorder state

		/** Callback invoked when items are reordered by drag-and-drop. */
		std::function<void(const EventArgs& args)> onReorder;

		/** `items[]` index being dragged (-1 = none). */
		int drag_source = -1;

		/** `items[]` index to insert before (`items.size()` = end). */
		int drag_target = -1;

		/** `items[]` index to drop onto for re-parenting (-1 = none). */
		int drag_reparent_target = -1;

		/** Item level for between-item drops (0 = root). */
		int drag_target_level = 0;

		/** Whether a drag-and-drop operation is in progress. */
		bool dragging = false;

		/** Pointer position where the drag began. */
		XMFLOAT2 drag_start_pos = {};

		/** Screen Y of the drop indicator line (updated in Update). */
		float drag_indicator_y = 0;

		/** Current pointer position while dragging (for ghost rendering). */
		XMFLOAT2 drag_pointer_pos = {};

		/**
		 * Recomputes the scrollbar length from the items and list size.
		 */
		void ComputeScrollbarLength();

	public:
		/**
		 * Initializes the tree list with default size and scrollbar styling.
		 *
		 * @param[in] name - Widget name.
		 */
		void Create(const std::string& name);

		/**
		 * Appends an item.
		 *
		 * @param[in] item - Item to add.
		 */
		void AddItem(const Item& item);

		/**
		 * Appends a root-level item with the given name.
		 *
		 * @param[in] name - Display name of the item.
		 */
		void AddItem(const std::string& name);

		/**
		 * Removes all items.
		 */
		void ClearItems();

		/**
		 * Returns whether the list needs a scrollbar.
		 */
		[[nodiscard]] bool HasScrollbar() const noexcept;

		/**
		 * Clears the selection of all items.
		 */
		void ClearSelection();

		/**
		 * Selects an item by index.
		 *
		 * @param[in] index - Item index.
		 * @param[in] allow_deselect - If true, clicking a selected item again
		 *                             deselects it.
		 */
		void Select(int index, bool allow_deselect = true);

		/**
		 * Scrolls the list so the given item is visible.
		 *
		 * @param[in] index - Item index.
		 */
		void FocusOnItem(int index);

		/**
		 * Scrolls the list to the first item with matching userdata.
		 *
		 * @param[in] userdata - Userdata to match.
		 */
		void FocusOnItemByUserdata(uint64_t userdata);

		/**
		 * Returns the number of items.
		 */
		[[nodiscard]] int GetItemCount() const noexcept {
			return static_cast<int>(items.size());
		}

		/**
		 * Returns the item at the given index.
		 *
		 * @param[in] index - Item index.
		 *
		 * @return Reference to the item.
		 */
		[[nodiscard]] const Item& GetItem(int index) const noexcept;

		/**
		 * Updates interaction: selection, scrolling, resizing and dragging.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] dt - Delta time in seconds since the last update.
		 */
		void Update(const wi::Canvas& canvas, float dt) override;

		/**
		 * Draws the items, openers, scrollbar and drag feedback.
		 *
		 * @param[in] canvas - Canvas providing resolution / DPI context.
		 * @param[in] cmd - Command list to record draw commands into.
		 */
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd)
			const override;

		/**
		 * Sets the color of the tree list and its scrollbar.
		 *
		 * @param[in] color - Color to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetColor(wi::Color color, int id = -1) override;

		/**
		 * Applies a theme to the tree list and its scrollbar.
		 *
		 * @param[in] theme - Theme to apply.
		 * @param[in] id - Target @ref WIDGET_ID; -1 applies to all states.
		 */
		void SetTheme(const Theme& theme, int id = -1) override;

		/**
		 * Returns the widget's type name ("TreeList").
		 */
		[[nodiscard]] const char* GetWidgetTypeName() const override
		{
			return "TreeList";
		}

		/**
		 * Sets the selection callback.
		 *
		 * @param[in] func - Callback invoked when the selection changes.
		 */
		void OnSelect(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the delete callback.
		 *
		 * @param[in] func - Callback invoked when an item is deleted.
		 */
		void OnDelete(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the double-click callback.
		 *
		 * @param[in] func - Callback invoked when an item is double-clicked.
		 */
		void OnDoubleClick(std::function<void(const EventArgs& args)> func);

		/**
		 * Sets the reorder callback.
		 *
		 * @param[in] func - Callback invoked when items are reordered.
		 */
		void OnReorder(std::function<void(const EventArgs& args)> func);

		/** Vertical scrollbar for the item list. */
		ScrollBar scrollbar;
	};
}
