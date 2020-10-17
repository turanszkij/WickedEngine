#include "stdafx.h"
#include "MaterialWindow.h"
#include "Editor.h"

#include <sstream>

using namespace std;
using namespace wiGraphics;
using namespace wiECS;
using namespace wiScene;

void MaterialWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Material Window");
	SetSize(XMFLOAT2(720, 520));

	float x = 670, y = 0;
	float hei = 18;
	float step = hei + 2;

	shadowCasterCheckBox.Create("Cast Shadow: ");
	shadowCasterCheckBox.SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox.SetPos(XMFLOAT2(670, y += step));
	shadowCasterCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCasterCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetCastShadow(args.bValue);
	});
	AddWidget(&shadowCasterCheckBox);

	useVertexColorsCheckBox.Create("Use vertex colors: ");
	useVertexColorsCheckBox.SetTooltip("Enable if you want to render the mesh with vertex colors (must have appropriate vertex buffer)");
	useVertexColorsCheckBox.SetPos(XMFLOAT2(670, y += step));
	useVertexColorsCheckBox.SetSize(XMFLOAT2(hei, hei));
	useVertexColorsCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseVertexColors(args.bValue);
	});
	AddWidget(&useVertexColorsCheckBox);

	specularGlossinessCheckBox.Create("Specular-glossiness workflow: ");
	specularGlossinessCheckBox.SetTooltip("If enabled, surface map will be viewed like it contains specular color (RGB) and smoothness (A)");
	specularGlossinessCheckBox.SetPos(XMFLOAT2(670, y += step));
	specularGlossinessCheckBox.SetSize(XMFLOAT2(hei, hei));
	specularGlossinessCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseSpecularGlossinessWorkflow(args.bValue);
	});
	AddWidget(&specularGlossinessCheckBox);

	occlusionPrimaryCheckBox.Create("Occlusion - Primary: ");
	occlusionPrimaryCheckBox.SetTooltip("If enabled, surface map's RED channel will be used as occlusion map");
	occlusionPrimaryCheckBox.SetPos(XMFLOAT2(670, y += step));
	occlusionPrimaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionPrimaryCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOcclusionEnabled_Primary(args.bValue);
	});
	AddWidget(&occlusionPrimaryCheckBox);

	occlusionSecondaryCheckBox.Create("Occlusion - Secondary: ");
	occlusionSecondaryCheckBox.SetTooltip("If enabled, occlusion map's RED channel will be used as occlusion map");
	occlusionSecondaryCheckBox.SetPos(XMFLOAT2(670, y += step));
	occlusionSecondaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionSecondaryCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOcclusionEnabled_Secondary(args.bValue);
	});
	AddWidget(&occlusionSecondaryCheckBox);

	windCheckBox.Create("Wind: ");
	windCheckBox.SetTooltip("If enabled, vertex wind weights will affect how much wind offset affects the subset.");
	windCheckBox.SetPos(XMFLOAT2(670, y += step));
	windCheckBox.SetSize(XMFLOAT2(hei, hei));
	windCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseWind(args.bValue);
		});
	AddWidget(&windCheckBox);



	x = 520;
	float wid = 170;


	shaderTypeComboBox.Create("Shader: ");
	shaderTypeComboBox.SetTooltip("Select a shader for this material. \nCustom shaders (*) will also show up here (see wiRenderer:RegisterCustomShader() for more info.)\nNote that custom shaders (*) can't select between blend modes, as they are created with an explicit blend mode.");
	shaderTypeComboBox.SetPos(XMFLOAT2(x, y += step));
	shaderTypeComboBox.SetSize(XMFLOAT2(wid, hei));
	shaderTypeComboBox.OnSelect([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			if (args.iValue >= MaterialComponent::SHADERTYPE_COUNT)
			{
				material->SetCustomShaderID(args.iValue - MaterialComponent::SHADERTYPE_COUNT);
				blendModeComboBox.SetEnabled(false);
			}
			else
			{
				material->shaderType = (MaterialComponent::SHADERTYPE)args.iValue;
				material->SetCustomShaderID(-1);
				blendModeComboBox.SetEnabled(true);
			}
		}
		});
	shaderTypeComboBox.AddItem("PBR");
	shaderTypeComboBox.AddItem("PBR + Planar reflections");
	shaderTypeComboBox.AddItem("PBR + Par. occl. mapping");
	shaderTypeComboBox.AddItem("PBR + Anisotropic");
	shaderTypeComboBox.AddItem("Water");
	shaderTypeComboBox.AddItem("Cartoon");
	shaderTypeComboBox.AddItem("Unlit");
	for (auto& x : wiRenderer::GetCustomShaders())
	{
		shaderTypeComboBox.AddItem("*" + x.name);
	}
	shaderTypeComboBox.SetEnabled(false);
	shaderTypeComboBox.SetMaxVisibleItemCount(5);
	AddWidget(&shaderTypeComboBox);

	blendModeComboBox.Create("Blend mode: ");
	blendModeComboBox.SetPos(XMFLOAT2(x, y += step));
	blendModeComboBox.SetSize(XMFLOAT2(wid, hei));
	blendModeComboBox.OnSelect([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr && args.iValue >= 0)
		{
			material->userBlendMode = (BLENDMODE)args.iValue;
		}
		});
	blendModeComboBox.AddItem("Opaque");
	blendModeComboBox.AddItem("Alpha");
	blendModeComboBox.AddItem("Premultiplied");
	blendModeComboBox.AddItem("Additive");
	blendModeComboBox.SetEnabled(false);
	blendModeComboBox.SetTooltip("Set the blend mode of the material.");
	AddWidget(&blendModeComboBox);

	sssComboBox.Create("Subsurface profile: ");
	sssComboBox.SetPos(XMFLOAT2(x, y += step));
	sssComboBox.SetSize(XMFLOAT2(wid, hei));
	sssComboBox.OnSelect([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr && args.iValue >= 0)
		{
			material->subsurfaceProfile = (MaterialComponent::SUBSURFACE_PROFILE)args.iValue;
		}
		});
	sssComboBox.AddItem("Solid");
	sssComboBox.AddItem("Skin");
	sssComboBox.AddItem("Snow");
	sssComboBox.SetEnabled(false);
	sssComboBox.SetTooltip("Set the subsurface profile of the material. Needs the SSS prost process enabled.");
	AddWidget(&sssComboBox);

	shadingRateComboBox.Create("Shading Rate: ");
	shadingRateComboBox.SetTooltip("Select shading rate for this material. \nSelecting larger shading rate will decrease rendering quality of this material, \nbut increases performance.\nDX12 only and requires Tier1 hardware support for variable shading rate");
	shadingRateComboBox.SetPos(XMFLOAT2(x, y += step));
	shadingRateComboBox.SetSize(XMFLOAT2(wid, hei));
	shadingRateComboBox.OnSelect([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->shadingRate = (SHADING_RATE)args.iValue;
		}
		});
	shadingRateComboBox.AddItem("1X1");
	shadingRateComboBox.AddItem("1X2");
	shadingRateComboBox.AddItem("2X1");
	shadingRateComboBox.AddItem("2X2");
	shadingRateComboBox.AddItem("2X4");
	shadingRateComboBox.AddItem("4X2");
	shadingRateComboBox.AddItem("4X4");
	shadingRateComboBox.SetEnabled(false);
	shadingRateComboBox.SetMaxVisibleItemCount(4);
	AddWidget(&shadingRateComboBox);




	// Sliders:
	wid = 150;

	normalMapSlider.Create(0, 4, 1, 4000, "Normalmap: ");
	normalMapSlider.SetTooltip("How much the normal map should distort the face normals (bumpiness).");
	normalMapSlider.SetSize(XMFLOAT2(wid, hei));
	normalMapSlider.SetPos(XMFLOAT2(x, y += step));
	normalMapSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetNormalMapStrength(args.fValue);
	});
	AddWidget(&normalMapSlider);

	roughnessSlider.Create(0, 1, 0.5f, 1000, "Roughness: ");
	roughnessSlider.SetTooltip("Adjust the surface roughness. Rough surfaces are less shiny, more matte.");
	roughnessSlider.SetSize(XMFLOAT2(wid, hei));
	roughnessSlider.SetPos(XMFLOAT2(x, y += step));
	roughnessSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRoughness(args.fValue);
	});
	AddWidget(&roughnessSlider);

	reflectanceSlider.Create(0, 1, 0.5f, 1000, "Reflectance: ");
	reflectanceSlider.SetTooltip("Adjust the overall surface reflectivity.");
	reflectanceSlider.SetSize(XMFLOAT2(wid, hei));
	reflectanceSlider.SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReflectance(args.fValue);
	});
	AddWidget(&reflectanceSlider);

	metalnessSlider.Create(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider.SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.");
	metalnessSlider.SetSize(XMFLOAT2(wid, hei));
	metalnessSlider.SetPos(XMFLOAT2(x, y += step));
	metalnessSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetMetalness(args.fValue);
	});
	AddWidget(&metalnessSlider);

	alphaRefSlider.Create(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider.SetTooltip("Adjust the alpha cutoff threshold. Some performance optimizations will be disabled.");
	alphaRefSlider.SetSize(XMFLOAT2(wid, hei));
	alphaRefSlider.SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetAlphaRef(args.fValue);
	});
	AddWidget(&alphaRefSlider);

	refractionIndexSlider.Create(0, 1.0f, 0.02f, 1000, "Refraction Index: ");
	refractionIndexSlider.SetTooltip("Adjust the IOR (index of refraction). It controls the amount of distortion of the scene visible through the transparent object. No effect when BlendMode is set to OPAQUE.");
	refractionIndexSlider.SetSize(XMFLOAT2(wid, hei));
	refractionIndexSlider.SetPos(XMFLOAT2(x, y += step));
	refractionIndexSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRefractionIndex(args.fValue);
	});
	AddWidget(&refractionIndexSlider);

	emissiveSlider.Create(0, 1, 0.0f, 1000, "Emissive: ");
	emissiveSlider.SetTooltip("Adjust the light emission of the surface. The color of the light emitted is that of the color of the material.");
	emissiveSlider.SetSize(XMFLOAT2(wid, hei));
	emissiveSlider.SetPos(XMFLOAT2(x, y += step));
	emissiveSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetEmissiveStrength(args.fValue);
	});
	AddWidget(&emissiveSlider);

	pomSlider.Create(0, 0.1f, 0.0f, 1000, "Parallax Occlusion Mapping: ");
	pomSlider.SetTooltip("Adjust how much the bump map should modulate the surface parallax effect. \nOnly works with PBR + Parallax shader.");
	pomSlider.SetSize(XMFLOAT2(wid, hei));
	pomSlider.SetPos(XMFLOAT2(x, y += step));
	pomSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetParallaxOcclusionMapping(args.fValue);
	});
	AddWidget(&pomSlider);

	displacementMappingSlider.Create(0, 0.1f, 0.0f, 1000, "Displacement Mapping: ");
	displacementMappingSlider.SetTooltip("Adjust how much the bump map should modulate the geometry when using tessellation.");
	displacementMappingSlider.SetSize(XMFLOAT2(wid, hei));
	displacementMappingSlider.SetPos(XMFLOAT2(x, y += step));
	displacementMappingSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetDisplacementMapping(args.fValue);
	});
	AddWidget(&displacementMappingSlider);

	texAnimFrameRateSlider.Create(0, 60, 0, 60, "Texcoord anim FPS: ");
	texAnimFrameRateSlider.SetTooltip("Adjust the texture animation frame rate (frames per second). Any value above 0 will make the material dynamic.");
	texAnimFrameRateSlider.SetSize(XMFLOAT2(wid, hei));
	texAnimFrameRateSlider.SetPos(XMFLOAT2(x, y += step));
	texAnimFrameRateSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimFrameRate = args.fValue;
		}
	});
	AddWidget(&texAnimFrameRateSlider);

	texAnimDirectionSliderU.Create(-0.05f, 0.05f, 0, 1000, "Texcoord anim U: ");
	texAnimDirectionSliderU.SetTooltip("Adjust the texture animation speed along the U direction in texture space.");
	texAnimDirectionSliderU.SetSize(XMFLOAT2(wid, hei));
	texAnimDirectionSliderU.SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderU.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimDirection.x = args.fValue;
		}
	});
	AddWidget(&texAnimDirectionSliderU);

	texAnimDirectionSliderV.Create(-0.05f, 0.05f, 0, 1000, "Texcoord anim V: ");
	texAnimDirectionSliderV.SetTooltip("Adjust the texture animation speed along the V direction in texture space.");
	texAnimDirectionSliderV.SetSize(XMFLOAT2(wid, hei));
	texAnimDirectionSliderV.SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderV.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->texAnimDirection.y = args.fValue;
		}
	});
	AddWidget(&texAnimDirectionSliderV);

	texMulSliderX.Create(0.01f, 10.0f, 0, 1000, "Texture TileSize X: ");
	texMulSliderX.SetTooltip("Adjust the texture mapping size.");
	texMulSliderX.SetSize(XMFLOAT2(wid, hei));
	texMulSliderX.SetPos(XMFLOAT2(x, y += step));
	texMulSliderX.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetDirty();
			material->texMulAdd.x = args.fValue;
		}
	});
	AddWidget(&texMulSliderX);

	texMulSliderY.Create(0.01f, 10.0f, 0, 1000, "Texture TileSize Y: ");
	texMulSliderY.SetTooltip("Adjust the texture mapping size.");
	texMulSliderY.SetSize(XMFLOAT2(wid, hei));
	texMulSliderY.SetPos(XMFLOAT2(x, y += step));
	texMulSliderY.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetDirty();
			material->texMulAdd.y = args.fValue;
		}
	});
	AddWidget(&texMulSliderY);


	// Textures:

	x = 10;
	y = 0;
	hei = 20;
	step = hei + 2;

	materialNameField.Create("MaterialName");
	materialNameField.SetTooltip("Set a name for the material...");
	materialNameField.SetPos(XMFLOAT2(10, y += step));
	materialNameField.SetSize(XMFLOAT2(300, hei));
	materialNameField.OnInputAccepted([=](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;

			editor->RefreshSceneGraphView();
		}
	});
	AddWidget(&materialNameField);

	newMaterialButton.Create("New Material");
	newMaterialButton.SetPos(XMFLOAT2(10 + 5 + 300, y));
	newMaterialButton.SetSize(XMFLOAT2(100, hei));
	newMaterialButton.OnClick([=](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		Entity entity = scene.Entity_CreateMaterial("editorMaterial");
		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
	});
	AddWidget(&newMaterialButton);

	texture_baseColor_Label.Create("BaseColorMap: ");
	texture_baseColor_Label.SetPos(XMFLOAT2(x, y += step));
	texture_baseColor_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_baseColor_Label);

	texture_baseColor_Button.Create("BaseColor");
	texture_baseColor_Button.SetText("");
	texture_baseColor_Button.SetTooltip("Load the basecolor texture. RGB: Albedo Base Color, A: Opacity");
	texture_baseColor_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_baseColor_Button.SetSize(XMFLOAT2(260, 20));
	texture_baseColor_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->baseColorMap != nullptr)
		{
			material->baseColorMap = nullptr;
			material->baseColorMapName = "";
			material->SetDirty();
			texture_baseColor_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->baseColorMap = wiResourceManager::Load(fileName);
					material->baseColorMapName = fileName;
					material->SetDirty();
					texture_baseColor_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_baseColor_Button);

	texture_baseColor_uvset_Field.Create("uvset_baseColor");
	texture_baseColor_uvset_Field.SetText("");
	texture_baseColor_uvset_Field.SetTooltip("uv set number");
	texture_baseColor_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_baseColor_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_baseColor_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_BaseColorMap(args.iValue);
		}
	});
	AddWidget(&texture_baseColor_uvset_Field);



	texture_normal_Label.Create("NormalMap: ");
	texture_normal_Label.SetPos(XMFLOAT2(x, y += step));
	texture_normal_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_normal_Label);

	texture_normal_Button.Create("NormalMap");
	texture_normal_Button.SetText("");
	texture_normal_Button.SetTooltip("Load the normalmap texture. RGB: Normal");
	texture_normal_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_normal_Button.SetSize(XMFLOAT2(260, 20));
	texture_normal_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->normalMap != nullptr)
		{
			material->normalMap = nullptr;
			material->normalMapName = "";
			material->SetDirty();
			texture_normal_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->normalMap = wiResourceManager::Load(fileName);
					material->normalMapName = fileName;
					material->SetDirty();
					texture_normal_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_normal_Button);

	texture_normal_uvset_Field.Create("uvset_normal");
	texture_normal_uvset_Field.SetText("");
	texture_normal_uvset_Field.SetTooltip("uv set number");
	texture_normal_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_normal_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_normal_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_NormalMap(args.iValue);
		}
	});
	AddWidget(&texture_normal_uvset_Field);



	texture_surface_Label.Create("SurfaceMap: ");
	texture_surface_Label.SetPos(XMFLOAT2(x, y += step));
	texture_surface_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_surface_Label);

	texture_surface_Button.Create("SurfaceMap");
	texture_surface_Button.SetText("");
	texture_surface_Button.SetTooltip("Load the surface property texture: R: Occlusion, G: Roughness, B: Metalness, A: Reflectance");
	texture_surface_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_surface_Button.SetSize(XMFLOAT2(260, 20));
	texture_surface_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->surfaceMap != nullptr)
		{
			material->surfaceMap = nullptr;
			material->surfaceMapName = "";
			material->SetDirty();
			texture_surface_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->surfaceMap = wiResourceManager::Load(fileName);
					material->surfaceMapName = fileName;
					material->SetDirty();
					texture_surface_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_surface_Button);

	texture_surface_uvset_Field.Create("uvset_surface");
	texture_surface_uvset_Field.SetText("");
	texture_surface_uvset_Field.SetTooltip("uv set number");
	texture_surface_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_surface_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_surface_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_SurfaceMap(args.iValue);
		}
	});
	AddWidget(&texture_surface_uvset_Field);



	texture_emissive_Label.Create("EmissiveMap: ");
	texture_emissive_Label.SetPos(XMFLOAT2(x, y += step));
	texture_emissive_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_emissive_Label);

	texture_emissive_Button.Create("EmissiveMap");
	texture_emissive_Button.SetText("");
	texture_emissive_Button.SetTooltip("Load the emissive map texture.");
	texture_emissive_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_emissive_Button.SetSize(XMFLOAT2(260, 20));
	texture_emissive_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->emissiveMap != nullptr)
		{
			material->emissiveMap = nullptr;
			material->emissiveMapName = "";
			material->SetDirty();
			texture_emissive_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->emissiveMap = wiResourceManager::Load(fileName);
					material->emissiveMapName = fileName;
					material->SetDirty();
					texture_emissive_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_emissive_Button);

	texture_emissive_uvset_Field.Create("uvset_emissive");
	texture_emissive_uvset_Field.SetText("");
	texture_emissive_uvset_Field.SetTooltip("uv set number");
	texture_emissive_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_emissive_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_emissive_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_EmissiveMap(args.iValue);
		}
		});
	AddWidget(&texture_emissive_uvset_Field);



	texture_displacement_Label.Create("DisplacementMap: ");
	texture_displacement_Label.SetPos(XMFLOAT2(x, y += step));
	texture_displacement_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_displacement_Label);

	texture_displacement_Button.Create("DisplacementMap");
	texture_displacement_Button.SetText("");
	texture_displacement_Button.SetTooltip("Load the displacement map texture.");
	texture_displacement_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_displacement_Button.SetSize(XMFLOAT2(260, 20));
	texture_displacement_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->displacementMap != nullptr)
		{
			material->displacementMap = nullptr;
			material->displacementMapName = "";
			material->SetDirty();
			texture_displacement_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->displacementMap = wiResourceManager::Load(fileName);
					material->displacementMapName = fileName;
					material->SetDirty();
					texture_displacement_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_displacement_Button);

	texture_displacement_uvset_Field.Create("uvset_displacement");
	texture_displacement_uvset_Field.SetText("");
	texture_displacement_uvset_Field.SetTooltip("uv set number");
	texture_displacement_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_displacement_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_displacement_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_DisplacementMap(args.iValue);
		}
	});
	AddWidget(&texture_displacement_uvset_Field);




	texture_occlusion_Label.Create("OcclusionMap: ");
	texture_occlusion_Label.SetPos(XMFLOAT2(x, y += step));
	texture_occlusion_Label.SetSize(XMFLOAT2(120, 20));
	AddWidget(&texture_occlusion_Label);

	texture_occlusion_Button.Create("OcclusionMap");
	texture_occlusion_Button.SetText("");
	texture_occlusion_Button.SetTooltip("Load the occlusion map texture. R: occlusion factor");
	texture_occlusion_Button.SetPos(XMFLOAT2(x + 122, y));
	texture_occlusion_Button.SetSize(XMFLOAT2(260, 20));
	texture_occlusion_Button.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		if (material->occlusionMap != nullptr)
		{
			material->occlusionMap = nullptr;
			material->occlusionMapName = "";
			material->SetDirty();
			texture_occlusion_Button.SetText("");
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
			wiHelper::FileDialog(params, [this, material](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->occlusionMap = wiResourceManager::Load(fileName);
					material->occlusionMapName = fileName;
					material->SetDirty();
					texture_occlusion_Button.SetText(wiHelper::GetFileNameFromPath(fileName));
				});
			});
		}
	});
	AddWidget(&texture_occlusion_Button);

	texture_occlusion_uvset_Field.Create("uvset_occlusion");
	texture_occlusion_uvset_Field.SetText("");
	texture_occlusion_uvset_Field.SetTooltip("uv set number");
	texture_occlusion_uvset_Field.SetPos(XMFLOAT2(x + 392, y));
	texture_occlusion_uvset_Field.SetSize(XMFLOAT2(20, 20));
	texture_occlusion_uvset_Field.OnInputAccepted([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetUVSet_OcclusionMap(args.iValue);
		}
	});
	AddWidget(&texture_occlusion_uvset_Field);


	y = 180;

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetSize(XMFLOAT2(120, hei));
	colorComboBox.SetPos(XMFLOAT2(x + 150, y += step));
	colorComboBox.AddItem("Base color");
	colorComboBox.AddItem("Emissive color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	y += 10;

	colorPicker.Create("Color", false);
	colorPicker.SetPos(XMFLOAT2(10, y += step));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			switch (colorComboBox.GetSelected())
			{
			default:
			case 0:
				material->SetBaseColor(args.color.toFloat4());
				break;
			case 1:
			{
				XMFLOAT3 col = args.color.toFloat3();
				material->SetEmissiveColor(XMFLOAT4(col.x, col.y, col.z, material->GetEmissiveStrength()));
			}
			break;
			}
		}
	});
	AddWidget(&colorPicker);


	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 880, 120, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}



void MaterialWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
	MaterialComponent* material = scene.materials.GetComponent(entity);

	if (material != nullptr)
	{
		SetEnabled(true);

		const NameComponent& name = *scene.names.GetComponent(entity);

		materialNameField.SetValue(name.name);
		shadowCasterCheckBox.SetCheck(material->IsCastingShadow());
		useVertexColorsCheckBox.SetCheck(material->IsUsingVertexColors());
		specularGlossinessCheckBox.SetCheck(material->IsUsingSpecularGlossinessWorkflow());
		occlusionPrimaryCheckBox.SetCheck(material->IsOcclusionEnabled_Primary());
		occlusionSecondaryCheckBox.SetCheck(material->IsOcclusionEnabled_Secondary());
		windCheckBox.SetCheck(material->IsUsingWind());
		normalMapSlider.SetValue(material->normalMapStrength);
		roughnessSlider.SetValue(material->roughness);
		reflectanceSlider.SetValue(material->reflectance);
		metalnessSlider.SetValue(material->metalness);
		refractionIndexSlider.SetValue(material->refractionIndex);
		emissiveSlider.SetValue(material->emissiveColor.w);
		pomSlider.SetValue(material->parallaxOcclusionMapping);
		displacementMappingSlider.SetValue(material->displacementMapping);
		texAnimFrameRateSlider.SetValue(material->texAnimFrameRate);
		texAnimDirectionSliderU.SetValue(material->texAnimDirection.x);
		texAnimDirectionSliderV.SetValue(material->texAnimDirection.y);
		texMulSliderX.SetValue(material->texMulAdd.x);
		texMulSliderY.SetValue(material->texMulAdd.y);
		alphaRefSlider.SetValue(material->alphaRef);
		blendModeComboBox.SetSelected((int)material->userBlendMode);
		sssComboBox.SetSelected((int)material->subsurfaceProfile);
		if (material->GetCustomShaderID() >= 0)
		{
			shaderTypeComboBox.SetSelected(MaterialComponent::SHADERTYPE_COUNT + material->GetCustomShaderID());
		}
		else
		{
			shaderTypeComboBox.SetSelected((int)material->shaderType);
		}
		shadingRateComboBox.SetSelected((int)material->shadingRate);

		texture_baseColor_Button.SetText(wiHelper::GetFileNameFromPath(material->baseColorMapName));
		texture_normal_Button.SetText(wiHelper::GetFileNameFromPath(material->normalMapName));
		texture_surface_Button.SetText(wiHelper::GetFileNameFromPath(material->surfaceMapName));
		texture_displacement_Button.SetText(wiHelper::GetFileNameFromPath(material->displacementMapName));
		texture_emissive_Button.SetText(wiHelper::GetFileNameFromPath(material->emissiveMapName));
		texture_occlusion_Button.SetText(wiHelper::GetFileNameFromPath(material->occlusionMapName));

		texture_baseColor_uvset_Field.SetText(std::to_string(material->uvset_baseColorMap));
		texture_normal_uvset_Field.SetText(std::to_string(material->uvset_normalMap));
		texture_surface_uvset_Field.SetText(std::to_string(material->uvset_surfaceMap));
		texture_displacement_uvset_Field.SetText(std::to_string(material->uvset_displacementMap));
		texture_emissive_uvset_Field.SetText(std::to_string(material->uvset_emissiveMap));
		texture_occlusion_uvset_Field.SetText(std::to_string(material->uvset_occlusionMap));


		colorComboBox.SetEnabled(true);
		colorPicker.SetEnabled(true);
		
		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wiColor::fromFloat4(material->baseColor));
			break;
		case 1:
			colorPicker.SetPickColor(wiColor::fromFloat3(XMFLOAT3(material->emissiveColor.x, material->emissiveColor.y, material->emissiveColor.z)));
			break;
		}

		switch (material->shaderType)
		{
		case MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC:
			pomSlider.SetText("Anisotropy: ");
			pomSlider.SetTooltip("Adjust anisotropy specular effect. \nOnly works with PBR + Anisotropic shader.");
			pomSlider.SetRange(0, 0.99f);
			break;
		case MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING:
			pomSlider.SetText("Parallax Occlusion Mapping: ");
			pomSlider.SetTooltip("Adjust how much the bump map should modulate the surface parallax effect. \nOnly works with PBR + Parallax shader.");
			pomSlider.SetRange(0, 0.1f);
			break;
		default:
			pomSlider.SetEnabled(false);
			break;
		}

		shadingRateComboBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING));
	}
	else
	{
		materialNameField.SetValue("No material selected");
		SetEnabled(false);
		colorComboBox.SetEnabled(false);
		colorPicker.SetEnabled(false);

		texture_baseColor_Button.SetText("");
		texture_normal_Button.SetText("");
		texture_surface_Button.SetText("");
		texture_displacement_Button.SetText("");
		texture_emissive_Button.SetText("");
		texture_occlusion_Button.SetText("");

		texture_baseColor_uvset_Field.SetText("");
		texture_normal_uvset_Field.SetText("");
		texture_surface_uvset_Field.SetText("");
		texture_displacement_uvset_Field.SetText("");
		texture_emissive_uvset_Field.SetText("");
		texture_occlusion_uvset_Field.SetText("");
	}

	newMaterialButton.SetEnabled(true);
}
