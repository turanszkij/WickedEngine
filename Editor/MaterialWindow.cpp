#include "stdafx.h"
#include "MaterialWindow.h"

#include <sstream>

#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiGraphicsTypes;
using namespace wiECS;
using namespace wiSceneSystem;

MaterialWindow::MaterialWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	materialWindow = new wiWindow(GUI, "Material Window");
	materialWindow->SetSize(XMFLOAT2(760, 720));
	materialWindow->SetEnabled(false);
	GUI->AddWidget(materialWindow);

	materialNameField = new wiTextInputField("MaterialName");
	materialNameField->SetPos(XMFLOAT2(10, 30));
	materialNameField->SetSize(XMFLOAT2(300, 20));
	materialNameField->OnInputAccepted([&](wiEventArgs args) {
		NameComponent* name = wiRenderer::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;
		}
	});
	materialWindow->AddWidget(materialNameField);

	float x = 540, y = 0;
	float step = 35;

	waterCheckBox = new wiCheckBox("Water: ");
	waterCheckBox->SetTooltip("Set material as special water material.");
	waterCheckBox->SetPos(XMFLOAT2(570, y += step));
	waterCheckBox->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetWater(args.bValue);
	});
	materialWindow->AddWidget(waterCheckBox);

	planarReflCheckBox = new wiCheckBox("Planar Reflections: ");
	planarReflCheckBox->SetTooltip("Enable planar reflections. The mesh should be a single plane for best results.");
	planarReflCheckBox->SetPos(XMFLOAT2(570, y += step));
	planarReflCheckBox->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetPlanarReflections(args.bValue);
	});
	materialWindow->AddWidget(planarReflCheckBox);

	shadowCasterCheckBox = new wiCheckBox("Cast Shadow: ");
	shadowCasterCheckBox->SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox->SetPos(XMFLOAT2(570, y += step));
	shadowCasterCheckBox->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetCastShadow(args.bValue);
	});
	materialWindow->AddWidget(shadowCasterCheckBox);

	normalMapSlider = new wiSlider(0, 4, 1, 4000, "Normalmap: ");
	normalMapSlider->SetTooltip("How much the normal map should distort the face normals (bumpiness).");
	normalMapSlider->SetSize(XMFLOAT2(100, 30));
	normalMapSlider->SetPos(XMFLOAT2(x, y += step));
	normalMapSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetNormalMapStrength(args.fValue);
	});
	materialWindow->AddWidget(normalMapSlider);

	roughnessSlider = new wiSlider(0, 1, 0.5f, 1000, "Roughness: ");
	roughnessSlider->SetTooltip("Adjust the surface roughness. Rough surfaces are less shiny, more matte.");
	roughnessSlider->SetSize(XMFLOAT2(100, 30));
	roughnessSlider->SetPos(XMFLOAT2(x, y += step));
	roughnessSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRoughness(args.fValue);
	});
	materialWindow->AddWidget(roughnessSlider);

	reflectanceSlider = new wiSlider(0, 1, 0.5f, 1000, "Reflectance: ");
	reflectanceSlider->SetTooltip("Adjust the overall surface reflectivity.");
	reflectanceSlider->SetSize(XMFLOAT2(100, 30));
	reflectanceSlider->SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReflectance(args.fValue);
	});
	materialWindow->AddWidget(reflectanceSlider);

	metalnessSlider = new wiSlider(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider->SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.");
	metalnessSlider->SetSize(XMFLOAT2(100, 30));
	metalnessSlider->SetPos(XMFLOAT2(x, y += step));
	metalnessSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetMetalness(args.fValue);
	});
	materialWindow->AddWidget(metalnessSlider);

	alphaSlider = new wiSlider(0, 1, 1.0f, 1000, "Alpha: ");
	alphaSlider->SetTooltip("Adjusts the overall transparency of the surface. Don't forget to set a Blend mode or refraction index to take effect.");
	alphaSlider->SetSize(XMFLOAT2(100, 30));
	alphaSlider->SetPos(XMFLOAT2(x, y += step));
	alphaSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOpacity(args.fValue);
	});
	materialWindow->AddWidget(alphaSlider);

	alphaRefSlider = new wiSlider(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider->SetTooltip("Adjust the alpha cutoff threshold. This disables some optimizations so performance can be affected.");
	alphaRefSlider->SetSize(XMFLOAT2(100, 30));
	alphaRefSlider->SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetAlphaRef(args.fValue);
	});
	materialWindow->AddWidget(alphaRefSlider);

	refractionIndexSlider = new wiSlider(0, 1.0f, 0.02f, 1000, "Refraction Index: ");
	refractionIndexSlider->SetTooltip("Adjust the IOR (index of refraction). It controls the amount of distortion of the scene visible through the transparent object.");
	refractionIndexSlider->SetSize(XMFLOAT2(100, 30));
	refractionIndexSlider->SetPos(XMFLOAT2(x, y += step));
	refractionIndexSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRefractionIndex(args.fValue);
	});
	materialWindow->AddWidget(refractionIndexSlider);

	emissiveSlider = new wiSlider(0, 1, 0.0f, 1000, "Emissive: ");
	emissiveSlider->SetTooltip("Adjust the light emission of the surface. The color of the light emitted is that of the color of the material.");
	emissiveSlider->SetSize(XMFLOAT2(100, 30));
	emissiveSlider->SetPos(XMFLOAT2(x, y += step));
	emissiveSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetEmissive(args.fValue);
	});
	materialWindow->AddWidget(emissiveSlider);

	sssSlider = new wiSlider(0, 1, 0.0f, 1000, "Subsurface Scattering: ");
	sssSlider->SetTooltip("Adjust how much the light is scattered when entered inside the surface of the object. (SSS postprocess must be enabled)");
	sssSlider->SetSize(XMFLOAT2(100, 30));
	sssSlider->SetPos(XMFLOAT2(x, y += step));
	sssSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetSubsurfaceScattering(args.fValue);
	});
	materialWindow->AddWidget(sssSlider);

	pomSlider = new wiSlider(0, 0.1f, 0.0f, 1000, "Parallax Occlusion Mapping: ");
	pomSlider->SetTooltip("Adjust how much the bump map should affect the object (slow).");
	pomSlider->SetSize(XMFLOAT2(100, 30));
	pomSlider->SetPos(XMFLOAT2(x, y += step));
	pomSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetParallaxOcclusionMapping(args.fValue);
	});
	materialWindow->AddWidget(pomSlider);

	texAnimFrameRateSlider = new wiSlider(0, 60, 0, 60, "Texcoord anim FPS: ");
	texAnimFrameRateSlider->SetTooltip("Adjust the texture animation frame rate (frames per second). Any value above 0 will make the material dynamic.");
	texAnimFrameRateSlider->SetSize(XMFLOAT2(100, 30));
	texAnimFrameRateSlider->SetPos(XMFLOAT2(x, y += step));
	texAnimFrameRateSlider->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimFrameRate = args.fValue;
		}
	});
	materialWindow->AddWidget(texAnimFrameRateSlider);

	texAnimDirectionSliderU = new wiSlider(-0.05f, 0.05f, 0, 1000, "Texcoord anim U: ");
	texAnimDirectionSliderU->SetTooltip("Adjust the texture animation speed along the U direction in texture space.");
	texAnimDirectionSliderU->SetSize(XMFLOAT2(100, 30));
	texAnimDirectionSliderU->SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderU->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimDirection.x = args.fValue;
		}
	});
	materialWindow->AddWidget(texAnimDirectionSliderU);

	texAnimDirectionSliderV = new wiSlider(-0.05f, 0.05f, 0, 1000, "Texcoord anim V: ");
	texAnimDirectionSliderV->SetTooltip("Adjust the texture animation speed along the V direction in texture space.");
	texAnimDirectionSliderV->SetSize(XMFLOAT2(100, 30));
	texAnimDirectionSliderV->SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderV->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimDirection.y = args.fValue;
		}
	});
	materialWindow->AddWidget(texAnimDirectionSliderV);

	texMulSliderX = new wiSlider(0.01f, 10.0f, 0, 1000, "Texture TileSize X: ");
	texMulSliderX->SetTooltip("Adjust the texture mapping size.");
	texMulSliderX->SetSize(XMFLOAT2(100, 30));
	texMulSliderX->SetPos(XMFLOAT2(x, y += step));
	texMulSliderX->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetDirty();
			material->texMulAdd.x = args.fValue;
		}
	});
	materialWindow->AddWidget(texMulSliderX);

	texMulSliderY = new wiSlider(0.01f, 10.0f, 0, 1000, "Texture TileSize Y: ");
	texMulSliderY->SetTooltip("Adjust the texture mapping size.");
	texMulSliderY->SetSize(XMFLOAT2(100, 30));
	texMulSliderY->SetPos(XMFLOAT2(x, y += step));
	texMulSliderY->OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetDirty();
			material->texMulAdd.y = args.fValue;
		}
	});
	materialWindow->AddWidget(texMulSliderY);


	colorPicker = new wiColorPicker(GUI, "Material Color");
	colorPicker->SetPos(XMFLOAT2(10, 400));
	colorPicker->RemoveWidgets();
	colorPicker->SetVisible(true);
	colorPicker->SetEnabled(true);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			XMFLOAT3 col = args.color.toFloat3();
			material->SetBaseColor(XMFLOAT4(powf(col.x, 1.f / 2.2f), powf(col.y, 1.f / 2.2f), powf(col.z, 1.f / 2.2f), material->GetOpacity()));
		}
	});
	materialWindow->AddWidget(colorPicker);


	blendModeComboBox = new wiComboBox("Blend mode: ");
	blendModeComboBox->SetPos(XMFLOAT2(x, y += step));
	blendModeComboBox->SetSize(XMFLOAT2(100, 25));
	blendModeComboBox->OnSelect([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material != nullptr && args.iValue >= 0)
		{
			material->blendMode = static_cast<BLENDMODE>(args.iValue);
		}
	});
	blendModeComboBox->AddItem("Opaque");
	blendModeComboBox->AddItem("Alpha");
	blendModeComboBox->AddItem("Premultiplied");
	blendModeComboBox->AddItem("Additive");
	blendModeComboBox->SetEnabled(false);
	blendModeComboBox->SetTooltip("Set the blend mode of the material.");
	materialWindow->AddWidget(blendModeComboBox);


	//shaderTypeComboBox = new wiComboBox("Custom Shader: ");
	//shaderTypeComboBox->SetPos(XMFLOAT2(x, y += step));
	//shaderTypeComboBox->SetSize(XMFLOAT2(100, 25));
	//shaderTypeComboBox->OnSelect([&](wiEventArgs args) {
	//	MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
	//	if (material != nullptr)
	//	{
	//		if (args.iValue == 0)
	//		{
	//			material->customShader = nullptr;
	//		}
	//		else if (args.iValue > 0)
	//		{
	//			material->customShader = Material::customShaderPresets[args.iValue - 1];
	//		}
	//	}
	//});
	//shaderTypeComboBox->AddItem("None");
	//for (auto& x : Material::customShaderPresets)
	//{
	//	shaderTypeComboBox->AddItem(x->name);
	//}
	//shaderTypeComboBox->SetEnabled(false);
	//shaderTypeComboBox->SetTooltip("Set the custom shader of the material.");
	//materialWindow->AddWidget(shaderTypeComboBox);


	// Textures:

	x = 10;
	y = 60;

	texture_baseColor_Label = new wiLabel("BaseColorMap: ");
	texture_baseColor_Label->SetPos(XMFLOAT2(x, y += step));
	texture_baseColor_Label->SetSize(XMFLOAT2(120, 20));
	materialWindow->AddWidget(texture_baseColor_Label);

	texture_baseColor_Button = new wiButton("BaseColor");
	texture_baseColor_Button->SetText("");
	texture_baseColor_Button->SetTooltip("Load the basecolor texture. RGB: Albedo Base Color, A: Opacity");
	texture_baseColor_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_baseColor_Button->SetSize(XMFLOAT2(260, 20));
	texture_baseColor_Button->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->baseColorMap != nullptr)
		{
			material->baseColorMap = nullptr;
			material->baseColorMapName = "";
			texture_baseColor_Button->SetText("");
		}
		else
		{
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;*.tga;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->baseColorMap = (Texture2D*)wiResourceManager::GetGlobal().add(fileName);
				material->baseColorMapName = fileName;
				fileName = wiHelper::GetFileNameFromPath(fileName);
				texture_baseColor_Button->SetText(fileName);
			}
		}
	});
	materialWindow->AddWidget(texture_baseColor_Button);



	texture_normal_Label = new wiLabel("NormalMap: ");
	texture_normal_Label->SetPos(XMFLOAT2(x, y += step));
	texture_normal_Label->SetSize(XMFLOAT2(120, 20));
	materialWindow->AddWidget(texture_normal_Label);

	texture_normal_Button = new wiButton("NormalMap");
	texture_normal_Button->SetText("");
	texture_normal_Button->SetTooltip("Load the normalmap texture. RGB: Normal, A: Roughness");
	texture_normal_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_normal_Button->SetSize(XMFLOAT2(260, 20));
	texture_normal_Button->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->normalMap != nullptr)
		{
			material->normalMap = nullptr;
			material->normalMapName = "";
			texture_normal_Button->SetText("");
		}
		else
		{
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;*.tga;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->normalMap = (Texture2D*)wiResourceManager::GetGlobal().add(fileName);
				material->normalMapName = fileName;
				fileName = wiHelper::GetFileNameFromPath(fileName);
				texture_normal_Button->SetText(fileName);
			}
		}
	});
	materialWindow->AddWidget(texture_normal_Button);



	texture_surface_Label = new wiLabel("SurfaceMap: ");
	texture_surface_Label->SetPos(XMFLOAT2(x, y += step));
	texture_surface_Label->SetSize(XMFLOAT2(120, 20));
	materialWindow->AddWidget(texture_surface_Label);

	texture_surface_Button = new wiButton("SurfaceMap");
	texture_surface_Button->SetText("");
	texture_surface_Button->SetTooltip("Load the surface property texture: R: Reflectance, G: Metalness, B: Emissive, A: SSS");
	texture_surface_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_surface_Button->SetSize(XMFLOAT2(260, 20));
	texture_surface_Button->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->surfaceMap != nullptr)
		{
			material->surfaceMap = nullptr;
			material->surfaceMapName = "";
			texture_surface_Button->SetText("");
		}
		else
		{
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;*.tga;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->surfaceMap = (Texture2D*)wiResourceManager::GetGlobal().add(fileName);
				material->surfaceMapName = fileName;
				fileName = wiHelper::GetFileNameFromPath(fileName);
				texture_surface_Button->SetText(fileName);
			}
		}
	});
	materialWindow->AddWidget(texture_surface_Button);



	texture_displacement_Label = new wiLabel("DisplacementMap: ");
	texture_displacement_Label->SetPos(XMFLOAT2(x, y += step));
	texture_displacement_Label->SetSize(XMFLOAT2(120, 20));
	materialWindow->AddWidget(texture_displacement_Label);

	texture_displacement_Button = new wiButton("DisplacementMap");
	texture_displacement_Button->SetText("");
	texture_displacement_Button->SetTooltip("Load the displacement map texture.");
	texture_displacement_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_displacement_Button->SetSize(XMFLOAT2(260, 20));
	texture_displacement_Button->OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiRenderer::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->displacementMap != nullptr)
		{
			material->displacementMap = nullptr;
			material->displacementMapName = "";
			texture_displacement_Button->SetText("");
		}
		else
		{
			char szFile[260];

			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = nullptr;
			ofn.lpstrFile = szFile;
			// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
			// use the contents of szFile to initialize itself.
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;*.tga;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->displacementMap = (Texture2D*)wiResourceManager::GetGlobal().add(fileName);
				material->displacementMapName = fileName;
				fileName = wiHelper::GetFileNameFromPath(fileName);
				texture_displacement_Button->SetText(fileName);
			}
		}
	});
	materialWindow->AddWidget(texture_displacement_Button);


	materialWindow->Translate(XMFLOAT3(screenW - 760, 50, 0));
	materialWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

MaterialWindow::~MaterialWindow()
{
	materialWindow->RemoveWidgets(true);
	GUI->RemoveWidget(materialWindow);
	SAFE_DELETE(materialWindow);
}



void MaterialWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wiRenderer::GetScene();
	MaterialComponent* material = scene.materials.GetComponent(entity);

	if (material != nullptr)
	{
		material->SetUserStencilRef(0);

		const NameComponent& name = *scene.names.GetComponent(entity);
		stringstream ss("");
		ss << name.name << " (" << entity << ")";

		materialNameField->SetValue(ss.str());
		waterCheckBox->SetCheck(material->IsWater());
		planarReflCheckBox->SetCheck(material->HasPlanarReflection());
		shadowCasterCheckBox->SetCheck(material->IsCastingShadow());
		normalMapSlider->SetValue(material->normalMapStrength);
		roughnessSlider->SetValue(material->roughness);
		reflectanceSlider->SetValue(material->reflectance);
		metalnessSlider->SetValue(material->metalness);
		alphaSlider->SetValue(material->GetOpacity());
		refractionIndexSlider->SetValue(material->refractionIndex);
		emissiveSlider->SetValue(material->emissive);
		sssSlider->SetValue(material->subsurfaceScattering);
		pomSlider->SetValue(material->parallaxOcclusionMapping);
		texAnimFrameRateSlider->SetValue(material->texAnimFrameRate);
		texAnimDirectionSliderU->SetValue(material->texAnimDirection.x);
		texAnimDirectionSliderV->SetValue(material->texAnimDirection.y);
		texMulSliderX->SetValue(material->texMulAdd.x);
		texMulSliderY->SetValue(material->texMulAdd.y);
		alphaRefSlider->SetValue(material->alphaRef);
		materialWindow->SetEnabled(true);
		colorPicker->SetEnabled(true);
		blendModeComboBox->SetSelected((int)material->blendMode);

		texture_baseColor_Button->SetText(wiHelper::GetFileNameFromPath(material->baseColorMapName));
		texture_normal_Button->SetText(wiHelper::GetFileNameFromPath(material->normalMapName));
		texture_surface_Button->SetText(wiHelper::GetFileNameFromPath(material->surfaceMapName));
		texture_displacement_Button->SetText(wiHelper::GetFileNameFromPath(material->displacementMapName));
	}
	else
	{
		materialNameField->SetValue("No material selected");
		materialWindow->SetEnabled(false);
		colorPicker->SetEnabled(false);

		texture_baseColor_Button->SetText("");
		texture_normal_Button->SetText("");
		texture_surface_Button->SetText("");
		texture_displacement_Button->SetText("");
	}
}
