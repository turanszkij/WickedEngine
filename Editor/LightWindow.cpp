#include "stdafx.h"
#include "LightWindow.h"
#include "Editor.h"

#include <string>

using namespace wiECS;
using namespace wiGraphics;
using namespace wiScene;


void LightWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Light Window");
	SetSize(XMFLOAT2(650, 500));

	float x = 450;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	energySlider.Create(0.1f, 64, 0, 100000, "Energy: ");
	energySlider.SetSize(XMFLOAT2(100, hei));
	energySlider.SetPos(XMFLOAT2(x, y += step));
	energySlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->energy = args.fValue;
		}
	});
	energySlider.SetEnabled(false);
	energySlider.SetTooltip("Adjust the light radiation amount inside the maximum range");
	AddWidget(&energySlider);

	rangeSlider.Create(1, 1000, 0, 100000, "Range: ");
	rangeSlider.SetSize(XMFLOAT2(100, hei));
	rangeSlider.SetPos(XMFLOAT2(x, y += step));
	rangeSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->range_local = args.fValue;
		}
	});
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Adjust the maximum range the light can affect.");
	AddWidget(&rangeSlider);

	radiusSlider.Create(0.01f, 10, 0, 100000, "Radius: ");
	radiusSlider.SetSize(XMFLOAT2(100, hei));
	radiusSlider.SetPos(XMFLOAT2(x, y += step));
	radiusSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->radius = args.fValue;
		}
	});
	radiusSlider.SetEnabled(false);
	radiusSlider.SetTooltip("Adjust the radius of an area light.");
	AddWidget(&radiusSlider);

	widthSlider.Create(1, 10, 0, 100000, "Width: ");
	widthSlider.SetSize(XMFLOAT2(100, hei));
	widthSlider.SetPos(XMFLOAT2(x, y += step));
	widthSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->width = args.fValue;
		}
	});
	widthSlider.SetEnabled(false);
	widthSlider.SetTooltip("Adjust the width of an area light.");
	AddWidget(&widthSlider);

	heightSlider.Create(1, 10, 0, 100000, "Height: ");
	heightSlider.SetSize(XMFLOAT2(100, hei));
	heightSlider.SetPos(XMFLOAT2(x, y += step));
	heightSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->height = args.fValue;
		}
	});
	heightSlider.SetEnabled(false);
	heightSlider.SetTooltip("Adjust the height of an area light.");
	AddWidget(&heightSlider);

	fovSlider.Create(0.1f, XM_PI - 0.01f, 0, 100000, "FOV: ");
	fovSlider.SetSize(XMFLOAT2(100, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	fovSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->fov = args.fValue;
		}
	});
	fovSlider.SetEnabled(false);
	fovSlider.SetTooltip("Adjust the cone aperture for spotlight.");
	AddWidget(&fovSlider);

	biasSlider.Create(0.0f, 0.2f, 0, 100000, "ShadowBias: ");
	biasSlider.SetSize(XMFLOAT2(100, hei));
	biasSlider.SetPos(XMFLOAT2(x, y += step));
	biasSlider.OnSlide([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->shadowBias = args.fValue;
		}
	});
	biasSlider.SetEnabled(false);
	biasSlider.SetTooltip("Adjust the shadow bias if shadow artifacts occur.");
	AddWidget(&biasSlider);

	shadowCheckBox.Create("Shadow: ");
	shadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox.OnClick([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetCastShadow(args.bValue);
		}
	});
	shadowCheckBox.SetEnabled(false);
	shadowCheckBox.SetTooltip("Set light as shadow caster. Many shadow casters can affect performance!");
	AddWidget(&shadowCheckBox);

	volumetricsCheckBox.Create("Volumetric: ");
	volumetricsCheckBox.SetSize(XMFLOAT2(hei, hei));
	volumetricsCheckBox.SetPos(XMFLOAT2(x, y += step));
	volumetricsCheckBox.OnClick([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetVolumetricsEnabled(args.bValue);
		}
	});
	volumetricsCheckBox.SetEnabled(false);
	volumetricsCheckBox.SetTooltip("Compute volumetric light scattering effect. \nThe fog settings affect scattering (see Weather window). If there is no fog, there is no scattering.");
	AddWidget(&volumetricsCheckBox);

	haloCheckBox.Create("Visualizer: ");
	haloCheckBox.SetSize(XMFLOAT2(hei, hei));
	haloCheckBox.SetPos(XMFLOAT2(x, y += step));
	haloCheckBox.OnClick([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetVisualizerEnabled(args.bValue);
		}
	});
	haloCheckBox.SetEnabled(false);
	haloCheckBox.SetTooltip("Visualize light source emission");
	AddWidget(&haloCheckBox);

	staticCheckBox.Create("Static: ");
	staticCheckBox.SetSize(XMFLOAT2(hei, hei));
	staticCheckBox.SetPos(XMFLOAT2(x, y += step));
	staticCheckBox.OnClick([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetStatic(args.bValue);
		}
	});
	staticCheckBox.SetEnabled(false);
	staticCheckBox.SetTooltip("Static lights will only be used for baking into lightmaps.");
	AddWidget(&staticCheckBox);

	addLightButton.Create("Add Light");
	addLightButton.SetPos(XMFLOAT2(x, y += step));
	addLightButton.SetSize(XMFLOAT2(150, hei));
	addLightButton.OnClick([=](wiEventArgs args) {
		Entity entity = wiScene::GetScene().Entity_CreateLight("editorLight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->type = (LightComponent::LightType)typeSelectorComboBox.GetSelected();
			editor->ClearSelected();
			editor->AddSelected(entity);
			editor->RefreshSceneGraphView();
			SetEntity(entity);
		}
		else
		{
			assert(0);
		}
	});
	addLightButton.SetTooltip("Add a light to the scene.");
	AddWidget(&addLightButton);


	colorPicker.Create("Light Color", false);
	colorPicker.SetPos(XMFLOAT2(10, 30));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(false);
	colorPicker.OnColorChanged([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->color = args.color.toFloat3();
		}
	});
	AddWidget(&colorPicker);

	typeSelectorComboBox.Create("Type: ");
	typeSelectorComboBox.SetSize(XMFLOAT2(150, hei));
	typeSelectorComboBox.SetPos(XMFLOAT2(x, y += step));
	typeSelectorComboBox.OnSelect([&](wiEventArgs args) {
		LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
		if (light != nullptr && args.iValue >= 0)
		{
			light->SetType((LightComponent::LightType)args.iValue);
			SetLightType(light->GetType());
			biasSlider.SetValue(light->shadowBias);
		}
	});
	typeSelectorComboBox.AddItem("Directional");
	typeSelectorComboBox.AddItem("Point");
	typeSelectorComboBox.AddItem("Spot");
	typeSelectorComboBox.AddItem("Sphere");
	typeSelectorComboBox.AddItem("Disc");
	typeSelectorComboBox.AddItem("Rectangle");
	typeSelectorComboBox.AddItem("Tube");
	typeSelectorComboBox.SetTooltip("Choose the light source type...");
	typeSelectorComboBox.SetSelected((int)LightComponent::POINT);
	AddWidget(&typeSelectorComboBox);



	x = 10;
	y = 280;
	hei = 20;
	step = hei + 2;

	lensflare_Label.Create("Lens flare textures: ");
	lensflare_Label.SetPos(XMFLOAT2(x, y += step));
	lensflare_Label.SetSize(XMFLOAT2(140, hei));
	AddWidget(&lensflare_Label);

	for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
	{
		lensflare_Button[i].Create("LensFlareSlot");
		lensflare_Button[i].SetText("");
		lensflare_Button[i].SetTooltip("Load a lensflare texture to this slot");
		lensflare_Button[i].SetPos(XMFLOAT2(x, y += step));
		lensflare_Button[i].SetSize(XMFLOAT2(260, hei));
		lensflare_Button[i].OnClick([=](wiEventArgs args) {
			LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);
			if (light == nullptr)
				return;

			if (light->lensFlareRimTextures.size() <= i)
			{
				light->lensFlareRimTextures.resize(i + 1);
				light->lensFlareNames.resize(i + 1);
			}

			if (light->lensFlareRimTextures[i] != nullptr)
			{
				light->lensFlareNames[i] = "";
				light->lensFlareRimTextures[i] = nullptr;
				lensflare_Button[i].SetText("");
			}
			else
			{
				wiHelper::FileDialogParams params;
				params.type = wiHelper::FileDialogParams::OPEN;
				params.description = "Texture";
				params.extensions.push_back("dds");
				params.extensions.push_back("png");
				params.extensions.push_back("jpg");
				params.extensions.push_back("tga");
				wiHelper::FileDialog(params, [this, light, i](std::string fileName) {
					wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
						light->lensFlareRimTextures[i] = wiResourceManager::Load(fileName);
						light->lensFlareNames[i] = fileName;
						lensflare_Button[i].SetText(wiHelper::GetFileNameFromPath(fileName));
					});
				});
			}
		});
		AddWidget(&lensflare_Button[i]);
	}


	Translate(XMFLOAT3(120, 30, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void LightWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const LightComponent* light = wiScene::GetScene().lights.GetComponent(entity);

	if (light != nullptr)
	{
		energySlider.SetEnabled(true);
		energySlider.SetValue(light->energy);
		rangeSlider.SetValue(light->range_local);
		radiusSlider.SetValue(light->radius);
		widthSlider.SetValue(light->width);
		heightSlider.SetValue(light->height);
		fovSlider.SetValue(light->fov);
		biasSlider.SetEnabled(true);
		biasSlider.SetValue(light->shadowBias);
		shadowCheckBox.SetEnabled(true);
		shadowCheckBox.SetCheck(light->IsCastingShadow());
		haloCheckBox.SetEnabled(true);
		haloCheckBox.SetCheck(light->IsVisualizerEnabled());
		volumetricsCheckBox.SetEnabled(true);
		volumetricsCheckBox.SetCheck(light->IsVolumetricsEnabled());
		staticCheckBox.SetEnabled(true);
		staticCheckBox.SetCheck(light->IsStatic());
		colorPicker.SetEnabled(true);
		colorPicker.SetPickColor(wiColor::fromFloat3(light->color));
		typeSelectorComboBox.SetSelected((int)light->GetType());
		
		SetLightType(light->GetType());

		for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
		{
			if (light->lensFlareRimTextures.size() > i && light->lensFlareRimTextures[i] && !light->lensFlareNames[i].empty())
			{
				lensflare_Button[i].SetText(light->lensFlareNames[i]);
			}
			else
			{
				lensflare_Button[i].SetText("");
			}
			lensflare_Button[i].SetEnabled(true);
		}
	}
	else
	{
		rangeSlider.SetEnabled(false);
		radiusSlider.SetEnabled(false);
		widthSlider.SetEnabled(false);
		heightSlider.SetEnabled(false);
		fovSlider.SetEnabled(false);
		biasSlider.SetEnabled(false);
		shadowCheckBox.SetEnabled(false);
		haloCheckBox.SetEnabled(false);
		volumetricsCheckBox.SetEnabled(false);
		staticCheckBox.SetEnabled(false);
		energySlider.SetEnabled(false);
		colorPicker.SetEnabled(false);

		for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
		{
			lensflare_Button[i].SetEnabled(false);
		}
	}
}
void LightWindow::SetLightType(LightComponent::LightType type)
{
	if (type == LightComponent::DIRECTIONAL)
	{
		rangeSlider.SetEnabled(false);
		fovSlider.SetEnabled(false);
	}
	else
	{
		if (type == LightComponent::SPHERE || type == LightComponent::DISC || type == LightComponent::RECTANGLE || type == LightComponent::TUBE)
		{
			rangeSlider.SetEnabled(false);
			radiusSlider.SetEnabled(true);
			widthSlider.SetEnabled(true);
			heightSlider.SetEnabled(true);
			fovSlider.SetEnabled(false);
		}
		else
		{
			rangeSlider.SetEnabled(true);
			radiusSlider.SetEnabled(false);
			widthSlider.SetEnabled(false);
			heightSlider.SetEnabled(false);
			if (type == LightComponent::SPOT)
			{
				fovSlider.SetEnabled(true);
			}
			else
			{
				fovSlider.SetEnabled(false);
			}
		}
	}

}
