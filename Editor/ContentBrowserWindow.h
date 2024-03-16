#pragma once
class EditorComponent;

class ContentBrowserWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	void ResetCam();

	EditorComponent* editor = nullptr;

	std::string content_folder = "../Content/";
	wi::vector<wi::gui::Button> folderButtons;

	class ItemWindow : public wi::gui::Window
	{
	public:
		EditorComponent* editor = nullptr;
		wi::vector<wi::gui::Button> itemButtons;

		void ClearItems();
		void AddItems(const std::string& folder, const std::string& extension);
		void RegisterItems();

		void ResizeLayout() override;
	};
	ItemWindow itemWindow;

	void Update(const wi::Canvas& canvas, float dt);
	void ResizeLayout() override;
};

