#pragma once
#include "wiGUI/components/TextInputField.h"

namespace wi::gui
{
	// Drop-down list
	class ComboBox :public Widget
	{
	protected:
		std::function<void(const EventArgs& args)> onSelect;
		int selected = -1;
		int maxVisibleItemCount = 8;
		int firstItemVisible = 0;
		bool drop_arrow = true;
		float fixed_drop_width = 0; // 0 = not fixed, takes width from base scale

		// While the widget is active (rolled down) these are the inner states that control behaviour
		enum COMBOSTATE
		{
			COMBOSTATE_INACTIVE,	// When the list is just being dropped down, or the widget is not active
			COMBOSTATE_HOVER,		// The widget is in drop-down state with the last item hovered highlited
			COMBOSTATE_SELECTING,	// The hovered item is clicked
			COMBOSTATE_SCROLLBAR_HOVER,		// scrollbar is to be selected
			COMBOSTATE_SCROLLBAR_GRABBED,	// scrollbar is moved
			COMBOSTATE_FILTER_INTERACT, // interaction with filter sub-widget
			COMBOSTATE_COUNT,
		} combostate = COMBOSTATE_INACTIVE;
		int hovered = -1;

		float scrollbar_delta = 0;

		struct Item
		{
			std::string name;
			uint64_t userdata = 0;
		};
		wi::vector<Item> items;

		wi::Color drop_color = wi::Color::Ghost();
		std::wstring invalid_selection_text;

		TextInputField filter;
		std::string filterText;
		int filteredItemCount = 0;

		float GetDropOffset(const wi::Canvas& canvas) const;
		float GetDropX(const wi::Canvas& canvas) const;
		float GetItemOffset(const wi::Canvas& canvas, int index) const;
	public:
		void Create(const std::string& name);

		void AddItem(const std::string& name, uint64_t userdata = 0);
		void RemoveItem(int index);
		void ClearItems();
		void SetMaxVisibleItemCount(int value);
		bool HasScrollbar() const;

		void SetSelected(int index);
		void SetSelectedWithoutCallback(int index); // SetSelected() but the OnSelect callback will not be executed
		void SetSelectedByUserdata(uint64_t userdata);
		void SetSelectedByUserdataWithoutCallback(uint64_t userdata); // SetSelectedByUserdata() but the OnSelect callback will not be executed
		int GetSelected() const;
		uint64_t GetSelectedUserdata() const;
		void SetItemText(int index, const std::string& text);
		void SetItemUserdata(int index, uint64_t userdata);
		std::string GetItemText(int index) const;
		uint64_t GetItemUserData(int index) const;
		size_t GetItemCount() const { return items.size(); }
		void SetInvalidSelectionText(const std::string& text);

		void Update(const wi::Canvas& canvas, float dt) override;
		void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
		void SetColor(wi::Color color, int id = -1) override;
		void SetTheme(const Theme& theme, int id = -1) override;
		const char* GetWidgetTypeName() const override { return "ComboBox"; }

		void OnSelect(std::function<void(const EventArgs& args)> func);

		wi::SpriteFont selected_font;

		void ExportLocalization(wi::Localization& localization) const override;
		void ImportLocalization(const wi::Localization& localization) override;

		void SetDropArrowEnabled(bool value) { drop_arrow = value; }
		bool IsDropArrowEnabled() const { return drop_arrow; }
		void SetFixedDropWidth(float value) { fixed_drop_width = value; }
		float GetFixedDropWidth() const { return fixed_drop_width; }
	};
}
