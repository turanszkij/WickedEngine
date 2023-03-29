#pragma once
class EditorComponent;

class EnvProbeWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::CheckBox realTimeCheckBox;
	wi::gui::CheckBox msaaCheckBox;
	wi::gui::Button refreshButton;
	wi::gui::Button refreshAllButton;

	void ResizeLayout() override;
};

