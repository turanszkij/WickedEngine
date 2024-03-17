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
		SELECTION_SCRIPTS,
		SELECTION_MODELS,

		SELECTION_COUNT
	};
	SELECTION current_selection = SELECTION_COUNT;
	wi::gui::Button folderButtons[SELECTION_COUNT];
	wi::vector<wi::gui::Button> itemButtons;

	wi::gui::Button openFolderButton;

	void RefreshContent();

	void SetSelection(SELECTION selection);
	void AddItems(const std::string& folder, const std::string& extension, const std::string& icon);

	void Update(const wi::Canvas& canvas, float dt);
	void ResizeLayout() override;
};

