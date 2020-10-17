#pragma once
#include "WickedEngine.h"

class EditorComponent;

class LightWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	void SetLightType(wiScene::LightComponent::LightType type);

	wiSlider energySlider;
	wiSlider rangeSlider;
	wiSlider radiusSlider;
	wiSlider widthSlider;
	wiSlider heightSlider;
	wiSlider fovSlider;
	wiSlider biasSlider;
	wiCheckBox	shadowCheckBox;
	wiCheckBox	haloCheckBox;
	wiCheckBox	volumetricsCheckBox;
	wiCheckBox	staticCheckBox;
	wiButton addLightButton;
	wiColorPicker colorPicker;
	wiComboBox typeSelectorComboBox;

	wiLabel lensflare_Label;
	wiButton lensflare_Button[7];
};

