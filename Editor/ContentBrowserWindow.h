#pragma once
class EditorComponent;

class ContentBrowserWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	std::string content_folder;
	enum SELECTION
	{
		SELECTION_ALL,
		SELECTION_MODELS,
		SELECTION_SCRIPTS,

		SELECTION_RECENTFOLDER_BEGIN,
		SELECTION_RECENTFOLDER_END = SELECTION_RECENTFOLDER_BEGIN + 10,

		SELECTION_COUNT
	};
	enum SORTING
	{
		SORTING_RECENT,
		SORTING_NAME,
		SORTING_PATH,
		SORTING_SIZE,

		SORTING_COUNT
	};
	SELECTION current_selection = SELECTION_COUNT;
	SORTING current_sorting = SORTING_RECENT;
	wi::gui::Button folderButtons[SELECTION_COUNT];
	std::deque<wi::gui::Button> itemButtons;

	wi::gui::Button openFolderButton;
	wi::gui::ComboBox sortingComboBox;

	struct ItemInfo
	{
		std::string fileName;
		std::string icon;
		size_t recent_index = 0;
		size_t file_size = 0;
	};
	std::vector<ItemInfo> added_items;

	void RefreshContent();

	void SetSelection(SELECTION selection);
	void AddItems(const std::string& folder, const std::string& extension, const std::string& icon);
	void AddItem(const std::string& fileName, const std::string& icon);
	void SortAndCreateItemButtons();
	void CreateItemButton(const std::string& fileName, const std::string& icon);

	void Update(const wi::Canvas& canvas, float dt) override;
	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
	void ResizeLayout() override;
};

