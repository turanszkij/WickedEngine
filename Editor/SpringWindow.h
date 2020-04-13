#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class EditorComponent;

class SpringWindow
{
public:
	SpringWindow(EditorComponent* editor);
	~SpringWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* window;

	wiButton* createButton;
	wiCheckBox* debugCheckBox;
	wiCheckBox* disabledCheckBox;
	wiCheckBox* stretchCheckBox;
	wiCheckBox* gravityCheckBox;
	wiSlider* stiffnessSlider;
	wiSlider* dampingSlider;
	wiSlider* windSlider;
};

