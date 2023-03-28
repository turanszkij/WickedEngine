#pragma once
class EditorComponent;

class LayerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label label;
	wi::gui::CheckBox layers[32];
	wi::gui::Button enableAllButton;
	wi::gui::Button enableNoneButton;

	void ResizeLayout() override;
};

