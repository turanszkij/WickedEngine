#pragma once

struct Material;
class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

struct Decal;

class DecalWindow
{
public:
	DecalWindow(wiGUI* gui);
	~DecalWindow();

	wiGUI* GUI;

	void SetDecal(Decal* decal);

	Decal* decal;

	wiWindow*	decalWindow;
	wiSlider*	opacitySlider;
	wiSlider*	emissiveSlider;
};

