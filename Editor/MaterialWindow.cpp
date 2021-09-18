#include "stdafx.h"
#include "MaterialWindow.h"
#include "Editor.h"

#include <sstream>

using namespace wiGraphics;
using namespace wiECS;
using namespace wiScene;

void MaterialWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Material Window");
	SetSize(XMFLOAT2(720, 620));

	float x = 670, y = 0;
	float hei = 18;
	float step = hei + 2;

	shadowReceiveCheckBox.Create("Receive Shadow: ");
	shadowReceiveCheckBox.SetTooltip("Receives shadow or not?");
	shadowReceiveCheckBox.SetPos(XMFLOAT2(540, y += step));
	shadowReceiveCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowReceiveCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReceiveShadow(args.bValue);
		});
	AddWidget(&shadowReceiveCheckBox);

	shadowCasterCheckBox.Create("Cast Shadow: ");
	shadowCasterCheckBox.SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox.SetPos(XMFLOAT2(670, y));
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
		SetEntity(entity);
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

	doubleSidedCheckBox.Create("Double sided: ");
	doubleSidedCheckBox.SetTooltip("Decide whether to render both sides of the material (It's also possible to set this behaviour per mesh).");
	doubleSidedCheckBox.SetPos(XMFLOAT2(540, y));
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.OnClick([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetDoubleSided(args.bValue);
		});
	AddWidget(&doubleSidedCheckBox);



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
				material->shaderType = (MaterialComponent::SHADERTYPE)args.userdata;
				material->SetCustomShaderID(-1);
				blendModeComboBox.SetEnabled(true);
			}
		}
		});
	shaderTypeComboBox.AddItem("PBR", MaterialComponent::SHADERTYPE_PBR);
	shaderTypeComboBox.AddItem("PBR + Planar reflections", MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION);
	shaderTypeComboBox.AddItem("PBR + Par. occl. mapping", MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING);
	shaderTypeComboBox.AddItem("PBR + Anisotropic", MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC);
	shaderTypeComboBox.AddItem("PBR + Cloth", MaterialComponent::SHADERTYPE_PBR_CLOTH);
	shaderTypeComboBox.AddItem("PBR + Clear coat", MaterialComponent::SHADERTYPE_PBR_CLEARCOAT);
	shaderTypeComboBox.AddItem("PBR + Cloth + Clear coat", MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT);
	shaderTypeComboBox.AddItem("Water", MaterialComponent::SHADERTYPE_WATER);
	shaderTypeComboBox.AddItem("Cartoon", MaterialComponent::SHADERTYPE_CARTOON);
	shaderTypeComboBox.AddItem("Unlit", MaterialComponent::SHADERTYPE_UNLIT);
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
	blendModeComboBox.AddItem("Multiply");
	blendModeComboBox.SetEnabled(false);
	blendModeComboBox.SetTooltip("Set the blend mode of the material.");
	AddWidget(&blendModeComboBox);

	shadingRateComboBox.Create("Shading Rate: ");
	shadingRateComboBox.SetTooltip("Select shading rate for this material. \nSelecting larger shading rate will decrease rendering quality of this material, \nbut increases performance.\nRequires hardware support for variable shading rate");
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
	reflectanceSlider.SetTooltip("Adjust the surface [non-metal] reflectivity (also called specularFactor).\nNote: this is not available in specular-glossiness workflow");
	reflectanceSlider.SetSize(XMFLOAT2(wid, hei));
	reflectanceSlider.SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReflectance(args.fValue);
	});
	AddWidget(&reflectanceSlider);

	metalnessSlider.Create(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider.SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.\nNote: this is not available in specular-glossiness workflow");
	metalnessSlider.SetSize(XMFLOAT2(wid, hei));
	metalnessSlider.SetPos(XMFLOAT2(x, y += step));
	metalnessSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetMetalness(args.fValue);
	});
	AddWidget(&metalnessSlider);

	alphaRefSlider.Create(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider.SetTooltip("Adjust the alpha cutoff threshold. Alpha cutout can affect performance");
	alphaRefSlider.SetSize(XMFLOAT2(wid, hei));
	alphaRefSlider.SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetAlphaRef(args.fValue);
	});
	AddWidget(&alphaRefSlider);

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

	transmissionSlider.Create(0, 1.0f, 0.02f, 1000, "Transmission: ");
	transmissionSlider.SetTooltip("Adjust the transmissiveness. More transmissiveness means more diffuse light is transmitted instead of absorbed.");
	transmissionSlider.SetSize(XMFLOAT2(wid, hei));
	transmissionSlider.SetPos(XMFLOAT2(x, y += step));
	transmissionSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetTransmissionAmount(args.fValue);
		});
	AddWidget(&transmissionSlider);

	refractionSlider.Create(0, 1, 0, 1000, "Refraction: ");
	refractionSlider.SetTooltip("Adjust the refraction amount for transmissive materials.");
	refractionSlider.SetSize(XMFLOAT2(wid, hei));
	refractionSlider.SetPos(XMFLOAT2(x, y += step));
	refractionSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRefractionAmount(args.fValue);
		});
	AddWidget(&refractionSlider);

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

	subsurfaceScatteringSlider.Create(0, 2, 0.0f, 1000, "Subsurface Scattering: ");
	subsurfaceScatteringSlider.SetTooltip("Subsurface scattering amount. \nYou can also adjust the subsurface color by selecting it in the color picker");
	subsurfaceScatteringSlider.SetSize(XMFLOAT2(wid, hei));
	subsurfaceScatteringSlider.SetPos(XMFLOAT2(x, y += step));
	subsurfaceScatteringSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetSubsurfaceScatteringAmount(args.fValue);
	});
	AddWidget(&subsurfaceScatteringSlider);

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


	sheenRoughnessSlider.Create(0, 1, 0, 1000, "Sheen Roughness: ");
	sheenRoughnessSlider.SetTooltip("This affects roughness of sheen layer for cloth shading.");
	sheenRoughnessSlider.SetSize(XMFLOAT2(wid, hei));
	sheenRoughnessSlider.SetPos(XMFLOAT2(x, y += step));
	sheenRoughnessSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetSheenRoughness(args.fValue);
		}
		});
	AddWidget(&sheenRoughnessSlider);

	clearcoatSlider.Create(0, 1, 0, 1000, "Clearcoat: ");
	clearcoatSlider.SetTooltip("This affects clearcoat layer blending.");
	clearcoatSlider.SetSize(XMFLOAT2(wid, hei));
	clearcoatSlider.SetPos(XMFLOAT2(x, y += step));
	clearcoatSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetClearcoatFactor(args.fValue);
		}
		});
	AddWidget(&clearcoatSlider);

	clearcoatRoughnessSlider.Create(0, 1, 0, 1000, "Clearcoat Roughness: ");
	clearcoatRoughnessSlider.SetTooltip("This affects roughness of clear coat layer.");
	clearcoatRoughnessSlider.SetSize(XMFLOAT2(wid, hei));
	clearcoatRoughnessSlider.SetPos(XMFLOAT2(x, y += step));
	clearcoatRoughnessSlider.OnSlide([&](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetClearcoatRoughness(args.fValue);
		}
		});
	AddWidget(&clearcoatRoughnessSlider);


	// 
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

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetSize(XMFLOAT2(120, hei));
	colorComboBox.SetPos(XMFLOAT2(x + 150, y += step));
	colorComboBox.AddItem("Base color");
	colorComboBox.AddItem("Specular color");
	colorComboBox.AddItem("Emissive color");
	colorComboBox.AddItem("Subsurface color");
	colorComboBox.AddItem("Sheen color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

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
				material->SetSpecularColor(args.color.toFloat4());
				break;
			case 2:
			{
				XMFLOAT3 col = args.color.toFloat3();
				material->SetEmissiveColor(XMFLOAT4(col.x, col.y, col.z, material->GetEmissiveStrength()));
			}
			break;
			case 3:
				material->SetSubsurfaceScatteringColor(args.color.toFloat3());
				break;
			case 4:
				material->SetSheenColor(args.color.toFloat3());
				break;
			}
		}
		});
	AddWidget(&colorPicker);


	// Textures:

	y += colorPicker.GetScale().y;

	textureSlotComboBox.Create("Texture Slot: ");
	textureSlotComboBox.SetSize(XMFLOAT2(170, hei));
	textureSlotComboBox.SetPos(XMFLOAT2(x + 100, y += step));
	for (int i = 0; i < MaterialComponent::TEXTURESLOT_COUNT; ++i)
	{
		switch (i)
		{
		case MaterialComponent::BASECOLORMAP:
			textureSlotComboBox.AddItem("BaseColor map");
			break;
		case MaterialComponent::NORMALMAP:
			textureSlotComboBox.AddItem("Normal map");
			break;
		case MaterialComponent::SURFACEMAP:
			textureSlotComboBox.AddItem("Surface map");
			break;
		case MaterialComponent::EMISSIVEMAP:
			textureSlotComboBox.AddItem("Emissive map");
			break;
		case MaterialComponent::OCCLUSIONMAP:
			textureSlotComboBox.AddItem("Occlusion map");
			break;
		case MaterialComponent::DISPLACEMENTMAP:
			textureSlotComboBox.AddItem("Displacement map");
			break;
		case MaterialComponent::TRANSMISSIONMAP:
			textureSlotComboBox.AddItem("Transmission map");
			break;
		case MaterialComponent::SHEENCOLORMAP:
			textureSlotComboBox.AddItem("SheenColor map");
			break;
		case MaterialComponent::SHEENROUGHNESSMAP:
			textureSlotComboBox.AddItem("SheenRoughness map");
			break;
		case MaterialComponent::CLEARCOATMAP:
			textureSlotComboBox.AddItem("Clearcoat map");
			break;
		case MaterialComponent::CLEARCOATROUGHNESSMAP:
			textureSlotComboBox.AddItem("ClearcoatRoughness map");
			break;
		case MaterialComponent::CLEARCOATNORMALMAP:
			textureSlotComboBox.AddItem("ClearcoatNormal map");
			break;
		case MaterialComponent::SPECULARMAP:
			textureSlotComboBox.AddItem("Specular map");
			break;
		default:
			break;
		}
	}
	textureSlotComboBox.OnSelect([this](wiEventArgs args)
		{

			switch (args.iValue)
			{
			case MaterialComponent::BASECOLORMAP:
				textureSlotButton.SetTooltip("RGBA: Basecolor");
				break;
			case MaterialComponent::NORMALMAP:
				textureSlotButton.SetTooltip("RGB: Normal");
				break;
			case MaterialComponent::SURFACEMAP:
				textureSlotButton.SetTooltip("Default workflow: R: Occlusion, G: Roughness, B: Metalness, A: Reflectance\nSpecular-glossiness workflow: RGB: Specular color (f0), A: smoothness");
				break;
			case MaterialComponent::EMISSIVEMAP:
				textureSlotButton.SetTooltip("RGBA: Emissive");
				break;
			case MaterialComponent::OCCLUSIONMAP:
				textureSlotButton.SetTooltip("R: Occlusion");
				break;
			case MaterialComponent::DISPLACEMENTMAP:
				textureSlotButton.SetTooltip("R: Displacement heightmap");
				break;
			case MaterialComponent::TRANSMISSIONMAP:
				textureSlotButton.SetTooltip("R: Transmission factor");
				break;
			case MaterialComponent::SHEENCOLORMAP:
				textureSlotButton.SetTooltip("RGB: Sheen color");
				break;
			case MaterialComponent::SHEENROUGHNESSMAP:
				textureSlotButton.SetTooltip("A: Roughness");
				break;
			case MaterialComponent::CLEARCOATMAP:
				textureSlotButton.SetTooltip("R: Clearcoat factor");
				break;
			case MaterialComponent::CLEARCOATROUGHNESSMAP:
				textureSlotButton.SetTooltip("G: Roughness");
				break;
			case MaterialComponent::CLEARCOATNORMALMAP:
				textureSlotButton.SetTooltip("RGB: Normal");
				break;
			case MaterialComponent::SPECULARMAP:
				textureSlotButton.SetTooltip("RGB: Specular color, A: Specular intensity [non-metal]");
				break;
			default:
				break;
			}

			MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
			if (material == nullptr)
				return;
			textureSlotButton.SetImage(material->textures[args.iValue].resource);

		});
	textureSlotComboBox.SetSelected(0);
	textureSlotComboBox.SetTooltip("Choose the texture slot to modify.");
	AddWidget(&textureSlotComboBox);

	textureSlotButton.Create("");
	textureSlotButton.SetSize(XMFLOAT2(180, 180));
	textureSlotButton.SetPos(XMFLOAT2(textureSlotComboBox.GetPosition().x + textureSlotComboBox.GetScale().x - textureSlotButton.GetScale().x, y += step));
	textureSlotButton.sprites[wiWidget::IDLE].params.color = wiColor::White();
	textureSlotButton.sprites[wiWidget::FOCUS].params.color = wiColor::Gray();
	textureSlotButton.sprites[wiWidget::ACTIVE].params.color = wiColor::White();
	textureSlotButton.sprites[wiWidget::DEACTIVATING].params.color = wiColor::Gray();
	textureSlotButton.OnClick([this](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		int slot = textureSlotComboBox.GetSelected();

		if (material->textures[slot].resource != nullptr)
		{
			material->textures[slot].resource = nullptr;
			material->textures[slot].name = "";
			material->SetDirty();
			textureSlotLabel.SetText("");
		}
		else
		{
			wiHelper::FileDialogParams params;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wiResourceManager::GetSupportedImageExtensions();
			wiHelper::FileDialog(params, [this, material, slot](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					material->textures[slot].resource = wiResourceManager::Load(fileName, wiResourceManager::IMPORT_RETAIN_FILEDATA);
					material->textures[slot].name = fileName;
					material->SetDirty();
					textureSlotLabel.SetText(wiHelper::GetFileNameFromPath(fileName));
					});
				});
		}
		});
	AddWidget(&textureSlotButton);

	y += textureSlotButton.GetScale().y - step + 2;

	textureSlotLabel.Create("");
	textureSlotLabel.SetPos(XMFLOAT2(x, y += step));
	textureSlotLabel.SetSize(XMFLOAT2(colorPicker.GetScale().x - hei - 2, hei));
	AddWidget(&textureSlotLabel);

	textureSlotUvsetField.Create("uvset");
	textureSlotUvsetField.SetText("");
	textureSlotUvsetField.SetTooltip("uv set number");
	textureSlotUvsetField.SetPos(XMFLOAT2(x + textureSlotLabel.GetScale().x + 2, y));
	textureSlotUvsetField.SetSize(XMFLOAT2(hei, hei));
	textureSlotUvsetField.OnInputAccepted([this](wiEventArgs args) {
		MaterialComponent* material = wiScene::GetScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			int slot = textureSlotComboBox.GetSelected();
			material->textures[slot].uvset = (uint32_t)args.iValue;
		}
		});
	AddWidget(&textureSlotUvsetField);


	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 880, 120, 0));
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
		shadowReceiveCheckBox.SetCheck(material->IsReceiveShadow());
		shadowCasterCheckBox.SetCheck(material->IsCastingShadow());
		useVertexColorsCheckBox.SetCheck(material->IsUsingVertexColors());
		specularGlossinessCheckBox.SetCheck(material->IsUsingSpecularGlossinessWorkflow());
		occlusionPrimaryCheckBox.SetCheck(material->IsOcclusionEnabled_Primary());
		occlusionSecondaryCheckBox.SetCheck(material->IsOcclusionEnabled_Secondary());
		windCheckBox.SetCheck(material->IsUsingWind());
		doubleSidedCheckBox.SetCheck(material->IsDoubleSided());
		normalMapSlider.SetValue(material->normalMapStrength);
		roughnessSlider.SetValue(material->roughness);
		reflectanceSlider.SetValue(material->reflectance);
		metalnessSlider.SetValue(material->metalness);
		transmissionSlider.SetValue(material->transmission);
		refractionSlider.SetValue(material->refraction);
		emissiveSlider.SetValue(material->emissiveColor.w);
		pomSlider.SetValue(material->parallaxOcclusionMapping);
		displacementMappingSlider.SetValue(material->displacementMapping);
		subsurfaceScatteringSlider.SetValue(material->subsurfaceScattering.w);
		texAnimFrameRateSlider.SetValue(material->texAnimFrameRate);
		texAnimDirectionSliderU.SetValue(material->texAnimDirection.x);
		texAnimDirectionSliderV.SetValue(material->texAnimDirection.y);
		texMulSliderX.SetValue(material->texMulAdd.x);
		texMulSliderY.SetValue(material->texMulAdd.y);
		alphaRefSlider.SetValue(material->alphaRef);
		blendModeComboBox.SetSelected((int)material->userBlendMode);
		if (material->GetCustomShaderID() >= 0)
		{
			shaderTypeComboBox.SetSelected(MaterialComponent::SHADERTYPE_COUNT + material->GetCustomShaderID());
		}
		else
		{
			shaderTypeComboBox.SetSelectedByUserdata(material->shaderType);
		}
		shadingRateComboBox.SetSelected((int)material->shadingRate);

		colorComboBox.SetEnabled(true);
		colorPicker.SetEnabled(true);
		
		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wiColor::fromFloat4(material->baseColor));
			break;
		case 1:
			colorPicker.SetPickColor(wiColor::fromFloat4(material->specularColor));
			break;
		case 2:
			colorPicker.SetPickColor(wiColor::fromFloat3(XMFLOAT3(material->emissiveColor.x, material->emissiveColor.y, material->emissiveColor.z)));
			break;
		case 3:
			colorPicker.SetPickColor(wiColor::fromFloat3(XMFLOAT3(material->subsurfaceScattering.x, material->subsurfaceScattering.y, material->subsurfaceScattering.z)));
			break;
		case 4:
			colorPicker.SetPickColor(wiColor::fromFloat3(XMFLOAT3(material->sheenColor.x, material->sheenColor.y, material->sheenColor.z)));
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

		sheenRoughnessSlider.SetEnabled(false);
		clearcoatSlider.SetEnabled(false);
		clearcoatRoughnessSlider.SetEnabled(false);
		switch (material->shaderType)
		{
		case MaterialComponent::SHADERTYPE_PBR_CLOTH:
			sheenRoughnessSlider.SetEnabled(true);
			break;
		case MaterialComponent::SHADERTYPE_PBR_CLEARCOAT:
			clearcoatSlider.SetEnabled(true);
			clearcoatRoughnessSlider.SetEnabled(true);
			break;
		case MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT:
			sheenRoughnessSlider.SetEnabled(true);
			clearcoatSlider.SetEnabled(true);
			clearcoatRoughnessSlider.SetEnabled(true);
			break;
		}
		sheenRoughnessSlider.SetValue(material->sheenRoughness);
		clearcoatSlider.SetValue(material->clearcoat);
		clearcoatRoughnessSlider.SetValue(material->clearcoatRoughness);

		shadingRateComboBox.SetEnabled(wiRenderer::GetDevice()->CheckCapability(GRAPHICSDEVICE_CAPABILITY_VARIABLE_RATE_SHADING));

		if (material->IsUsingSpecularGlossinessWorkflow())
		{
			reflectanceSlider.SetEnabled(false);
			metalnessSlider.SetEnabled(false);
		}

		int slot = textureSlotComboBox.GetSelected();
		textureSlotButton.SetImage(material->textures[slot].resource);
		textureSlotLabel.SetText(wiHelper::GetFileNameFromPath(material->textures[slot].name));
		textureSlotUvsetField.SetText(std::to_string(material->textures[slot].uvset));
	}
	else
	{
		materialNameField.SetValue("No material selected");
		SetEnabled(false);
		colorComboBox.SetEnabled(false);
		colorPicker.SetEnabled(false);

		textureSlotButton.SetImage(nullptr);
		textureSlotLabel.SetText("");
		textureSlotUvsetField.SetText("");
	}

	newMaterialButton.SetEnabled(true);
}
