#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ObjectWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label nameLabel;
	wi::gui::CheckBox renderableCheckBox;
	wi::gui::CheckBox shadowCheckBox;
	wi::gui::Slider ditherSlider;
	wi::gui::Slider cascadeMaskSlider;

	wi::gui::ComboBox colorComboBox;
	wi::gui::ColorPicker colorPicker;

	wi::gui::Label physicsLabel;
	wi::gui::ComboBox collisionShapeComboBox;
	wi::gui::Slider XSlider;
	wi::gui::Slider YSlider;
	wi::gui::Slider ZSlider;
	wi::gui::Slider massSlider;
	wi::gui::Slider frictionSlider;
	wi::gui::Slider restitutionSlider;
	wi::gui::Slider lineardampingSlider;
	wi::gui::Slider angulardampingSlider;
	wi::gui::CheckBox disabledeactivationCheckBox;
	wi::gui::CheckBox kinematicCheckBox;

	wi::gui::Slider lightmapResolutionSlider;
	wi::gui::ComboBox lightmapSourceUVSetComboBox;
	wi::gui::Button generateLightmapButton;
	wi::gui::Button stopLightmapGenButton;
	wi::gui::Button clearLightmapButton;
};

