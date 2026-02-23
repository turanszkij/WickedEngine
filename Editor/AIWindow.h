#pragma once
class EditorComponent;

class AIWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	wi::gui::Label infoLabel;
	wi::gui::Button preview;
	wi::gui::TextInputField prompt;
	wi::gui::Button generate;
	wi::gui::ComboBox hardwareCombo;
	wi::gui::ComboBox modelCombo;
	wi::gui::TextInputField widthInput;
	wi::gui::TextInputField heightInput;

	void ResizeLayout() override;

	void GenerateImage();

	std::mutex preview_locker;
	wi::Resource imageResource;
	wi::jobsystem::context generation_status;
	std::atomic<int> progress{ 0 };
};
