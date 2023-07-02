#include "stdafx.h"
#include "MaterialWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void MaterialWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_MATERIAL " Material", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(300, 1340));

	closeButton.SetTooltip("Delete MaterialComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().materials.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float hei = 18;
	float step = hei + 2;
	float x = 150, y = 0;
	float wid = 130;

	shadowReceiveCheckBox.Create("Receive Shadow: ");
	shadowReceiveCheckBox.SetTooltip("Receives shadow or not?");
	shadowReceiveCheckBox.SetPos(XMFLOAT2(x, y));
	shadowReceiveCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowReceiveCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReceiveShadow(args.bValue);
		});
	AddWidget(&shadowReceiveCheckBox);

	shadowCasterCheckBox.Create("Cast Shadow: ");
	shadowCasterCheckBox.SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCasterCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCasterCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetCastShadow(args.bValue);
	});
	AddWidget(&shadowCasterCheckBox);

	useVertexColorsCheckBox.Create("Use vertex colors: ");
	useVertexColorsCheckBox.SetTooltip("Enable if you want to render the mesh with vertex colors (must have appropriate vertex buffer)");
	useVertexColorsCheckBox.SetPos(XMFLOAT2(x, y += step));
	useVertexColorsCheckBox.SetSize(XMFLOAT2(hei, hei));
	useVertexColorsCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseVertexColors(args.bValue);
	});
	AddWidget(&useVertexColorsCheckBox);

	specularGlossinessCheckBox.Create("Spec-gloss workflow: ");
	specularGlossinessCheckBox.SetTooltip("If enabled, surface map will be viewed like it contains specular color (RGB) and smoothness (A)");
	specularGlossinessCheckBox.SetPos(XMFLOAT2(x, y += step));
	specularGlossinessCheckBox.SetSize(XMFLOAT2(hei, hei));
	specularGlossinessCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseSpecularGlossinessWorkflow(args.bValue);
		SetEntity(entity);
	});
	AddWidget(&specularGlossinessCheckBox);

	occlusionPrimaryCheckBox.Create("Occlusion 1: ");
	occlusionPrimaryCheckBox.SetTooltip("If enabled, surface map's RED channel will be used as occlusion map");
	occlusionPrimaryCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionPrimaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionPrimaryCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOcclusionEnabled_Primary(args.bValue);
	});
	AddWidget(&occlusionPrimaryCheckBox);

	occlusionSecondaryCheckBox.Create("Occlusion 2: ");
	occlusionSecondaryCheckBox.SetTooltip("If enabled, occlusion map's RED channel will be used as occlusion map");
	occlusionSecondaryCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionSecondaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionSecondaryCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOcclusionEnabled_Secondary(args.bValue);
	});
	AddWidget(&occlusionSecondaryCheckBox);

	windCheckBox.Create("Wind: ");
	windCheckBox.SetTooltip("If enabled, vertex wind weights will affect how much wind offset affects the subset.");
	windCheckBox.SetPos(XMFLOAT2(x, y += step));
	windCheckBox.SetSize(XMFLOAT2(hei, hei));
	windCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetUseWind(args.bValue);
		});
	AddWidget(&windCheckBox);

	doubleSidedCheckBox.Create("Double sided: ");
	doubleSidedCheckBox.SetTooltip("Decide whether to render both sides of the material (It's also possible to set this behaviour per mesh).");
	doubleSidedCheckBox.SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetDoubleSided(args.bValue);
		});
	AddWidget(&doubleSidedCheckBox);

	outlineCheckBox.Create("Cartoon Outline: ");
	outlineCheckBox.SetTooltip("Enable cartoon outline. The Cartoon Outline graphics setting also needs to be enabled for it to show up.");
	outlineCheckBox.SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox.SetSize(XMFLOAT2(hei, hei));
	outlineCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetOutlineEnabled(args.bValue);
		});
	AddWidget(&outlineCheckBox);

	preferUncompressedCheckBox.Create("Prefer Uncompressed Textures: ");
	preferUncompressedCheckBox.SetTooltip("For uncompressed textures (jpg, png, etc.) or transcodable textures (KTX2, Basis) here it is possible to enable/disable auto block compression on importing. \nBlock compression can reduce GPU memory usage and improve performance, but it can result in degraded quality.");
	preferUncompressedCheckBox.SetPos(XMFLOAT2(x, y += step));
	preferUncompressedCheckBox.SetSize(XMFLOAT2(hei, hei));
	preferUncompressedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetPreferUncompressedTexturesEnabled(args.bValue);
			textureSlotComboBox.SetSelected(textureSlotComboBox.GetSelected());
		});
	AddWidget(&preferUncompressedCheckBox);


	shaderTypeComboBox.Create("Shader: ");
	shaderTypeComboBox.SetTooltip("Select a shader for this material. \nCustom shaders (*) will also show up here (see wi::renderer:RegisterCustomShader() for more info.)\nNote that custom shaders (*) can't select between blend modes, as they are created with an explicit blend mode.");
	shaderTypeComboBox.SetPos(XMFLOAT2(x, y += step));
	shaderTypeComboBox.SetSize(XMFLOAT2(wid, hei));
	shaderTypeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	shaderTypeComboBox.SetEnabled(false);
	shaderTypeComboBox.SetMaxVisibleItemCount(5);
	AddWidget(&shaderTypeComboBox);

	blendModeComboBox.Create("Blend mode: ");
	blendModeComboBox.SetPos(XMFLOAT2(x, y += step));
	blendModeComboBox.SetSize(XMFLOAT2(wid, hei));
	blendModeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr && args.iValue >= 0)
		{
			material->userBlendMode = (wi::enums::BLENDMODE)args.iValue;
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
	shadingRateComboBox.OnSelect([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->shadingRate = (ShadingRate)args.iValue;
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

	normalMapSlider.Create(0, 4, 1, 4000, "Normalmap: ");
	normalMapSlider.SetTooltip("How much the normal map should distort the face normals (bumpiness).");
	normalMapSlider.SetSize(XMFLOAT2(wid, hei));
	normalMapSlider.SetPos(XMFLOAT2(x, y += step));
	normalMapSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetNormalMapStrength(args.fValue);
	});
	AddWidget(&normalMapSlider);

	roughnessSlider.Create(0, 1, 0.5f, 1000, "Roughness: ");
	roughnessSlider.SetTooltip("Adjust the surface roughness. Rough surfaces are less shiny, more matte.");
	roughnessSlider.SetSize(XMFLOAT2(wid, hei));
	roughnessSlider.SetPos(XMFLOAT2(x, y += step));
	roughnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRoughness(args.fValue);
	});
	AddWidget(&roughnessSlider);

	reflectanceSlider.Create(0, 1, 0.5f, 1000, "Reflectance: ");
	reflectanceSlider.SetTooltip("Adjust the surface [non-metal] reflectivity (also called specularFactor).\nNote: this is not available in specular-glossiness workflow");
	reflectanceSlider.SetSize(XMFLOAT2(wid, hei));
	reflectanceSlider.SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetReflectance(args.fValue);
	});
	AddWidget(&reflectanceSlider);

	metalnessSlider.Create(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider.SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.\nNote: this is not available in specular-glossiness workflow");
	metalnessSlider.SetSize(XMFLOAT2(wid, hei));
	metalnessSlider.SetPos(XMFLOAT2(x, y += step));
	metalnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetMetalness(args.fValue);
	});
	AddWidget(&metalnessSlider);

	alphaRefSlider.Create(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider.SetTooltip("Adjust the alpha cutoff threshold. Alpha cutout can affect performance");
	alphaRefSlider.SetSize(XMFLOAT2(wid, hei));
	alphaRefSlider.SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetAlphaRef(args.fValue);
	});
	AddWidget(&alphaRefSlider);

	emissiveSlider.Create(0, 1, 0.0f, 1000, "Emissive: ");
	emissiveSlider.SetTooltip("Adjust the light emission of the surface. The color of the light emitted is that of the color of the material.");
	emissiveSlider.SetSize(XMFLOAT2(wid, hei));
	emissiveSlider.SetPos(XMFLOAT2(x, y += step));
	emissiveSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetEmissiveStrength(args.fValue);
	});
	AddWidget(&emissiveSlider);

	transmissionSlider.Create(0, 1.0f, 0.02f, 1000, "Transmission: ");
	transmissionSlider.SetTooltip("Adjust the transmissiveness. More transmissiveness means more diffuse light is transmitted instead of absorbed.");
	transmissionSlider.SetSize(XMFLOAT2(wid, hei));
	transmissionSlider.SetPos(XMFLOAT2(x, y += step));
	transmissionSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetTransmissionAmount(args.fValue);
		});
	AddWidget(&transmissionSlider);

	refractionSlider.Create(0, 1, 0, 1000, "Refraction: ");
	refractionSlider.SetTooltip("Adjust the refraction amount for transmissive materials.");
	refractionSlider.SetSize(XMFLOAT2(wid, hei));
	refractionSlider.SetPos(XMFLOAT2(x, y += step));
	refractionSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetRefractionAmount(args.fValue);
		});
	AddWidget(&refractionSlider);

	pomSlider.Create(0, 1.0f, 0.0f, 1000, "Par Occl Mapping: ");
	pomSlider.SetTooltip("[Parallax Occlusion Mapping] Adjust how much the bump map should modulate the surface parallax effect. \nOnly works with PBR + Parallax shader.");
	pomSlider.SetSize(XMFLOAT2(wid, hei));
	pomSlider.SetPos(XMFLOAT2(x, y += step));
	pomSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetParallaxOcclusionMapping(args.fValue);
	});
	AddWidget(&pomSlider);

	anisotropyStrengthSlider.Create(0, 1.0f, 0.0f, 1000, "Anisotropy Strength: ");
	anisotropyStrengthSlider.SetTooltip("Adjust anisotropy specular effect's strength. \nOnly works with PBR + Anisotropic shader.");
	anisotropyStrengthSlider.SetSize(XMFLOAT2(wid, hei));
	anisotropyStrengthSlider.SetPos(XMFLOAT2(x, y += step));
	anisotropyStrengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->anisotropy_strength = args.fValue;
		});
	AddWidget(&anisotropyStrengthSlider);

	anisotropyRotationSlider.Create(0, 360, 0.0f, 360, "Anisotropy Rot: ");
	anisotropyRotationSlider.SetTooltip("Adjust anisotropy specular effect's rotation. \nOnly works with PBR + Anisotropic shader.");
	anisotropyRotationSlider.SetSize(XMFLOAT2(wid, hei));
	anisotropyRotationSlider.SetPos(XMFLOAT2(x, y += step));
	anisotropyRotationSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->anisotropy_rotation = wi::math::DegreesToRadians(args.fValue);
		});
	AddWidget(&anisotropyRotationSlider);


	displacementMappingSlider.Create(0, 10.0f, 0.0f, 1000, "Displacement: ");
	displacementMappingSlider.SetTooltip("Adjust how much the bump map should modulate the geometry when using tessellation.");
	displacementMappingSlider.SetSize(XMFLOAT2(wid, hei));
	displacementMappingSlider.SetPos(XMFLOAT2(x, y += step));
	displacementMappingSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetDisplacementMapping(args.fValue);
	});
	AddWidget(&displacementMappingSlider);

	subsurfaceScatteringSlider.Create(0, 2, 0.0f, 1000, "Subsurface Scattering: ");
	subsurfaceScatteringSlider.SetTooltip("Subsurface scattering amount. \nYou can also adjust the subsurface color by selecting it in the color picker");
	subsurfaceScatteringSlider.SetSize(XMFLOAT2(wid, hei));
	subsurfaceScatteringSlider.SetPos(XMFLOAT2(x, y += step));
	subsurfaceScatteringSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
			material->SetSubsurfaceScatteringAmount(args.fValue);
	});
	AddWidget(&subsurfaceScatteringSlider);

	texAnimFrameRateSlider.Create(0, 60, 0, 60, "Texcoord anim FPS: ");
	texAnimFrameRateSlider.SetTooltip("Adjust the texture animation frame rate (frames per second). Any value above 0 will make the material dynamic.");
	texAnimFrameRateSlider.SetSize(XMFLOAT2(wid, hei));
	texAnimFrameRateSlider.SetPos(XMFLOAT2(x, y += step));
	texAnimFrameRateSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	texAnimDirectionSliderU.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	texAnimDirectionSliderV.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	texMulSliderX.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	texMulSliderY.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	sheenRoughnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	clearcoatSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
	clearcoatRoughnessSlider.OnSlide([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			material->SetClearcoatRoughness(args.fValue);
		}
		});
	AddWidget(&clearcoatRoughnessSlider);


	// 
	hei = 20;
	step = hei + 2;
	x = 10;

	materialNameField.Create("MaterialName");
	materialNameField.SetTooltip("Set a name for the material...");
	materialNameField.SetPos(XMFLOAT2(10, y += step));
	materialNameField.SetSize(XMFLOAT2(300, hei));
	materialNameField.OnInputAccepted([=](wi::gui::EventArgs args) {
		NameComponent* name = editor->GetCurrentScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;

			editor->optionsWnd.RefreshEntityTree();
		}
		});
	AddWidget(&materialNameField);

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

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(10, y += step));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
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
		case MaterialComponent::ANISOTROPYMAP:
			textureSlotComboBox.AddItem("Anisotropy map");
			break;
		default:
			break;
		}
	}
	textureSlotComboBox.OnSelect([this](wi::gui::EventArgs args) {

		std::string tooltiptext;

		switch (args.iValue)
		{
		case MaterialComponent::BASECOLORMAP:
			tooltiptext = "RGBA: Basecolor";
			break;
		case MaterialComponent::NORMALMAP:
			tooltiptext = "RG: Normal";
			break;
		case MaterialComponent::SURFACEMAP:
			tooltiptext = "Default workflow: R: Occlusion, G: Roughness, B: Metalness, A: Reflectance\nSpecular-glossiness workflow: RGB: Specular color (f0), A: smoothness";
			break;
		case MaterialComponent::EMISSIVEMAP:
			tooltiptext = "RGBA: Emissive";
			break;
		case MaterialComponent::OCCLUSIONMAP:
			tooltiptext = "R: Occlusion";
			break;
		case MaterialComponent::DISPLACEMENTMAP:
			tooltiptext = "R: Displacement heightmap";
			break;
		case MaterialComponent::TRANSMISSIONMAP:
			tooltiptext = "R: Transmission factor";
			break;
		case MaterialComponent::SHEENCOLORMAP:
			tooltiptext = "RGB: Sheen color";
			break;
		case MaterialComponent::SHEENROUGHNESSMAP:
			tooltiptext = "A: Roughness";
			break;
		case MaterialComponent::CLEARCOATMAP:
			tooltiptext = "R: Clearcoat factor";
			break;
		case MaterialComponent::CLEARCOATROUGHNESSMAP:
			tooltiptext = "G: Roughness";
			break;
		case MaterialComponent::CLEARCOATNORMALMAP:
			tooltiptext = "RG: Normal";
			break;
		case MaterialComponent::SPECULARMAP:
			tooltiptext = "RGB: Specular color, A: Specular intensity [non-metal]";
			break;
		case MaterialComponent::ANISOTROPYMAP:
			tooltiptext = "RG: The anisotropy texture. Red and green channels represent the anisotropy direction in [-1, 1] tangent, bitangent space.\nThe vector is rotated by anisotropyRotation, and multiplied by anisotropyStrength, to obtain the final anisotropy direction and strength.";
			break;
		default:
			break;
		}

		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			textureSlotButton.SetImage(material->textures[args.iValue].resource);
			if (material->textures[args.iValue].resource.IsValid())
			{
				const Texture& texture = material->textures[args.iValue].resource.GetTexture();
				tooltiptext += "\nResolution: " + std::to_string(texture.desc.width) + " * " + std::to_string(texture.desc.height);
				tooltiptext += "\nMip levels: " + std::to_string(texture.desc.mip_levels);
				tooltiptext += "\nFormat: ";
				tooltiptext += GetFormatString(texture.desc.format);
				tooltiptext += "\nSwizzle: ";
				tooltiptext += GetSwizzleString(texture.desc.swizzle);
				tooltiptext += "\nMemory: " + wi::helper::GetMemorySizeText(ComputeTextureMemorySizeInBytes(texture.desc));
			}
		}

		textureSlotButton.SetTooltip(tooltiptext);

	});
	textureSlotComboBox.SetSelected(0);
	textureSlotComboBox.SetTooltip("Choose the texture slot to modify.");
	AddWidget(&textureSlotComboBox);

	textureSlotButton.Create("");
	textureSlotButton.SetSize(XMFLOAT2(180, 180));
	textureSlotButton.SetPos(XMFLOAT2(textureSlotComboBox.GetPosition().x + textureSlotComboBox.GetScale().x - textureSlotButton.GetScale().x, y += step));
	textureSlotButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	textureSlotButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	textureSlotButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	textureSlotButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	textureSlotButton.OnClick([this](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		int slot = textureSlotComboBox.GetSelected();

		if (material->textures[slot].resource.IsValid())
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			material->textures[slot].resource = {};
			material->textures[slot].name = "";
			material->SetDirty();
			textureSlotLabel.SetText("");

			editor->RecordEntity(archive, entity);
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this, material, slot](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					wi::resourcemanager::Flags flags = material->GetTextureSlotResourceFlags(MaterialComponent::TEXTURESLOT(slot));
					material->textures[slot].resource = wi::resourcemanager::Load(fileName, flags);
					material->textures[slot].name = fileName;
					material->SetDirty();
					textureSlotLabel.SetText(wi::helper::GetFileNameFromPath(fileName));
					textureSlotComboBox.SetSelected(slot);
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
	textureSlotUvsetField.OnInputAccepted([this](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			int slot = textureSlotComboBox.GetSelected();
			material->textures[slot].uvset = (uint32_t)args.iValue;
		}
		});
	AddWidget(&textureSlotUvsetField);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void MaterialWindow::SetEntity(Entity entity)
{
	bool changed = this->entity != entity;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	MaterialComponent* material = scene.materials.GetComponent(entity);

	if (material != nullptr)
	{
		SetEnabled(true);

		const NameComponent* name = scene.names.GetComponent(entity);
		if (name == nullptr)
		{
			materialNameField.SetValue("[no_name] " + std::to_string(entity));
		}
		else if (name->name.empty())
		{
			materialNameField.SetValue("[name_empty] " + std::to_string(entity));
		}
		else
		{
			materialNameField.SetValue(name->name);
		}
		shadowReceiveCheckBox.SetCheck(material->IsReceiveShadow());
		shadowCasterCheckBox.SetCheck(material->IsCastingShadow());
		useVertexColorsCheckBox.SetCheck(material->IsUsingVertexColors());
		specularGlossinessCheckBox.SetCheck(material->IsUsingSpecularGlossinessWorkflow());
		occlusionPrimaryCheckBox.SetCheck(material->IsOcclusionEnabled_Primary());
		occlusionSecondaryCheckBox.SetCheck(material->IsOcclusionEnabled_Secondary());
		windCheckBox.SetCheck(material->IsUsingWind());
		doubleSidedCheckBox.SetCheck(material->IsDoubleSided());
		outlineCheckBox.SetCheck(material->IsOutlineEnabled());
		preferUncompressedCheckBox.SetCheck(material->IsPreferUncompressedTexturesEnabled());
		normalMapSlider.SetValue(material->normalMapStrength);
		roughnessSlider.SetValue(material->roughness);
		reflectanceSlider.SetValue(material->reflectance);
		metalnessSlider.SetValue(material->metalness);
		transmissionSlider.SetValue(material->transmission);
		refractionSlider.SetValue(material->refraction);
		emissiveSlider.SetValue(material->emissiveColor.w);
		pomSlider.SetValue(material->parallaxOcclusionMapping);
		anisotropyStrengthSlider.SetValue(material->anisotropy_strength);
		anisotropyRotationSlider.SetValue(wi::math::RadiansToDegrees(material->anisotropy_rotation));
		displacementMappingSlider.SetValue(material->displacementMapping);
		subsurfaceScatteringSlider.SetValue(material->subsurfaceScattering.w);
		texAnimFrameRateSlider.SetValue(material->texAnimFrameRate);
		texAnimDirectionSliderU.SetValue(material->texAnimDirection.x);
		texAnimDirectionSliderV.SetValue(material->texAnimDirection.y);
		texMulSliderX.SetValue(material->texMulAdd.x);
		texMulSliderY.SetValue(material->texMulAdd.y);
		alphaRefSlider.SetValue(material->alphaRef);
		blendModeComboBox.SetSelected((int)material->userBlendMode);

		shaderTypeComboBox.ClearItems();
		shaderTypeComboBox.AddItem("PBR", MaterialComponent::SHADERTYPE_PBR);
		shaderTypeComboBox.AddItem("Planar reflections", MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION);
		shaderTypeComboBox.AddItem("Par. occl. mapping", MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING);
		shaderTypeComboBox.AddItem("Anisotropic", MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC);
		shaderTypeComboBox.AddItem("Cloth", MaterialComponent::SHADERTYPE_PBR_CLOTH);
		shaderTypeComboBox.AddItem("Clear coat", MaterialComponent::SHADERTYPE_PBR_CLEARCOAT);
		shaderTypeComboBox.AddItem("Cloth + Clear coat", MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT);
		shaderTypeComboBox.AddItem("Water", MaterialComponent::SHADERTYPE_WATER);
		shaderTypeComboBox.AddItem("Cartoon", MaterialComponent::SHADERTYPE_CARTOON);
		shaderTypeComboBox.AddItem("Unlit", MaterialComponent::SHADERTYPE_UNLIT);
		for (auto& x : wi::renderer::GetCustomShaders())
		{
			shaderTypeComboBox.AddItem("*" + x.name);
		}
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
			colorPicker.SetPickColor(wi::Color::fromFloat4(material->baseColor));
			break;
		case 1:
			colorPicker.SetPickColor(wi::Color::fromFloat4(material->specularColor));
			break;
		case 2:
			colorPicker.SetPickColor(wi::Color::fromFloat3(XMFLOAT3(material->emissiveColor.x, material->emissiveColor.y, material->emissiveColor.z)));
			break;
		case 3:
			colorPicker.SetPickColor(wi::Color::fromFloat3(XMFLOAT3(material->subsurfaceScattering.x, material->subsurfaceScattering.y, material->subsurfaceScattering.z)));
			break;
		case 4:
			colorPicker.SetPickColor(wi::Color::fromFloat3(XMFLOAT3(material->sheenColor.x, material->sheenColor.y, material->sheenColor.z)));
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

		shadingRateComboBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING));

		if (material->IsUsingSpecularGlossinessWorkflow())
		{
			reflectanceSlider.SetEnabled(false);
			metalnessSlider.SetEnabled(false);
		}

		int slot = textureSlotComboBox.GetSelected();
		textureSlotButton.SetImage(material->textures[slot].resource);
		textureSlotLabel.SetText(wi::helper::GetFileNameFromPath(material->textures[slot].name));
		textureSlotUvsetField.SetText(std::to_string(material->textures[slot].uvset));
		if (changed)
		{
			textureSlotComboBox.SetSelected(slot);
		}
	}
	else
	{
		materialNameField.SetValue("No material selected");
		SetEnabled(false);
		colorComboBox.SetEnabled(false);
		colorPicker.SetEnabled(false);

		textureSlotButton.SetImage(wi::Resource());
		textureSlotLabel.SetText("");
		textureSlotUvsetField.SetText("");
	}

}


void MaterialWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 150;
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

	add_fullwidth(materialNameField);
	add_right(shadowReceiveCheckBox);
	add_right(shadowCasterCheckBox);
	add_right(useVertexColorsCheckBox);
	add_right(specularGlossinessCheckBox);
	add_right(occlusionPrimaryCheckBox);
	add_right(occlusionSecondaryCheckBox);
	add_right(windCheckBox);
	add_right(doubleSidedCheckBox);
	add_right(outlineCheckBox);
	add_right(preferUncompressedCheckBox);
	add(shaderTypeComboBox);
	add(blendModeComboBox);
	add(shadingRateComboBox);
	add(alphaRefSlider);
	add(normalMapSlider);
	add(roughnessSlider);
	add(reflectanceSlider);
	add(metalnessSlider);
	add(emissiveSlider);
	add(transmissionSlider);
	add(refractionSlider);
	add(pomSlider);
	add(anisotropyStrengthSlider);
	add(anisotropyRotationSlider);
	add(displacementMappingSlider);
	add(subsurfaceScatteringSlider);
	add(texAnimFrameRateSlider);
	add(texAnimDirectionSliderU);
	add(texAnimDirectionSliderV);
	add(texMulSliderX);
	add(texMulSliderY);
	add(sheenRoughnessSlider);
	add(clearcoatSlider);
	add(clearcoatRoughnessSlider);
	add(colorComboBox);
	add_fullwidth(colorPicker);
	add(textureSlotComboBox);
	add_fullwidth(textureSlotButton);
	add_fullwidth(textureSlotLabel);
	textureSlotLabel.SetSize(XMFLOAT2(textureSlotLabel.GetSize().x - textureSlotLabel.GetSize().y - 2, textureSlotLabel.GetSize().y));
	textureSlotUvsetField.SetPos(XMFLOAT2(textureSlotLabel.GetPos().x + textureSlotLabel.GetSize().x + 2, textureSlotLabel.GetPos().y));

}
