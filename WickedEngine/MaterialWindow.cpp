#include "stdafx.h"
#include "MaterialWindow.h"

#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiGraphicsTypes;

MaterialWindow::MaterialWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	material = nullptr;

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	materialWindow = new wiWindow(GUI, "Material Window");
	materialWindow->SetSize(XMFLOAT2(760, 670));
	materialWindow->SetEnabled(false);
	GUI->AddWidget(materialWindow);

	materialLabel = new wiLabel("MaterialName");
	materialLabel->SetPos(XMFLOAT2(10, 30));
	materialLabel->SetSize(XMFLOAT2(300, 20));
	materialLabel->SetText("");
	materialWindow->AddWidget(materialLabel);

	float x = 540, y = 0;
	float step = 35;

	waterCheckBox = new wiCheckBox("Water: ");
	waterCheckBox->SetTooltip("Set material as special water material.");
	waterCheckBox->SetPos(XMFLOAT2(570, y += step));
	waterCheckBox->OnClick([&](wiEventArgs args) {
		material->water = args.bValue;
	});
	materialWindow->AddWidget(waterCheckBox);

	planarReflCheckBox = new wiCheckBox("Planar Reflections: ");
	planarReflCheckBox->SetTooltip("Enable planar reflections. The mesh should be a single plane for best results.");
	planarReflCheckBox->SetPos(XMFLOAT2(570, y += step));
	planarReflCheckBox->OnClick([&](wiEventArgs args) {
		material->planar_reflections = args.bValue;
	});
	materialWindow->AddWidget(planarReflCheckBox);

	shadowCasterCheckBox = new wiCheckBox("Cast Shadow: ");
	shadowCasterCheckBox->SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox->SetPos(XMFLOAT2(570, y += step));
	shadowCasterCheckBox->OnClick([&](wiEventArgs args) {
		material->cast_shadow = args.bValue;
	});
	materialWindow->AddWidget(shadowCasterCheckBox);

	normalMapSlider = new wiSlider(0, 4, 1, 4000, "Normalmap: ");
	normalMapSlider->SetTooltip("How much the normal map should distort the face normals (bumpiness).");
	normalMapSlider->SetSize(XMFLOAT2(100, 30));
	normalMapSlider->SetPos(XMFLOAT2(x, y += step));
	normalMapSlider->OnSlide([&](wiEventArgs args) {
		material->normalMapStrength = args.fValue;
	});
	materialWindow->AddWidget(normalMapSlider);

	roughnessSlider = new wiSlider(0, 1, 0.5f, 1000, "Roughness: ");
	roughnessSlider->SetTooltip("Adjust the surface roughness. Rough surfaces are less shiny, more matte.");
	roughnessSlider->SetSize(XMFLOAT2(100, 30));
	roughnessSlider->SetPos(XMFLOAT2(x, y += step));
	roughnessSlider->OnSlide([&](wiEventArgs args) {
		material->roughness = args.fValue;
	});
	materialWindow->AddWidget(roughnessSlider);

	reflectanceSlider = new wiSlider(0, 1, 0.5f, 1000, "Reflectance: ");
	reflectanceSlider->SetTooltip("Adjust the overall surface reflectivity.");
	reflectanceSlider->SetSize(XMFLOAT2(100, 30));
	reflectanceSlider->SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider->OnSlide([&](wiEventArgs args) {
		material->reflectance = args.fValue;
	});
	materialWindow->AddWidget(reflectanceSlider);

	metalnessSlider = new wiSlider(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider->SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.");
	metalnessSlider->SetSize(XMFLOAT2(100, 30));
	metalnessSlider->SetPos(XMFLOAT2(x, y += step));
	metalnessSlider->OnSlide([&](wiEventArgs args) {
		material->metalness = args.fValue;
	});
	materialWindow->AddWidget(metalnessSlider);

	alphaSlider = new wiSlider(0, 1, 1.0f, 1000, "Alpha: ");
	alphaSlider->SetTooltip("Adjusts the overall transparency of the surface.");
	alphaSlider->SetSize(XMFLOAT2(100, 30));
	alphaSlider->SetPos(XMFLOAT2(x, y += step));
	alphaSlider->OnSlide([&](wiEventArgs args) {
		material->alpha = args.fValue;
	});
	materialWindow->AddWidget(alphaSlider);

	alphaRefSlider = new wiSlider(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider->SetTooltip("Adjust the alpha cutoff threshold.");
	alphaRefSlider->SetSize(XMFLOAT2(100, 30));
	alphaRefSlider->SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider->OnSlide([&](wiEventArgs args) {
		material->alphaRef = args.fValue;
	});
	materialWindow->AddWidget(alphaRefSlider);

	refractionIndexSlider = new wiSlider(0, 1.0f, 0.02f, 1000, "Refraction Index: ");
	refractionIndexSlider->SetTooltip("Adjust the IOR (index of refraction). It controls the amount of distortion of the scene visible through the transparent object.");
	refractionIndexSlider->SetSize(XMFLOAT2(100, 30));
	refractionIndexSlider->SetPos(XMFLOAT2(x, y += step));
	refractionIndexSlider->OnSlide([&](wiEventArgs args) {
		material->refractionIndex = args.fValue;
	});
	materialWindow->AddWidget(refractionIndexSlider);

	emissiveSlider = new wiSlider(0, 1, 0.0f, 1000, "Emissive: ");
	emissiveSlider->SetTooltip("Adjust the light emission of the surface. The color of the light emitted is that of the color of the material.");
	emissiveSlider->SetSize(XMFLOAT2(100, 30));
	emissiveSlider->SetPos(XMFLOAT2(x, y += step));
	emissiveSlider->OnSlide([&](wiEventArgs args) {
		material->emissive = args.fValue;
	});
	materialWindow->AddWidget(emissiveSlider);

	sssSlider = new wiSlider(0, 1, 0.0f, 1000, "Subsurface Scattering: ");
	sssSlider->SetTooltip("Adjust how much the light is scattered when entered inside the surface of the object. (SSS postprocess must be enabled)");
	sssSlider->SetSize(XMFLOAT2(100, 30));
	sssSlider->SetPos(XMFLOAT2(x, y += step));
	sssSlider->OnSlide([&](wiEventArgs args) {
		material->subsurfaceScattering = args.fValue;
	});
	materialWindow->AddWidget(sssSlider);

	pomSlider = new wiSlider(0, 0.1f, 0.0f, 1000, "Parallax Occlusion Mapping: ");
	pomSlider->SetTooltip("Adjust how much the bump map should affect the object (slow).");
	pomSlider->SetSize(XMFLOAT2(100, 30));
	pomSlider->SetPos(XMFLOAT2(x, y += step));
	pomSlider->OnSlide([&](wiEventArgs args) {
		material->parallaxOcclusionMapping = args.fValue;
	});
	materialWindow->AddWidget(pomSlider);

	movingTexSliderU = new wiSlider(-0.05f, 0.05f, 0, 1000, "Texcoord anim U: ");
	movingTexSliderU->SetTooltip("Adjust the texture animation speed along the U direction in texture space.");
	movingTexSliderU->SetSize(XMFLOAT2(100, 30));
	movingTexSliderU->SetPos(XMFLOAT2(x, y += step));
	movingTexSliderU->OnSlide([&](wiEventArgs args) {
		material->movingTex.x = args.fValue;
	});
	materialWindow->AddWidget(movingTexSliderU);

	movingTexSliderV = new wiSlider(-0.05f, 0.05f, 0, 1000, "Texcoord anim V: ");
	movingTexSliderV->SetTooltip("Adjust the texture animation speed along the V direction in texture space.");
	movingTexSliderV->SetSize(XMFLOAT2(100, 30));
	movingTexSliderV->SetPos(XMFLOAT2(x, y += step));
	movingTexSliderV->OnSlide([&](wiEventArgs args) {
		material->movingTex.y = args.fValue;
	});
	materialWindow->AddWidget(movingTexSliderV);

	texMulSliderX = new wiSlider(0.01f, 10.0f, 0, 1000, "Texture TileSize X: ");
	texMulSliderX->SetTooltip("Adjust the texture mapping size.");
	texMulSliderX->SetSize(XMFLOAT2(100, 30));
	texMulSliderX->SetPos(XMFLOAT2(x, y += step));
	texMulSliderX->OnSlide([&](wiEventArgs args) {
		material->texMulAdd.x = args.fValue;
	});
	materialWindow->AddWidget(texMulSliderX);

	texMulSliderY = new wiSlider(0.01f, 10.0f, 0, 1000, "Texture TileSize Y: ");
	texMulSliderY->SetTooltip("Adjust the texture mapping size.");
	texMulSliderY->SetSize(XMFLOAT2(100, 30));
	texMulSliderY->SetPos(XMFLOAT2(x, y += step));
	texMulSliderY->OnSlide([&](wiEventArgs args) {
		material->texMulAdd.y = args.fValue;
	});
	materialWindow->AddWidget(texMulSliderY);


	colorPickerToggleButton = new wiButton("Color");
	colorPickerToggleButton->SetTooltip("Adjust the material color.");
	colorPickerToggleButton->SetPos(XMFLOAT2(x, y += step));
	colorPickerToggleButton->OnClick([&](wiEventArgs args) {
		colorPicker->SetVisible(!colorPicker->IsVisible());
	});
	materialWindow->AddWidget(colorPickerToggleButton);


	colorPicker = new wiColorPicker(GUI, "Material Color");
	colorPicker->SetVisible(false);
	colorPicker->SetEnabled(false);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		material->baseColor = XMFLOAT3(args.color.x, args.color.y, args.color.z);
	});
	GUI->AddWidget(colorPicker);


	// Textures:

	x = 10;
	y = 60;

	texture_baseColor_Label = new wiLabel("BaseColorMap: ");
	texture_baseColor_Label->SetPos(XMFLOAT2(x, y += step));
	texture_baseColor_Label->SetSize(XMFLOAT2(120, 20));
	materialWindow->AddWidget(texture_baseColor_Label);

	texture_baseColor_Button = new wiButton("BaseColor");
	texture_baseColor_Button->SetText("");
	texture_baseColor_Button->SetTooltip("Load the basecolor texture.");
	texture_baseColor_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_baseColor_Button->SetSize(XMFLOAT2(260, 20));
	texture_baseColor_Button->OnClick([&](wiEventArgs args) {

		if (material->texture != nullptr)
		{
			material->texture = nullptr;
			material->textureName = "";
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
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->texture = (Texture2D*)wiResourceManager::GetGlobal()->add(fileName);
				material->textureName = fileName;
				texture_baseColor_Button->SetText(wiHelper::GetFileNameFromPath(material->textureName));
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
	texture_normal_Button->SetTooltip("Load the normalmap texture.");
	texture_normal_Button->SetPos(XMFLOAT2(x + 122, y));
	texture_normal_Button->SetSize(XMFLOAT2(260, 20));
	texture_normal_Button->OnClick([&](wiEventArgs args) {

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
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->normalMap = (Texture2D*)wiResourceManager::GetGlobal()->add(fileName);
				material->normalMapName = fileName;
				texture_normal_Button->SetText(wiHelper::GetFileNameFromPath(material->normalMapName));
			}
		}
	});
	materialWindow->AddWidget(texture_normal_Button);



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
			ofn.lpstrFilter = "Texture\0*.dds;*.png;*.jpg;\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			ofn.Flags = 0;
			if (GetSaveFileNameA(&ofn) == TRUE) {
				string fileName = ofn.lpstrFile;
				material->displacementMap = (Texture2D*)wiResourceManager::GetGlobal()->add(fileName);
				material->displacementMapName = fileName;
				texture_displacement_Button->SetText(wiHelper::GetFileNameFromPath(material->displacementMapName));
			}
		}
	});
	materialWindow->AddWidget(texture_displacement_Button);


	materialWindow->Translate(XMFLOAT3(screenW - 760, 50, 0));
	materialWindow->SetVisible(false);

	SetMaterial(nullptr);
}

MaterialWindow::~MaterialWindow()
{
	SAFE_DELETE(materialWindow);
	SAFE_DELETE(materialLabel);
	SAFE_DELETE(waterCheckBox);
	SAFE_DELETE(planarReflCheckBox);
	SAFE_DELETE(shadowCasterCheckBox);
	SAFE_DELETE(normalMapSlider);
	SAFE_DELETE(roughnessSlider);
	SAFE_DELETE(reflectanceSlider);
	SAFE_DELETE(metalnessSlider);
	SAFE_DELETE(alphaSlider);
	SAFE_DELETE(refractionIndexSlider);
	SAFE_DELETE(emissiveSlider);
	SAFE_DELETE(sssSlider);
	SAFE_DELETE(pomSlider);
	SAFE_DELETE(movingTexSliderU);
	SAFE_DELETE(movingTexSliderV);
	SAFE_DELETE(texMulSliderX);
	SAFE_DELETE(texMulSliderY);
	SAFE_DELETE(colorPickerToggleButton);
	SAFE_DELETE(colorPicker);
	SAFE_DELETE(alphaRefSlider);

	SAFE_DELETE(texture_baseColor_Label);
	SAFE_DELETE(texture_normal_Label);
	SAFE_DELETE(texture_displacement_Label);

	SAFE_DELETE(texture_baseColor_Button);
	SAFE_DELETE(texture_normal_Button);
	SAFE_DELETE(texture_displacement_Button);
}



void MaterialWindow::SetMaterial(Material* mat)
{
	material = mat;
	if (material != nullptr)
	{
		materialLabel->SetText(material->name);
		waterCheckBox->SetCheck(material->water);
		planarReflCheckBox->SetCheck(material->planar_reflections);
		shadowCasterCheckBox->SetCheck(material->cast_shadow);
		normalMapSlider->SetValue(material->normalMapStrength);
		roughnessSlider->SetValue(material->roughness);
		reflectanceSlider->SetValue(material->reflectance);
		metalnessSlider->SetValue(material->metalness);
		alphaSlider->SetValue(material->alpha);
		refractionIndexSlider->SetValue(material->refractionIndex);
		emissiveSlider->SetValue(material->emissive);
		sssSlider->SetValue(material->subsurfaceScattering);
		pomSlider->SetValue(material->parallaxOcclusionMapping);
		movingTexSliderU->SetValue(material->movingTex.x);
		movingTexSliderU->SetValue(material->movingTex.x);
		texMulSliderX->SetValue(material->texMulAdd.x);
		texMulSliderY->SetValue(material->texMulAdd.y);
		alphaRefSlider->SetValue(material->alphaRef);
		materialWindow->SetEnabled(true);
		colorPicker->SetEnabled(true);

		texture_baseColor_Button->SetText(wiHelper::GetFileNameFromPath(material->textureName));
		texture_normal_Button->SetText(wiHelper::GetFileNameFromPath(material->normalMapName));
		texture_displacement_Button->SetText(wiHelper::GetFileNameFromPath(material->displacementMapName));
	}
	else
	{
		materialLabel->SetText("No material selected");
		materialWindow->SetEnabled(false);
		colorPicker->SetEnabled(false);

		texture_baseColor_Button->SetText("");
		texture_normal_Button->SetText("");
		texture_displacement_Button->SetText("");
	}
}
