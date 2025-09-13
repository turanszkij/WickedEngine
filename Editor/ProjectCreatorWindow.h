#pragma once
class EditorComponent;

class ProjectCreatorWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::gui::Label infoLabel;
	wi::gui::TextInputField projectNameInput;
	wi::gui::Button reloadButton;
	wi::gui::Button iconButton;
	wi::gui::Button thumbnailButton;
	wi::gui::Button splashScreenButton;

	wi::gui::Label cursorLabel;
	wi::gui::Button cursorButton;

	wi::gui::ColorPicker fontColorPicker;
	wi::gui::ColorPicker backgroundColorPicker;
	wi::gui::Button colorPreviewButton;

	wi::gui::Button createButton;

	wi::Resource iconResource;
	wi::Resource thumbnailResource;
	wi::Resource splashScreenResource;
	wi::Resource splashScreenResourceCroppedPreview;
	wi::Resource cursorResource;

	std::string iconFilename;
	std::string thumbnailFilename;
	std::string splashScreenFilename;
	std::string cursorFilename;

	float hotspotX = 0.5f;
	float hotspotY = 0.5f;

	void Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const override;
	void ResizeLayout() override;
};

