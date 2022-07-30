#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialPickerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::vector<wi::gui::Button> buttons;

	wi::gui::Slider zoomSlider;

	void Update();

	// Recreating buttons shouldn't be done every frame because interaction states need multi-frame execution
	void RecreateButtons();
};

