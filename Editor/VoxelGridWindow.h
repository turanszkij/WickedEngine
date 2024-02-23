#pragma once
class EditorComponent;

class VoxelGridWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::TextInputField dimXInput;
	wi::gui::TextInputField dimYInput;
	wi::gui::TextInputField dimZInput;
	wi::gui::Button	clearButton;
	wi::gui::Button	generateWholeButton;
	wi::gui::Button	generateNavigationButton;
	wi::gui::CheckBox debugAllCheckBox;

	void ResizeLayout() override;
};

