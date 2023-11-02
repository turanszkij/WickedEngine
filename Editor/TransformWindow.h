#pragma once
class EditorComponent;

class TransformWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button clearButton;

	wi::gui::TextInputField txInput;
	wi::gui::TextInputField tyInput;
	wi::gui::TextInputField tzInput;

	wi::gui::TextInputField rollInput;
	wi::gui::TextInputField pitchInput;
	wi::gui::TextInputField yawInput;
					 
	wi::gui::TextInputField rxInput;
	wi::gui::TextInputField ryInput;
	wi::gui::TextInputField rzInput;
	wi::gui::TextInputField rwInput;
					 
	wi::gui::TextInputField sxInput;
	wi::gui::TextInputField syInput;
	wi::gui::TextInputField szInput;

	wi::gui::TextInputField snapScaleInput;
	wi::gui::TextInputField snapRotateInput;
	wi::gui::TextInputField snapTranslateInput;

	wi::gui::Button resetTranslationButton;
	wi::gui::Button resetScaleButton;
	wi::gui::Button resetRotationButton;

	void ResizeLayout() override;
};

