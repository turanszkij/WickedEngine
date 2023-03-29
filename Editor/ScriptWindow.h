#pragma once
class EditorComponent;

class ScriptWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Button fileButton;
	wi::gui::CheckBox playonceCheckBox;
	wi::gui::Button playstopButton;

	void Update(const wi::Canvas& canvas, float dt) override;
	void ResizeLayout() override;
};

