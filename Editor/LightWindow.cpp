#include "stdafx.h"
#include "LightWindow.h"
#include "Editor.h"

#include <string>

using namespace wi::ecs;
using namespace wi::graphics;
using namespace wi::scene;


void LightWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_POINTLIGHT " Light", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(650, 740));

	closeButton.SetTooltip("Delete LightComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().lights.Remove(entity);
		editor->GetCurrentScene().aabb_lights.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 130;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 130;


	float mod_x = 10;

	colorPicker.Create("Light Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(mod_x, y));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(false);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->color = args.color.toFloat3();
		}
		});
	AddWidget(&colorPicker);

	float mod_wid = colorPicker.GetScale().x;
	y += colorPicker.GetScale().y + 5;

	intensitySlider.Create(0, 1000, 0, 100000, "Intensity: ");
	intensitySlider.SetSize(XMFLOAT2(wid, hei));
	intensitySlider.SetPos(XMFLOAT2(x, y));
	intensitySlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->intensity = args.fValue;
		}
	});
	intensitySlider.SetEnabled(false);
	intensitySlider.SetTooltip("Brightness of light in. The units that this is defined in depend on the type of light. \nPoint and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2).");
	AddWidget(&intensitySlider);

	rangeSlider.Create(1, 1000, 0, 100000, "Range: ");
	rangeSlider.SetSize(XMFLOAT2(wid, hei));
	rangeSlider.SetPos(XMFLOAT2(x, y += step));
	rangeSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->range = args.fValue;
		}
	});
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Adjust the maximum range the light can affect.");
	AddWidget(&rangeSlider);

	outerConeAngleSlider.Create(0.1f, XM_PIDIV2 - 0.01f, 0, 100000, "Outer Cone Angle: ");
	outerConeAngleSlider.SetSize(XMFLOAT2(wid, hei));
	outerConeAngleSlider.SetPos(XMFLOAT2(x, y += step));
	outerConeAngleSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->outerConeAngle = args.fValue;
		}
	});
	outerConeAngleSlider.SetEnabled(false);
	outerConeAngleSlider.SetTooltip("Adjust the main cone aperture for spotlight.");
	AddWidget(&outerConeAngleSlider);

	innerConeAngleSlider.Create(0, XM_PI - 0.01f, 0, 100000, "Inner Cone Angle: ");
	innerConeAngleSlider.SetSize(XMFLOAT2(wid, hei));
	innerConeAngleSlider.SetPos(XMFLOAT2(x, y += step));
	innerConeAngleSlider.OnSlide([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->innerConeAngle = args.fValue;
		}
		});
	innerConeAngleSlider.SetEnabled(false);
	innerConeAngleSlider.SetTooltip("Adjust the inner cone aperture for spotlight.\n(The inner cone will always be inside the outer cone)");
	AddWidget(&innerConeAngleSlider);

	shadowCheckBox.Create("Shadow: ");
	shadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
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
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
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
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
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
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr)
		{
			light->SetStatic(args.bValue);
		}
	});
	staticCheckBox.SetEnabled(false);
	staticCheckBox.SetTooltip("Static lights will only be used for baking into lightmaps.");
	AddWidget(&staticCheckBox);

	typeSelectorComboBox.Create("Type: ");
	typeSelectorComboBox.SetSize(XMFLOAT2(wid, hei));
	typeSelectorComboBox.SetPos(XMFLOAT2(x, y += step));
	typeSelectorComboBox.OnSelect([&](wi::gui::EventArgs args) {
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light != nullptr && args.iValue >= 0)
		{
			light->SetType((LightComponent::LightType)args.iValue);
			SetLightType(light->GetType());
		}
	});
	typeSelectorComboBox.AddItem("Directional");
	typeSelectorComboBox.AddItem("Point");
	typeSelectorComboBox.AddItem("Spot");
	typeSelectorComboBox.SetTooltip("Choose the light source type...");
	typeSelectorComboBox.SetSelected((int)LightComponent::POINT);
	AddWidget(&typeSelectorComboBox);

	shadowResolutionComboBox.Create("Shadow resolution: ");
	shadowResolutionComboBox.SetTooltip("You can force a fixed resolution for this light's shadow map to avoid dynamic scaling.\nIf you leave it as dynamic, the resolution will be scaled between 0 and the max shadow resolution in the renderer for this light type, based on light's distance and size.");
	shadowResolutionComboBox.SetSize(XMFLOAT2(wid, hei));
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
		LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
		if (light == nullptr)
			return;
		light->forced_shadow_resolution = int(args.userdata);
		});
	shadowResolutionComboBox.SetSelected(0);
	AddWidget(&shadowResolutionComboBox);

	y += step * 0.5f;

	lensflare_Label.Create("Lens flare textures: ");
	lensflare_Label.SetPos(XMFLOAT2(mod_x, y += step));
	lensflare_Label.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&lensflare_Label);

	for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
	{
		lensflare_Button[i].Create("LensFlareSlot");
		lensflare_Button[i].SetText("");
		lensflare_Button[i].SetTooltip("Load a lensflare texture to this slot");
		lensflare_Button[i].SetPos(XMFLOAT2(mod_x, y += step));
		lensflare_Button[i].SetSize(XMFLOAT2(mod_wid, hei));
		lensflare_Button[i].OnClick([=](wi::gui::EventArgs args) {
			LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
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

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void LightWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);

	if (light != nullptr)
	{
		intensitySlider.SetEnabled(true);
		intensitySlider.SetValue(light->intensity);
		rangeSlider.SetValue(light->range);
		outerConeAngleSlider.SetValue(light->outerConeAngle);
		innerConeAngleSlider.SetValue(light->innerConeAngle);
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
		outerConeAngleSlider.SetEnabled(false);
		innerConeAngleSlider.SetEnabled(false);
		shadowCheckBox.SetEnabled(false);
		haloCheckBox.SetEnabled(false);
		volumetricsCheckBox.SetEnabled(false);
		staticCheckBox.SetEnabled(false);
		intensitySlider.SetEnabled(false);
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
		outerConeAngleSlider.SetEnabled(false);
		innerConeAngleSlider.SetEnabled(false);
	}
	else
	{
		rangeSlider.SetEnabled(true);
		if (type == LightComponent::SPOT)
		{
			outerConeAngleSlider.SetEnabled(true);
			innerConeAngleSlider.SetEnabled(true);
		}
		else
		{
			outerConeAngleSlider.SetEnabled(false);
			innerConeAngleSlider.SetEnabled(false);
		}
	}

}

void LightWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 140;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(colorPicker);
	add(typeSelectorComboBox);
	add(intensitySlider);
	add(rangeSlider);
	add(outerConeAngleSlider);
	add(innerConeAngleSlider);
	add_right(shadowCheckBox);
	add_right(haloCheckBox);
	add_right(volumetricsCheckBox);
	add_right(staticCheckBox);
	add(shadowResolutionComboBox);

	y += jump;

	add_fullwidth(lensflare_Label);
	add_fullwidth(lensflare_Button[0]);
	add_fullwidth(lensflare_Button[1]);
	add_fullwidth(lensflare_Button[2]);
	add_fullwidth(lensflare_Button[3]);
	add_fullwidth(lensflare_Button[4]);
	add_fullwidth(lensflare_Button[5]);
	add_fullwidth(lensflare_Button[6]);

}
