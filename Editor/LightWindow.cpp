#include "stdafx.h"
#include "LightWindow.h"

using namespace wi::ecs;
using namespace wi::graphics;
using namespace wi::scene;

void LightWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_POINTLIGHT " Light", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(650, 940));

	closeButton.SetTooltip("Delete LightComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().lights.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 130;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 130;


	float mod_x = 10;

	auto forEachSelected = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				LightComponent* light = scene.lights.GetComponent(x.entity);
				if (light != nullptr) func(light, args);
			}
		};
	};

	colorPicker.Create("Light Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(mod_x, y));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(false);
	colorPicker.OnColorChanged(forEachSelected([] (auto light, auto args) {
		light->color = args.color.toFloat3();
	}));
	AddWidget(&colorPicker);

	float mod_wid = colorPicker.GetScale().x;
	y += colorPicker.GetScale().y + 5;

	intensitySlider.Create(0, 1000, 0, 100000, "Intensity: ");
	intensitySlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->intensity = args.fValue;
	}));
	intensitySlider.SetEnabled(false);
	intensitySlider.SetTooltip("Brightness of light in. The units that this is defined in depend on the type of light. \nPoint and spot lights use luminous intensity in candela (lm/sr) while directional lights use illuminance in lux (lm/m2).");
	AddWidget(&intensitySlider);

	rangeSlider.Create(1, 1000, 0, 100000, "Range: ");
	rangeSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->range = args.fValue;
	}));
	rangeSlider.SetEnabled(false);
	rangeSlider.SetTooltip("Adjust the maximum range the light can affect.");
	AddWidget(&rangeSlider);

	radiusSlider.Create(0, 10, 0, 100000, "Radius: ");
	radiusSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->radius = args.fValue;
	}));
	radiusSlider.SetEnabled(false);
	radiusSlider.SetTooltip("[Experimental] Adjust the radius of the light source.\nFor directional light, this will only affect ray traced shadow softness.");
	AddWidget(&radiusSlider);

	lengthSlider.Create(0, 10, 0, 100000, "Length: ");
	lengthSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->length = args.fValue;
	}));
	lengthSlider.SetEnabled(false);
	lengthSlider.SetTooltip("Adjust the length of the light source.\nWith this you can make capsule light out of a point light.");
	AddWidget(&lengthSlider);

	heightSlider.Create(0, 10, 0, 100000, "Height: ");
	heightSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->height = args.fValue;
	}));
	heightSlider.SetEnabled(false);
	heightSlider.SetTooltip("Adjust the height of the rectangle light source.\n");
	AddWidget(&heightSlider);

	outerConeAngleSlider.Create(0.1f, XM_PIDIV2 - 0.01f, 0, 100000, "Outer Cone Angle: ");
	outerConeAngleSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->outerConeAngle = args.fValue;
	}));
	outerConeAngleSlider.SetEnabled(false);
	outerConeAngleSlider.SetTooltip("Adjust the main cone aperture for spotlight.");
	AddWidget(&outerConeAngleSlider);

	innerConeAngleSlider.Create(0, XM_PI - 0.01f, 0, 100000, "Inner Cone Angle: ");
	innerConeAngleSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->innerConeAngle = args.fValue;
	}));
	innerConeAngleSlider.SetEnabled(false);
	innerConeAngleSlider.SetTooltip("Adjust the inner cone aperture for spotlight.\n(The inner cone will always be inside the outer cone)");
	AddWidget(&innerConeAngleSlider);

	volumetricBoostSlider.Create(0, 10, 0, 1000, "Volumetric boost: ");
	volumetricBoostSlider.OnSlide(forEachSelected([] (auto light, auto args) {
		light->volumetric_boost = args.fValue;
	}));
	volumetricBoostSlider.SetTooltip("Adjust the volumetric fog effect's strength just for this light");
	AddWidget(&volumetricBoostSlider);

	shadowCheckBox.Create("Shadow: ");
	shadowCheckBox.OnClick(forEachSelected([] (auto light, auto args) {
		light->SetCastShadow(args.bValue);
	}));
	shadowCheckBox.SetEnabled(false);
	shadowCheckBox.SetTooltip("Set light as shadow caster. Many shadow casters can affect performance!");
	AddWidget(&shadowCheckBox);

	volumetricsCheckBox.Create("Volumetric: ");
	volumetricsCheckBox.OnClick(forEachSelected([] (auto light, auto args) {
		light->SetVolumetricsEnabled(args.bValue);
	}));
	volumetricsCheckBox.SetEnabled(false);
	volumetricsCheckBox.SetTooltip("Compute volumetric light scattering effect. \nThe fog settings affect scattering (see Weather window). If there is no fog, there is no scattering.");
	AddWidget(&volumetricsCheckBox);

	haloCheckBox.Create("Visualizer: ");
	haloCheckBox.OnClick(forEachSelected([] (auto light, auto args) {
		light->SetVisualizerEnabled(args.bValue);
	}));
	haloCheckBox.SetEnabled(false);
	haloCheckBox.SetTooltip("Visualize light source emission");
	AddWidget(&haloCheckBox);

	staticCheckBox.Create("Static: ");
	staticCheckBox.OnClick(forEachSelected([] (auto light, auto args) {
		light->SetStatic(args.bValue);
	}));
	staticCheckBox.SetEnabled(false);
	staticCheckBox.SetTooltip("Static lights will only be used for baking into lightmaps, DDGI and Surfel GI.");
	AddWidget(&staticCheckBox);

	volumetricCloudsCheckBox.Create("Volumetric Clouds: ");
	volumetricCloudsCheckBox.OnClick(forEachSelected([] (auto light, auto args) {
		light->SetVolumetricCloudsEnabled(args.bValue);
	}));
	volumetricCloudsCheckBox.SetEnabled(false);
	volumetricCloudsCheckBox.SetTooltip("When enabled light emission will affect volumetric clouds.");
	AddWidget(&volumetricCloudsCheckBox);

	typeSelectorComboBox.Create("Type: ");
	typeSelectorComboBox.OnSelect([=](wi::gui::EventArgs args) {
		if (args.iValue < 0)
			return;
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			LightComponent* light = scene.lights.GetComponent(x.entity);
			if (light != nullptr)
			{
			light->SetType((LightComponent::LightType)args.iValue);
				SetLightType(light->GetType());
			}
		}
	});
	typeSelectorComboBox.AddItem("Directional");
	typeSelectorComboBox.AddItem("Point");
	typeSelectorComboBox.AddItem("Spot");
	typeSelectorComboBox.AddItem("Rectangle");
	typeSelectorComboBox.SetTooltip("Choose the light source type...");
	typeSelectorComboBox.SetSelected((int)LightComponent::POINT);
	AddWidget(&typeSelectorComboBox);

	shadowResolutionComboBox.Create("Shadow resolution: ");
	shadowResolutionComboBox.SetTooltip("You can force a fixed resolution for this light's shadow map to avoid dynamic scaling.\nIf you leave it as dynamic, the resolution will be scaled between 0 and the max shadow resolution in the renderer for this light type, based on light's distance and size.");
	shadowResolutionComboBox.AddItem("Dynamic", uint64_t(-1));
	shadowResolutionComboBox.AddItem("32", 32);
	shadowResolutionComboBox.AddItem("64", 64);
	shadowResolutionComboBox.AddItem("128", 128);
	shadowResolutionComboBox.AddItem("256", 256);
	shadowResolutionComboBox.AddItem("512", 512);
	shadowResolutionComboBox.AddItem("1024", 1024);
	shadowResolutionComboBox.AddItem("2048", 2048);
	shadowResolutionComboBox.AddItem("4096", 4096);
	shadowResolutionComboBox.AddItem("8192", 8192);
	shadowResolutionComboBox.OnSelect(forEachSelected([] (auto light, auto args) {
		light->forced_shadow_resolution = int(args.userdata);
	}));
	shadowResolutionComboBox.SetSelected(0);
	AddWidget(&shadowResolutionComboBox);


	cameraComboBox.Create("Camera source: ");
	cameraComboBox.SetTooltip("Select a camera to use as texture");
	cameraComboBox.OnSelect([=] (auto args) {
		forEachSelected([] (auto light, auto args) {
			light->cameraSource = int(args.userdata);
		})(args);
		auto& scene = editor->GetCurrentScene();
		CameraComponent* camera = scene.cameras.GetComponent((Entity)args.userdata);
		if (camera != nullptr)
		{
		 	camera->render_to_texture.resolution = XMUINT2(256, 256);
		}
	});
	AddWidget(&cameraComboBox);


	tipLabel.Create("TipLabel");
	tipLabel.SetText("Tip: you can add a material to this entity, and the base color texture of it will be used to tint the light color (point light needs a cubemap, spotlight needs a texture2D). You can also add a video to this entity and the video will be used as light color multiplier (only for spotlight and rectangle light).");
	tipLabel.SetFitTextEnabled(true);
	AddWidget(&tipLabel);

	lensflare_Label.Create("Lens flare textures: ");
	AddWidget(&lensflare_Label);

	for (size_t i = 0; i < arraysize(lensflare_Button); ++i)
	{
		lensflare_Button[i].Create("LensFlareSlot");
		lensflare_Button[i].SetText("");
		lensflare_Button[i].SetTooltip("Load a lensflare texture to this slot");
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
					light->lensFlareRimTextures[i] = wi::resourcemanager::Load(fileName);
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
	bool changed = this->entity != entity;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const LightComponent* light = scene.lights.GetComponent(entity);

	if (light != nullptr)
	{
		intensitySlider.SetEnabled(true);
		intensitySlider.SetValue(light->intensity);
		rangeSlider.SetValue(light->range);
		radiusSlider.SetValue(light->radius);
		lengthSlider.SetValue(light->length);
		heightSlider.SetValue(light->height);
		outerConeAngleSlider.SetValue(light->outerConeAngle);
		innerConeAngleSlider.SetValue(light->innerConeAngle);
		volumetricBoostSlider.SetValue(light->volumetric_boost);
		shadowCheckBox.SetEnabled(true);
		shadowCheckBox.SetCheck(light->IsCastingShadow());
		haloCheckBox.SetEnabled(true);
		haloCheckBox.SetCheck(light->IsVisualizerEnabled());
		volumetricsCheckBox.SetEnabled(true);
		volumetricsCheckBox.SetCheck(light->IsVolumetricsEnabled());
		staticCheckBox.SetEnabled(true);
		staticCheckBox.SetCheck(light->IsStatic());
		volumetricCloudsCheckBox.SetEnabled(true);
		volumetricCloudsCheckBox.SetCheck(light->IsVolumetricCloudsEnabled());
		colorPicker.SetEnabled(true);
		colorPicker.SetPickColor(wi::Color::fromFloat3(light->color));
		typeSelectorComboBox.SetSelectedWithoutCallback((int)light->GetType());
		shadowResolutionComboBox.SetSelectedByUserdataWithoutCallback(uint64_t(light->forced_shadow_resolution));
		shadowResolutionComboBox.SetEnabled(true);

		if (changed)
		{
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

			cameraComboBox.ClearItems();
			cameraComboBox.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
			for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
			{
				Entity cameraEntity = scene.cameras.GetEntity(i);
				const NameComponent* name = scene.names.GetComponent(cameraEntity);
				if (name != nullptr)
				{
					cameraComboBox.AddItem(name->name, (uint64_t)cameraEntity);
				}
			}
			cameraComboBox.SetSelectedByUserdataWithoutCallback((uint64_t)light->cameraSource);
		}
	}
}
void LightWindow::SetLightType(LightComponent::LightType type)
{
	RefreshCascades();
}

void LightWindow::RefreshCascades()
{
	LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);

	for (auto& x : cascades)
	{
		RemoveWidget(&x.distanceSlider);
		RemoveWidget(&x.removeButton);
	}
	cascades.clear();
	RemoveWidget(&addCascadeButton);

	if (light == nullptr || light->type != LightComponent::DIRECTIONAL)
		return;

	int counter = 0;
	for (auto& x : light->cascade_distances)
	{
		CascadeConfig& cascade = cascades.emplace_back();

		cascade.distanceSlider.Create(1, 1000, 0, 1000, "");
		cascade.distanceSlider.SetTooltip("Specify cascade's maximum reach distance from camera.\nNote: Increasing cascades indices should use increasing distances.");
		cascade.distanceSlider.SetSize(XMFLOAT2(100, 18));
		cascade.distanceSlider.OnSlide([=](wi::gui::EventArgs args) {
			light->cascade_distances[counter] = args.fValue;
		});
		cascade.distanceSlider.SetValue(light->cascade_distances[counter]);
		AddWidget(&cascade.distanceSlider);
		cascade.distanceSlider.SetEnabled(true);

		cascade.removeButton.Create("X");
		cascade.removeButton.SetTooltip("Remove this shadow cascade");
		cascade.removeButton.SetDescription("Cascade " + std::to_string(counter) + ": ");
		cascade.removeButton.SetSize(XMFLOAT2(18, 18));
		cascade.removeButton.OnClick([=](wi::gui::EventArgs args) {
			light->cascade_distances.erase(light->cascade_distances.begin() + counter);
			RefreshCascades();
		});
		AddWidget(&cascade.removeButton);
		cascade.removeButton.SetEnabled(true);

		counter++;
	}

	addCascadeButton.Create("Add shadow cascade");
	addCascadeButton.SetTooltip("Add new shadow cascade. Note that for each shadow cascades, the scene will be rendered again, so adding more will affect performance!");
	addCascadeButton.SetSize(XMFLOAT2(100, 20));
	addCascadeButton.OnClick([=](wi::gui::EventArgs args) {
		float prev_cascade = 1;
		if (!light->cascade_distances.empty())
		{
			prev_cascade = light->cascade_distances.back();
		}
		light->cascade_distances.push_back(prev_cascade * 10);
		RefreshCascades();
	});
	AddWidget(&addCascadeButton);
	addCascadeButton.SetEnabled(true);

	editor->generalWnd.RefreshTheme();

}

void LightWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 140;

	LightComponent* light = editor->GetCurrentScene().lights.GetComponent(entity);
	if (light != nullptr)
	{
		if (light->type == LightComponent::DIRECTIONAL)
		{
			rangeSlider.SetVisible(false);
			outerConeAngleSlider.SetVisible(false);
			innerConeAngleSlider.SetVisible(false);
			radiusSlider.SetVisible(true);
			radiusSlider.SetRange(0, 1);
			lengthSlider.SetVisible(false);
			heightSlider.SetVisible(false);
		}
		else
		{
			rangeSlider.SetVisible(true);
			if (light->type == LightComponent::SPOT)
			{
				outerConeAngleSlider.SetVisible(true);
				innerConeAngleSlider.SetVisible(true);
				radiusSlider.SetVisible(true);
				lengthSlider.SetVisible(false);
				heightSlider.SetVisible(false);
			}
			else if (light->type == LightComponent::POINT)
			{
				outerConeAngleSlider.SetVisible(false);
				innerConeAngleSlider.SetVisible(false);
				radiusSlider.SetVisible(true);
				lengthSlider.SetVisible(true);
				heightSlider.SetVisible(false);
			}
			else if (light->type == LightComponent::RECTANGLE)
			{
				outerConeAngleSlider.SetVisible(false);
				innerConeAngleSlider.SetVisible(false);
				radiusSlider.SetVisible(false);
				lengthSlider.SetVisible(true);
				heightSlider.SetVisible(true);
			}
			radiusSlider.SetRange(0, 10);
		}
	}

	layout.add_fullwidth(colorPicker);
	layout.add(typeSelectorComboBox);
	layout.add(intensitySlider);
	layout.add(rangeSlider);
	layout.add(outerConeAngleSlider);
	layout.add(innerConeAngleSlider);
	layout.add(volumetricBoostSlider);
	layout.add(radiusSlider);
	layout.add(lengthSlider);
	layout.add(heightSlider);
	layout.add_right(shadowCheckBox);
	layout.add_right(haloCheckBox);
	layout.add_right(volumetricsCheckBox);
	layout.add_right(staticCheckBox);
	layout.add_right(volumetricCloudsCheckBox);
	layout.add(shadowResolutionComboBox);
	layout.add(cameraComboBox);

	if (light != nullptr && light->GetType() == LightComponent::DIRECTIONAL)
	{
		layout.jump();
		for (auto& x : cascades)
		{
			layout.add(x.distanceSlider);
			x.removeButton.SetPos(XMFLOAT2(x.distanceSlider.GetPos().x - layout.padding - x.removeButton.GetSize().x, x.distanceSlider.GetPos().y));
		}
		layout.add_fullwidth(addCascadeButton);
	}

	layout.jump();

	layout.add_fullwidth(tipLabel);

	layout.jump();

	layout.add_fullwidth(lensflare_Label);
	layout.add_fullwidth(lensflare_Button[0]);
	layout.add_fullwidth(lensflare_Button[1]);
	layout.add_fullwidth(lensflare_Button[2]);
	layout.add_fullwidth(lensflare_Button[3]);
	layout.add_fullwidth(lensflare_Button[4]);
	layout.add_fullwidth(lensflare_Button[5]);
	layout.add_fullwidth(lensflare_Button[6]);

}
