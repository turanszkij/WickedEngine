#pragma once

struct Object;

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class ObjectWindow
{
public:
	ObjectWindow(wiGUI* gui);
	~ObjectWindow();

	void SetObject(Object* obj);

	Object* object;

	wiGUI* GUI;

	wiWindow*	objectWindow;

	wiSlider*	ditherSlider;
};

