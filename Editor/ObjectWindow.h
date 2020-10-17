#pragma once
#include "WickedEngine.h"

class EditorComponent;

class ObjectWindow : public wiWindow
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor;
	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiLabel nameLabel;
	wiCheckBox renderableCheckBox;
	wiSlider ditherSlider;
	wiSlider cascadeMaskSlider;
	wiColorPicker colorPicker;

	wiLabel physicsLabel;
	wiCheckBox rigidBodyCheckBox;
	wiCheckBox disabledeactivationCheckBox;
	wiCheckBox kinematicCheckBox;
	wiComboBox collisionShapeComboBox;

	wiSlider lightmapResolutionSlider;
	wiComboBox lightmapSourceUVSetComboBox;
	wiButton generateLightmapButton;
	wiButton stopLightmapGenButton;
	wiButton clearLightmapButton;
};

