#pragma once
#include "CommonInclude.h"

class wiGUI;
class wiWindow;
class wiLabel;
class wiCheckBox;
class wiSlider;
class wiComboBox;
class wiColorPicker;
class wiButton;

class PaintToolWindow
{
public:
	PaintToolWindow(wiGUI* gui);
	~PaintToolWindow();

	wiGUI* GUI;

	wiWindow* window;
	wiComboBox* modeComboBox;
	wiSlider* radiusSlider;

	void DrawBrush() const;

	XMFLOAT2 pos = XMFLOAT2(0, 0);

	enum MODE
	{
		MODE_DISABLED,
		MODE_SOFTBODY_PINNING,
		MODE_SOFTBODY_PHYSICS,
		MODE_HAIRPARTICLE_ADD_TRIANGLE,
		MODE_HAIRPARTICLE_REMOVE_TRIANGLE,
	};
	MODE GetMode() const;
	float GetRadius() const;
};
