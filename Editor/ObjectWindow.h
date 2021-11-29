#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ObjectWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::widget::Label nameLabel;
	wi::widget::CheckBox renderableCheckBox;
	wi::widget::CheckBox shadowCheckBox;
	wi::widget::Slider ditherSlider;
	wi::widget::Slider cascadeMaskSlider;

	wi::widget::ComboBox colorComboBox;
	wi::widget::ColorPicker colorPicker;

	wi::widget::Label physicsLabel;
	wi::widget::ComboBox collisionShapeComboBox;
	wi::widget::Slider XSlider;
	wi::widget::Slider YSlider;
	wi::widget::Slider ZSlider;
	wi::widget::Slider massSlider;
	wi::widget::Slider frictionSlider;
	wi::widget::Slider restitutionSlider;
	wi::widget::Slider lineardampingSlider;
	wi::widget::Slider angulardampingSlider;
	wi::widget::CheckBox disabledeactivationCheckBox;
	wi::widget::CheckBox kinematicCheckBox;

	wi::widget::Slider lightmapResolutionSlider;
	wi::widget::ComboBox lightmapSourceUVSetComboBox;
	wi::widget::Button generateLightmapButton;
	wi::widget::Button stopLightmapGenButton;
	wi::widget::Button clearLightmapButton;
};

