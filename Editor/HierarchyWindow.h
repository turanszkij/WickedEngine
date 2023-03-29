#pragma once
class EditorComponent;

class HierarchyWindow : public wi::gui::Window
{
private:
	wi::unordered_set<wi::ecs::Entity> entities;
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox parentCombo;

	void ResizeLayout() override;
};

