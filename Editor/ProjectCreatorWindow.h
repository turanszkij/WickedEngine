#pragma once
class EditorComponent;

class ProjectCreatorWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::gui::Label infoLabel;
	wi::gui::TextInputField projectNameInput;
	wi::gui::Button iconButton;
	wi::gui::Button thumbnailButton;
	wi::gui::Button splashScreenButton;
	wi::gui::Button createButton;

	wi::gui::ColorPicker backlogColorPicker;
	wi::gui::ColorPicker backgroundColorPicker;
	wi::gui::Button colorPreviewButton;

	wi::Resource iconResource;
	std::string iconName;

	wi::Resource thumbnailResource;
	std::string thumbnailName;

	wi::Resource splashScreenResource;
	wi::Resource splashScreenResourceCroppedPreview;
	std::string splashScreenName;

	void ResizeLayout() override;
};

