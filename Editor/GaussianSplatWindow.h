#pragma once

class EditorComponent;

class GaussianSplatWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;

	void ResizeLayout() override;
};

