#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class WorldWindow
{
public:
	WorldWindow(wiGUI* gui);
	~WorldWindow();

	void UpdateFromRenderer();

	wiGUI* GUI;

	wiWindow* worldWindow;
	wiSlider* fogStartSlider;
	wiSlider* fogEndSlider;
	wiSlider* fogHeightSlider;
};

