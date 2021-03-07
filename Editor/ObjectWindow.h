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
	wiCheckBox shadowCheckBox;
	wiSlider ditherSlider;
	wiSlider cascadeMaskSlider;

	wiComboBox colorComboBox;
	wiColorPicker colorPicker;

	wiLabel physicsLabel;
	wiComboBox collisionShapeComboBox;
	wiSlider XSlider;
	wiSlider YSlider;
	wiSlider ZSlider;
	wiSlider massSlider;
	wiSlider frictionSlider;
	wiSlider restitutionSlider;
	wiSlider lineardampingSlider;
	wiSlider angulardampingSlider;
	wiCheckBox disabledeactivationCheckBox;
	wiCheckBox kinematicCheckBox;

	wiSlider lightmapResolutionSlider;
	wiComboBox lightmapSourceUVSetComboBox;
	wiButton generateLightmapButton;
	wiButton stopLightmapGenButton;
	wiButton clearLightmapButton;
};

