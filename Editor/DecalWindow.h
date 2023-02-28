#pragma once
#include "WickedEngine.h"

class EditorComponent;

class DecalWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::CheckBox placementCheckBox;
	wi::gui::CheckBox onlyalphaCheckBox;
	wi::gui::Label infoLabel;

	void ResizeLayout() override;
};

