#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;

class EditorComponent;

class IKWindow
{
public:
	IKWindow(EditorComponent* editor);
	~IKWindow();

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiGUI* GUI;

	wiWindow* window;

	wiButton* createButton;
	wiComboBox* targetCombo;
	wiCheckBox* disabledCheckBox;
	wiSlider* chainLengthSlider;
	wiSlider* iterationCountSlider;
};

