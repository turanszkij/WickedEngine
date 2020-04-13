#pragma once

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class EditorComponent;

class EnvProbeWindow
{
public:
	EnvProbeWindow(EditorComponent* editor);
	~EnvProbeWindow();

	wiGUI* GUI;

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiWindow*	envProbeWindow;

	wiCheckBox*	realTimeCheckBox;
	wiButton*	generateButton;
	wiButton*	refreshButton;
	wiButton*	refreshAllButton;
};

