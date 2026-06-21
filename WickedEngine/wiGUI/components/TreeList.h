#pragma once
#include "wiGUI/components/ScrollBar.h"

namespace wi::gui
{
	// List of items in a tree (parent-child relationships)
	class TreeList : public Widget
	{
	public:
		struct Item
		{
			std::string name;
			int level = 0;
			uint64_t userdata = 0;
			bool open = false;
			bool selected = false;
		};
	protected:
		std::function<void(const EventArgs& args)> onSelect;
		std::function<void(const EventArgs& args)> onDelete;
		std::function<void(const EventArgs& args)> onDoubleClick;
		int item_highlight = -1;
		int opener_highlight = -1;

		wi::primitive::Hitbox2D GetHitbox_ListArea() const;
		wi::primitive::Hitbox2D GetHitbox_Item(int visible_count, int level) const;
		wi::primitive::Hitbox2D GetHitbox_ItemOpener(int visible_count, int level) const;

		wi::vector<Item> items;

		float GetItemOffset(int index) const;
		bool DoesItemHaveChildren(int index) const;

		float resizehitboxwidth = 6;
		enum RESIZE_STATE
		{
			RESIZE_STATE_NONE,
			RESIZE_STATE_BOTTOM,
		} resize_state = RESIZE_STATE_NONE;
		XMFLOAT2 resize_begin = XMFLOAT2(0, 0);
		float resize_blink_timer = 0;

		// Drag-and-drop reorder state
		std::function<void(const EventArgs& args)> onReorder;
		int drag_source = -1;           // items[] index being dragged (-1 = none)
		int drag_target = -1;           // items[] index to insert before (items.size() = end)
		int drag_reparent_target = -1;  // items[] index to drop ON (re-parent), -1 = none
		int drag_target_level = 0;      // item level for BETWEEN drops (0 = root)
		bool dragging = false;
		XMFLOAT2 drag_start_pos = {};
				float drag_indicator_y = 0;    // screen Y for drop indicator line (updated in Update)
				XMFLOAT2 drag_pointer_pos = {}; // current pointer pos while dragging (for ghost rendering)
		void ComputeScrollbarLength();

	public:
		void Create(const std::string& name);

		void AddItem(const Item& item);
		void AddItem(const std::string& name);
		void ClearItems();
		bool HasScrollbar() const;

		void ClearSelection();
		void Select(int index, bool allow_deselect = true);
		void FocusOnItem(int index);
		void FocusOnItemByUserdata(uint64_t userdata);

		int GetItemCount() const { return (int)items.size(); }
		const Item& GetItem(int index) const;

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "TreeList"; }

		void OnSelect(std::function<void(const EventArgs& args)> func);
		void OnDelete(std::function<void(const EventArgs& args)> func);
		void OnDoubleClick(std::function<void(const EventArgs& args)> func);
		void OnReorder(std::function<void(const EventArgs& args)> func);

		ScrollBar scrollbar;
	};
}
