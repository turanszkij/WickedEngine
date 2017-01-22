#include "stdafx.h"
#include "LightWindow.h"


LightWindow::LightWindow(wiGUI* gui) : GUI(gui), light(nullptr)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	lightWindow = new wiWindow(GUI, "Light Window");
	lightWindow->SetSize(XMFLOAT2(400, 420));
	//lightWindow->SetEnabled(false);
	GUI->AddWidget(lightWindow);

	float x = 200;
	float y = 0;
	float step = 35;

	energySlider = new wiSlider(0.1f, 64, 0, 100000, "Energy: ");
	energySlider->SetSize(XMFLOAT2(100, 30));
	energySlider->SetPos(XMFLOAT2(x, y += step));
	energySlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.x = args.fValue;
		}
	});
	energySlider->SetEnabled(false);
	energySlider->SetTooltip("Adjust the light radiation amount inside the maximum range");
	lightWindow->AddWidget(energySlider);

	distanceSlider = new wiSlider(1, 1000, 0, 100000, "Distance: ");
	distanceSlider->SetSize(XMFLOAT2(100, 30));
	distanceSlider->SetPos(XMFLOAT2(x, y += step));
	distanceSlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.y = args.fValue;
		}
	});
	distanceSlider->SetEnabled(false);
	distanceSlider->SetTooltip("Adjust the maximum range the light can affect.");
	lightWindow->AddWidget(distanceSlider);

	fovSlider = new wiSlider(0.1f, XM_PI - 0.01f, 0, 100000, "FOV: ");
	fovSlider->SetSize(XMFLOAT2(100, 30));
	fovSlider->SetPos(XMFLOAT2(x, y += step));
	fovSlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->enerDis.z = args.fValue;
		}
	});
	fovSlider->SetEnabled(false);
	fovSlider->SetTooltip("Adjust the cone aperture for spotlight.");
	lightWindow->AddWidget(fovSlider);

	biasSlider = new wiSlider(0.0f, 0.01f, 0, 100000, "ShadowBias: ");
	biasSlider->SetSize(XMFLOAT2(100, 30));
	biasSlider->SetPos(XMFLOAT2(x, y += step));
	biasSlider->OnSlide([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->shadowBias = args.fValue;
		}
	});
	biasSlider->SetEnabled(false);
	biasSlider->SetTooltip("Adjust the shadow bias if shadow artifacts occur.");
	lightWindow->AddWidget(biasSlider);

	shadowCheckBox = new wiCheckBox("Shadow: ");
	shadowCheckBox->SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox->OnClick([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->shadow = args.bValue;
		}
	});
	shadowCheckBox->SetEnabled(false);
	shadowCheckBox->SetTooltip("Set light as shadow caster. Many shadow casters can affect performance!");
	lightWindow->AddWidget(shadowCheckBox);

	haloCheckBox = new wiCheckBox("Halo: ");
	haloCheckBox->SetPos(XMFLOAT2(x, y += step));
	haloCheckBox->OnClick([&](wiEventArgs args) {
		if (light != nullptr)
		{
			light->noHalo = !args.bValue;
		}
	});
	haloCheckBox->SetEnabled(false);
	haloCheckBox->SetTooltip("Visualize light source emission");
	lightWindow->AddWidget(haloCheckBox);

	addLightButton = new wiButton("Add Light");
	addLightButton->SetPos(XMFLOAT2(x, y += step));
	addLightButton->SetSize(XMFLOAT2(180, 30));
	addLightButton->OnClick([&](wiEventArgs args) {
		Model* model = new Model;
		Light* light = new Light;
		light->enerDis = XMFLOAT4(2, 60, XM_PIDIV4, 0);
		light->color = XMFLOAT4(1, 1, 1, 1);
		light->type = Light::SPOT;
		model->lights.push_back(light);
		wiRenderer::AddModel(model);
	});
	addLightButton->SetTooltip("Add a light to the scene. It will be added to the origin.");
	lightWindow->AddWidget(addLightButton);


	colorPickerToggleButton = new wiButton("Color");
	colorPickerToggleButton->SetPos(XMFLOAT2(x, y += step));
	colorPickerToggleButton->OnClick([&](wiEventArgs args) {
		colorPicker->SetVisible(!colorPicker->IsVisible());
	});
	colorPickerToggleButton->SetTooltip("Toggle the light color picker.");
	lightWindow->AddWidget(colorPickerToggleButton);


	colorPicker = new wiColorPicker(GUI, "Light Color");
	colorPicker->SetVisible(false);
	colorPicker->SetEnabled(false);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		light->color = XMFLOAT4(powf(args.color.x, 1.f / 2.2f), powf(args.color.y, 1.f / 2.2f), powf(args.color.z, 1.f / 2.2f), 1);
	});
	GUI->AddWidget(colorPicker);

	typeSelectorComboBox = new wiComboBox("Type: ");
	typeSelectorComboBox->SetPos(XMFLOAT2(x, y += step));
	typeSelectorComboBox->OnSelect([&](wiEventArgs args) {
		if (light != nullptr && args.iValue >= 0)
		{
			light->type = (Light::LightType)args.iValue;
			SetLightType(light->type); // for the gui changes to apply to the new type
		}
	});
	typeSelectorComboBox->SetEnabled(false);
	typeSelectorComboBox->AddItem("Directional");
	typeSelectorComboBox->AddItem("Point");
	typeSelectorComboBox->AddItem("Spot");
	typeSelectorComboBox->SetTooltip("Choose the light source type...");
	lightWindow->AddWidget(typeSelectorComboBox);


	lightWindow->Translate(XMFLOAT3(30, 30, 0));
	lightWindow->SetVisible(false);

	SetLight(nullptr);
}


LightWindow::~LightWindow()
{
	SAFE_DELETE(lightWindow);
	SAFE_DELETE(energySlider);
	SAFE_DELETE(distanceSlider);
	SAFE_DELETE(fovSlider);
	SAFE_DELETE(biasSlider);
	SAFE_DELETE(shadowCheckBox);
	SAFE_DELETE(haloCheckBox);
	SAFE_DELETE(addLightButton);
	SAFE_DELETE(colorPickerToggleButton);
	SAFE_DELETE(colorPicker);
	SAFE_DELETE(typeSelectorComboBox);
}

void LightWindow::SetLight(Light* light)
{
	this->light = light;
	if (light != nullptr)
	{
		//lightWindow->SetEnabled(true);
		energySlider->SetEnabled(true);
		energySlider->SetValue(light->enerDis.x);
		distanceSlider->SetValue(light->enerDis.y);
		fovSlider->SetValue(light->enerDis.z);
		biasSlider->SetEnabled(true);
		biasSlider->SetValue(light->shadowBias);
		shadowCheckBox->SetEnabled(true);
		shadowCheckBox->SetCheck(light->shadow);
		haloCheckBox->SetEnabled(true);
		haloCheckBox->SetCheck(!light->noHalo);
		colorPicker->SetEnabled(true);
		typeSelectorComboBox->SetEnabled(true);
		typeSelectorComboBox->SetSelected((int)light->type);
		
		SetLightType(light->type);
	}
	else
	{
		distanceSlider->SetEnabled(false);
		fovSlider->SetEnabled(false);
		biasSlider->SetEnabled(false);
		shadowCheckBox->SetEnabled(false);
		haloCheckBox->SetEnabled(false);
		energySlider->SetEnabled(false);
		colorPicker->SetEnabled(false);
		typeSelectorComboBox->SetEnabled(false);
		//lightWindow->SetEnabled(false);
	}
}
void LightWindow::SetLightType(Light::LightType type)
{
	if (type == Light::DIRECTIONAL)
	{
		distanceSlider->SetEnabled(false);
		fovSlider->SetEnabled(false);
	}
	else
	{
		distanceSlider->SetEnabled(true);
		if (type == Light::SPOT)
		{
			fovSlider->SetEnabled(true);
		}
		else
		{
			fovSlider->SetEnabled(false);
		}
	}
}
