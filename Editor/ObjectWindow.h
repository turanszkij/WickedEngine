#pragma once
class EditorComponent;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class ObjectWindow
{
public:
	ObjectWindow(EditorComponent* editor);
	~ObjectWindow();

	EditorComponent* editor;
	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow*	objectWindow;

	wiLabel*	nameLabel;
	wiCheckBox* renderableCheckBox;
	wiSlider*	ditherSlider;
	wiSlider*	cascadeMaskSlider;
	wiColorPicker* colorPicker;

	wiLabel*	physicsLabel;
	wiCheckBox*	rigidBodyCheckBox;
	wiCheckBox* disabledeactivationCheckBox;
	wiCheckBox* kinematicCheckBox;
	wiComboBox*	collisionShapeComboBox;

	wiSlider*	lightmapResolutionSlider;
	wiComboBox*	lightmapSourceUVSetComboBox;
	wiButton*	generateLightmapButton;
	wiButton*	stopLightmapGenButton;
	wiButton*	clearLightmapButton;
};

