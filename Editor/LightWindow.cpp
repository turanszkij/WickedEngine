#include "stdafx.h"
#include "LightWindow.h"

using namespace wiECS;
using namespace wiSceneSystem;


LightWindow::LightWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	lightWindow = new wiWindow(GUI, "Light Window");
	lightWindow->SetSize(XMFLOAT2(650, 520));
	//lightWindow->SetEnabled(false);
	GUI->AddWidget(lightWindow);

	float x = 450;
	float y = 0;
	float step = 35;

	energySlider = new wiSlider(0.1f, 64, 0, 100000, "Energy: ");
	energySlider->SetSize(XMFLOAT2(100, 30));
	energySlider->SetPos(XMFLOAT2(x, y += step));
	energySlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->energy = args.fValue;
		}
	});
	energySlider->SetEnabled(false);
	energySlider->SetTooltip("Adjust the light radiation amount inside the maximum range");
	lightWindow->AddWidget(energySlider);

	rangeSlider = new wiSlider(1, 1000, 0, 100000, "Range: ");
	rangeSlider->SetSize(XMFLOAT2(100, 30));
	rangeSlider->SetPos(XMFLOAT2(x, y += step));
	rangeSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->range_local = args.fValue;
		}
	});
	rangeSlider->SetEnabled(false);
	rangeSlider->SetTooltip("Adjust the maximum range the light can affect.");
	lightWindow->AddWidget(rangeSlider);

	radiusSlider = new wiSlider(0.01f, 10, 0, 100000, "Radius: ");
	radiusSlider->SetSize(XMFLOAT2(100, 30));
	radiusSlider->SetPos(XMFLOAT2(x, y += step));
	radiusSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->radius = args.fValue;
		}
	});
	radiusSlider->SetEnabled(false);
	radiusSlider->SetTooltip("Adjust the radius of an area light.");
	lightWindow->AddWidget(radiusSlider);

	widthSlider = new wiSlider(1, 10, 0, 100000, "Width: ");
	widthSlider->SetSize(XMFLOAT2(100, 30));
	widthSlider->SetPos(XMFLOAT2(x, y += step));
	widthSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->width = args.fValue;
		}
	});
	widthSlider->SetEnabled(false);
	widthSlider->SetTooltip("Adjust the width of an area light.");
	lightWindow->AddWidget(widthSlider);

	heightSlider = new wiSlider(1, 10, 0, 100000, "Height: ");
	heightSlider->SetSize(XMFLOAT2(100, 30));
	heightSlider->SetPos(XMFLOAT2(x, y += step));
	heightSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->height = args.fValue;
		}
	});
	heightSlider->SetEnabled(false);
	heightSlider->SetTooltip("Adjust the height of an area light.");
	lightWindow->AddWidget(heightSlider);

	fovSlider = new wiSlider(0.1f, XM_PI - 0.01f, 0, 100000, "FOV: ");
	fovSlider->SetSize(XMFLOAT2(100, 30));
	fovSlider->SetPos(XMFLOAT2(x, y += step));
	fovSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->fov = args.fValue;
		}
	});
	fovSlider->SetEnabled(false);
	fovSlider->SetTooltip("Adjust the cone aperture for spotlight.");
	lightWindow->AddWidget(fovSlider);

	biasSlider = new wiSlider(0.0f, 0.2f, 0, 100000, "ShadowBias: ");
	biasSlider->SetSize(XMFLOAT2(100, 30));
	biasSlider->SetPos(XMFLOAT2(x, y += step));
	biasSlider->OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
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
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetCastShadow(args.bValue);
		}
	});
	shadowCheckBox->SetEnabled(false);
	shadowCheckBox->SetTooltip("Set light as shadow caster. Many shadow casters can affect performance!");
	lightWindow->AddWidget(shadowCheckBox);

	volumetricsCheckBox = new wiCheckBox("Volumetric Scattering: ");
	volumetricsCheckBox->SetPos(XMFLOAT2(x, y += step));
	volumetricsCheckBox->OnClick([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetVolumetricsEnabled(args.bValue);
		}
	});
	volumetricsCheckBox->SetEnabled(false);
	volumetricsCheckBox->SetTooltip("Compute volumetric light scattering effect. The scattering is modulated by fog settings!");
	lightWindow->AddWidget(volumetricsCheckBox);

	haloCheckBox = new wiCheckBox("Visualizer: ");
	haloCheckBox->SetPos(XMFLOAT2(x, y += step));
	haloCheckBox->OnClick([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetVisualizerEnabled(args.bValue);
		}
	});
	haloCheckBox->SetEnabled(false);
	haloCheckBox->SetTooltip("Visualize light source emission");
	lightWindow->AddWidget(haloCheckBox);

	staticCheckBox = new wiCheckBox("Static: ");
	staticCheckBox->SetPos(XMFLOAT2(x, y += step));
	staticCheckBox->OnClick([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetStatic(args.bValue);
		}
	});
	staticCheckBox->SetEnabled(false);
	staticCheckBox->SetTooltip("Static lights will only be used for baking into lightmaps.");
	lightWindow->AddWidget(staticCheckBox);

	addLightButton = new wiButton("Add Light");
	addLightButton->SetPos(XMFLOAT2(x, y += step));
	addLightButton->SetSize(XMFLOAT2(150, 30));
	addLightButton->OnClick([&](wiEventArgs args) {
		wiSceneSystem::GetScene().Entity_CreateLight("editorLight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
	});
	addLightButton->SetTooltip("Add a light to the scene.");
	lightWindow->AddWidget(addLightButton);


	colorPicker = new wiColorPicker(GUI, "Light Color");
	colorPicker->SetPos(XMFLOAT2(10, 30));
	colorPicker->RemoveWidgets();
	colorPicker->SetVisible(true);
	colorPicker->SetEnabled(true);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->color = args.color.toFloat3();
		}
	});
	lightWindow->AddWidget(colorPicker);

	typeSelectorComboBox = new wiComboBox("Type: ");
	typeSelectorComboBox->SetPos(XMFLOAT2(x, y += step));
	typeSelectorComboBox->OnSelect([&](wiEventArgs args) {
		LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);
		if (light != nullptr && args.iValue >= 0)
		{
			light->SetType((LightComponent::LightType)args.iValue);
			SetLightType(light->GetType());
			biasSlider->SetValue(light->shadowBias);
		}
	});
	typeSelectorComboBox->SetEnabled(false);
	typeSelectorComboBox->AddItem("Directional");
	typeSelectorComboBox->AddItem("Point");
	typeSelectorComboBox->AddItem("Spot");
	typeSelectorComboBox->AddItem("Sphere");
	typeSelectorComboBox->AddItem("Disc");
	typeSelectorComboBox->AddItem("Rectangle");
	typeSelectorComboBox->AddItem("Tube");
	typeSelectorComboBox->SetTooltip("Choose the light source type...");
	lightWindow->AddWidget(typeSelectorComboBox);


	lightWindow->Translate(XMFLOAT3(120, 30, 0));
	lightWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


LightWindow::~LightWindow()
{
	lightWindow->RemoveWidgets(true);
	GUI->RemoveWidget(lightWindow);
	SAFE_DELETE(lightWindow);
}

void LightWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	const LightComponent* light = wiSceneSystem::GetScene().lights.GetComponent(entity);

	if (light != nullptr)
	{
		//lightWindow->SetEnabled(true);
		energySlider->SetEnabled(true);
		energySlider->SetValue(light->energy);
		rangeSlider->SetValue(light->range_local);
		radiusSlider->SetValue(light->radius);
		widthSlider->SetValue(light->width);
		heightSlider->SetValue(light->height);
		fovSlider->SetValue(light->fov);
		biasSlider->SetEnabled(true);
		biasSlider->SetValue(light->shadowBias);
		shadowCheckBox->SetEnabled(true);
		shadowCheckBox->SetCheck(light->IsCastingShadow());
		haloCheckBox->SetEnabled(true);
		haloCheckBox->SetCheck(light->IsVisualizerEnabled());
		volumetricsCheckBox->SetEnabled(true);
		volumetricsCheckBox->SetCheck(light->IsVolumetricsEnabled());
		staticCheckBox->SetEnabled(true);
		staticCheckBox->SetCheck(light->IsStatic());
		colorPicker->SetEnabled(true);
		typeSelectorComboBox->SetEnabled(true);
		typeSelectorComboBox->SetSelected((int)light->GetType());
		
		SetLightType(light->GetType());
	}
	else
	{
		rangeSlider->SetEnabled(false);
		radiusSlider->SetEnabled(false);
		widthSlider->SetEnabled(false);
		heightSlider->SetEnabled(false);
		fovSlider->SetEnabled(false);
		biasSlider->SetEnabled(false);
		shadowCheckBox->SetEnabled(false);
		haloCheckBox->SetEnabled(false);
		volumetricsCheckBox->SetEnabled(false);
		staticCheckBox->SetEnabled(false);
		energySlider->SetEnabled(false);
		colorPicker->SetEnabled(false);
		typeSelectorComboBox->SetEnabled(false);
		//lightWindow->SetEnabled(false);
	}
}
void LightWindow::SetLightType(LightComponent::LightType type)
{
	if (type == LightComponent::DIRECTIONAL)
	{
		rangeSlider->SetEnabled(false);
		fovSlider->SetEnabled(false);
	}
	else
	{
		if (type == LightComponent::SPHERE || type == LightComponent::DISC || type == LightComponent::RECTANGLE || type == LightComponent::TUBE)
		{
			rangeSlider->SetEnabled(false);
			radiusSlider->SetEnabled(true);
			widthSlider->SetEnabled(true);
			heightSlider->SetEnabled(true);
			fovSlider->SetEnabled(false);
		}
		else
		{
			rangeSlider->SetEnabled(true);
			radiusSlider->SetEnabled(false);
			widthSlider->SetEnabled(false);
			heightSlider->SetEnabled(false);
			if (type == LightComponent::SPOT)
			{
				fovSlider->SetEnabled(true);
			}
			else
			{
				fovSlider->SetEnabled(false);
			}
		}
	}

}
