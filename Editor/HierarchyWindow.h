#pragma once
#include "WickedEngine.h"

class EditorComponent;

class HierarchyWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox parentCombo;

	void ResizeLayout() override;
};

