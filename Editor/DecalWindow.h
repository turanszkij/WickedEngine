#pragma once

namespace wiSceneComponents
{
	struct Decal;
}

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;

class DecalWindow
{
public:
	DecalWindow(wiGUI* gui);
	~DecalWindow();

	wiGUI* GUI;

	void SetDecal(wiSceneComponents::Decal* decal);

	wiSceneComponents::Decal* decal;

	wiWindow*	decalWindow;
	wiSlider*	opacitySlider;
	wiSlider*	emissiveSlider;
};

