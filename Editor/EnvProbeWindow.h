#pragma once
#include "WickedEngine.h"

class EditorComponent;

class EnvProbeWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::CheckBox realTimeCheckBox;
	wi::gui::CheckBox msaaCheckBox;
	wi::gui::Button generateButton;
	wi::gui::Button refreshButton;
	wi::gui::Button refreshAllButton;
};

