#include "stdafx.h"
#include "LightWindow.h"
#include "Editor.h"

#include <string>

using namespace wi::ecs;
using namespace wi::graphics;
using namespace wi::scene;


void LightWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Light Window");
	SetSize(XMFLOAT2(650, 300));

	float x = 450;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	energySlider.Create(0.1f, 64, 0, 100000, "Energy: ");
	energySlider.SetSize(XMFLOAT2(100, hei));
	energySlider.SetPos(XMFLOAT2(x, y));
	energySlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
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
	rangeSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->range = args.fValue;
		}
	});
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Adjust the maximum range the light can affect.");
	AddWidget(&rangeSlider);

	//radiusSlider.Create(0.01f, 10, 0, 100000, "Radius: ");
	//radiusSlider.SetSize(XMFLOAT2(100, hei));
	//radiusSlider.SetPos(XMFLOAT2(x, y += step));
	//radiusSlider.OnSlide([&](wi::gui::EventArgs args) {
	//	LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
	//	if (light != nullptr)
	//	{
	//		light->radius = args.fValue;
	//	}
	//});
	//radiusSlider.SetEnabled(false);
	//radiusSlider.SetTooltip("Adjust the radius of an area light.");
	//AddWidget(&radiusSlider);

	//widthSlider.Create(1, 10, 0, 100000, "Width: ");
	//widthSlider.SetSize(XMFLOAT2(100, hei));
	//widthSlider.SetPos(XMFLOAT2(x, y += step));
	//widthSlider.OnSlide([&](wi::gui::EventArgs args) {
	//	LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
	//	if (light != nullptr)
	//	{
	//		light->width = args.fValue;
	//	}
	//});
	//widthSlider.SetEnabled(false);
	//widthSlider.SetTooltip("Adjust the width of an area light.");
	//AddWidget(&widthSlider);

	//heightSlider.Create(1, 10, 0, 100000, "Height: ");
	//heightSlider.SetSize(XMFLOAT2(100, hei));
	//heightSlider.SetPos(XMFLOAT2(x, y += step));
	//heightSlider.OnSlide([&](wi::gui::EventArgs args) {
	//	LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
	//	if (light != nullptr)
	//	{
	//		light->height = args.fValue;
	//	}
	//});
	//heightSlider.SetEnabled(false);
	//heightSlider.SetTooltip("Adjust the height of an area light.");
	//AddWidget(&heightSlider);

	fovSlider.Create(0.1f, XM_PI - 0.01f, 0, 100000, "FOV: ");
	fovSlider.SetSize(XMFLOAT2(100, hei));
	fovSlider.SetPos(XMFLOAT2(x, y += step));
	fovSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->fov = args.fValue;
		}
	});
	fovSlider.SetEnabled(false);
	fovSlider.SetTooltip("Adjust the cone aperture for spotlight.");
	AddWidget(&fovSlider);

	fovInnerSlider.Create(0, XM_PI - 0.01f, 0, 100000, "FOV (inner): ");
	fovInnerSlider.SetSize(XMFLOAT2(100, hei));
	fovInnerSlider.SetPos(XMFLOAT2(x, y += step));
	fovInnerSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->fov_inner = args.fValue;
		}
		});
	fovInnerSlider.SetEnabled(false);
	fovInnerSlider.SetTooltip("Adjust the inner cone aperture for spotlight.\n(The inner cone will always be inside the outer/main cone)");
	AddWidget(&fovInnerSlider);

	shadowCheckBox.Create("Shadow: ");
	shadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
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
	volumetricsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
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
	haloCheckBox.OnClick([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
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
	staticCheckBox.OnClick([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
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
	addLightButton.OnClick([=](wi::gui::EventArgs args) {
		Entity entity = wi::scene::GetScene().Entity_CreateLight("editorLight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->type = (LightComponent::LightType)typeSelectorComboBox.GetSelected();

			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			editor->RecordSelection(archive);

			editor->ClearSelected();
			editor->AddSelected(entity);

			editor->RecordSelection(archive);
			editor->RecordAddedEntity(archive, entity);

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
	colorPicker.SetPos(XMFLOAT2(10, 0));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(false);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->color = args.color.toFloat3();
		}
	});
	AddWidget(&colorPicker);

	typeSelectorComboBox.Create("Type: ");
	typeSelectorComboBox.SetSize(XMFLOAT2(150, hei));
	typeSelectorComboBox.SetPos(XMFLOAT2(x, y += step));
	typeSelectorComboBox.OnSelect([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light != nullptr && args.iValue >= 0)
		{
			light->SetType((LightComponent::LightType)args.iValue);
			SetLightType(light->GetType());
		}
	});
	typeSelectorComboBox.AddItem("Directional");
	typeSelectorComboBox.AddItem("Point");
	typeSelectorComboBox.AddItem("Spot");
	//typeSelectorComboBox.AddItem("Sphere");
	//typeSelectorComboBox.AddItem("Disc");
	//typeSelectorComboBox.AddItem("Rectangle");
	//typeSelectorComboBox.AddItem("Tube");
	typeSelectorComboBox.SetTooltip("Choose the light source type...");
	typeSelectorComboBox.SetSelected((int)LightComponent::POINT);
	AddWidget(&typeSelectorComboBox);

	shadowResolutionComboBox.Create("Shadow resolution: ");
	shadowResolutionComboBox.SetTooltip("You can force a fixed resolution for this light's shadow map to avoid dynamic scaling.\nIf you leave it as dynamic, the resolution will be scaled between 0 and the max shadow resolution in the renderer for this light type, based on light's distance and size.");
	shadowResolutionComboBox.SetSize(XMFLOAT2(150, hei));
	shadowResolutionComboBox.SetPos(XMFLOAT2(x, y += step));
	shadowResolutionComboBox.AddItem("Dynamic", uint64_t(-1));
	shadowResolutionComboBox.AddItem("32", 32);
	shadowResolutionComboBox.AddItem("64", 64);
	shadowResolutionComboBox.AddItem("128", 128);
	shadowResolutionComboBox.AddItem("256", 256);
	shadowResolutionComboBox.AddItem("512", 512);
	shadowResolutionComboBox.AddItem("1024", 1024);
	shadowResolutionComboBox.AddItem("2048", 2048);
	shadowResolutionComboBox.OnSelect([&](wi::gui::EventArgs args) {
		LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
		if (light == nullptr)
			return;
		light->forced_shadow_resolution = int(args.userdata);
		});
	shadowResolutionComboBox.SetSelected(0);
	AddWidget(&shadowResolutionComboBox);

	y += step;
	x -= 100;

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
		lensflare_Button[i].OnClick([=](wi::gui::EventArgs args) {
			LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);
			if (light == nullptr)
				return;

			if (light->lensFlareRimTextures.size() <= i)
			{
				light->lensFlareRimTextures.resize(i + 1);
				light->lensFlareNames.resize(i + 1);
			}

			if (light->lensFlareRimTextures[i].IsValid())
			{
				light->lensFlareNames[i] = "";
				light->lensFlareRimTextures[i] = {};
				lensflare_Button[i].SetText("");
			}
			else
			{
				wi::helper::FileDialogParams params;
				params.type = wi::helper::FileDialogParams::OPEN;
				params.description = "Texture";
				params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
				wi::helper::FileDialog(params, [this, light, i](std::string fileName) {
					wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
						light->lensFlareRimTextures[i] = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
						light->lensFlareNames[i] = fileName;
						lensflare_Button[i].SetText(wi::helper::GetFileNameFromPath(fileName));
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

	const LightComponent* light = wi::scene::GetScene().lights.GetComponent(entity);

	if (light != nullptr)
	{
		energySlider.SetEnabled(true);
		energySlider.SetValue(light->energy);
		rangeSlider.SetValue(light->range);
		//radiusSlider.SetValue(light->radius);
		//widthSlider.SetValue(light->width);
		//heightSlider.SetValue(light->height);
		fovSlider.SetValue(light->fov);
		fovInnerSlider.SetValue(light->fov_inner);
		shadowCheckBox.SetEnabled(true);
		shadowCheckBox.SetCheck(light->IsCastingShadow());
		haloCheckBox.SetEnabled(true);
		haloCheckBox.SetCheck(light->IsVisualizerEnabled());
		volumetricsCheckBox.SetEnabled(true);
		volumetricsCheckBox.SetCheck(light->IsVolumetricsEnabled());
		staticCheckBox.SetEnabled(true);
		staticCheckBox.SetCheck(light->IsStatic());
		colorPicker.SetEnabled(true);
		colorPicker.SetPickColor(wi::Color::fromFloat3(light->color));
		typeSelectorComboBox.SetSelected((int)light->GetType());
		shadowResolutionComboBox.SetSelectedByUserdataWithoutCallback(uint64_t(light->forced_shadow_resolution));
		shadowResolutionComboBox.SetEnabled(true);
		
		SetLightType(light->GetType());

		for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
		{
			if (light->lensFlareRimTextures.size() > i && light->lensFlareRimTextures[i].IsValid() && !light->lensFlareNames[i].empty())
			{
				lensflare_Button[i].SetText(wi::helper::GetFileNameFromPath(light->lensFlareNames[i]));
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
		fovInnerSlider.SetEnabled(false);
		shadowCheckBox.SetEnabled(false);
		haloCheckBox.SetEnabled(false);
		volumetricsCheckBox.SetEnabled(false);
		staticCheckBox.SetEnabled(false);
		energySlider.SetEnabled(false);
		colorPicker.SetEnabled(false);
		shadowResolutionComboBox.SetEnabled(false);

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
		fovInnerSlider.SetEnabled(false);
	}
	else
	{
		//if (type == LightComponent::SPHERE || type == LightComponent::DISC || type == LightComponent::RECTANGLE || type == LightComponent::TUBE)
		//{
		//	rangeSlider.SetEnabled(false);
		//	radiusSlider.SetEnabled(true);
		//	widthSlider.SetEnabled(true);
		//	heightSlider.SetEnabled(true);
		//	fovSlider.SetEnabled(false);
		//}
		//else
		{
			rangeSlider.SetEnabled(true);
			radiusSlider.SetEnabled(false);
			widthSlider.SetEnabled(false);
			heightSlider.SetEnabled(false);
			if (type == LightComponent::SPOT)
			{
				fovSlider.SetEnabled(true);
				fovInnerSlider.SetEnabled(true);
			}
			else
			{
				fovSlider.SetEnabled(false);
				fovInnerSlider.SetEnabled(false);
			}
		}
	}

}
