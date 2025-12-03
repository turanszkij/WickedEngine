#include "stdafx.h"
#include "MaterialWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

MaterialComponent* get_material(Scene& scene, PickResult x)
{
	MaterialComponent* material = scene.materials.GetComponent(x.entity);
	if (material == nullptr)
	{
		// Material could be selected indirectly as part of selected subset:
		ObjectComponent* object = scene.objects.GetComponent(x.entity);
		if (object != nullptr && object->meshID != INVALID_ENTITY)
		{
			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh != nullptr)
			{
				Entity materialID = mesh->subsets[x.subsetIndex].materialID;
				material = scene.materials.GetComponent(materialID);
			}
		}
	}
	return material;
};

void MaterialWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_MATERIAL " Material", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(300, 1580));

	closeButton.SetTooltip("Delete MaterialComponent");
	OnClose([this](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().materials.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float hei = 18;
	float step = hei + 2;
	float x = 150, y = 0;
	float wid = 130;

	auto forEachSelected = [this] (auto func) {
		return [=] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				MaterialComponent* material = get_material(scene, x);
				if (material != nullptr)
					func(material, args);
			}
		};
	};

	shadowReceiveCheckBox.Create("Receive Shadow: ");
	shadowReceiveCheckBox.SetTooltip("Receives shadow or not?");
	shadowReceiveCheckBox.SetPos(XMFLOAT2(x, y));
	shadowReceiveCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowReceiveCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetReceiveShadow(args.bValue);
	}));
	AddWidget(&shadowReceiveCheckBox);

	shadowCasterCheckBox.Create("Cast Shadow: ");
	shadowCasterCheckBox.SetTooltip("The subset will contribute to the scene shadows if enabled.");
	shadowCasterCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCasterCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCasterCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetCastShadow(args.bValue);
	}));
	AddWidget(&shadowCasterCheckBox);

	useVertexColorsCheckBox.Create("Use vertex colors: ");
	useVertexColorsCheckBox.SetTooltip("Enable if you want to render the mesh with vertex colors (must have appropriate vertex buffer)");
	useVertexColorsCheckBox.SetPos(XMFLOAT2(x, y += step));
	useVertexColorsCheckBox.SetSize(XMFLOAT2(hei, hei));
	useVertexColorsCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetUseVertexColors(args.bValue);
	}));
	AddWidget(&useVertexColorsCheckBox);

	specularGlossinessCheckBox.Create("Spec-gloss workflow: ");
	specularGlossinessCheckBox.SetTooltip("If enabled, surface map will be viewed like it contains specular color (RGB) and smoothness (A)");
	specularGlossinessCheckBox.SetPos(XMFLOAT2(x, y += step));
	specularGlossinessCheckBox.SetSize(XMFLOAT2(hei, hei));
	specularGlossinessCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetUseSpecularGlossinessWorkflow(args.bValue);
	}));
	AddWidget(&specularGlossinessCheckBox);

	occlusionPrimaryCheckBox.Create("Occlusion 1: ");
	occlusionPrimaryCheckBox.SetTooltip("If enabled, surface map's RED channel will be used as occlusion map");
	occlusionPrimaryCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionPrimaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionPrimaryCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetOcclusionEnabled_Primary(args.bValue);
	}));
	AddWidget(&occlusionPrimaryCheckBox);

	occlusionSecondaryCheckBox.Create("Occlusion 2: ");
	occlusionSecondaryCheckBox.SetTooltip("If enabled, occlusion map's RED channel will be used as occlusion map");
	occlusionSecondaryCheckBox.SetPos(XMFLOAT2(x, y += step));
	occlusionSecondaryCheckBox.SetSize(XMFLOAT2(hei, hei));
	occlusionSecondaryCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetOcclusionEnabled_Secondary(args.bValue);
	}));
	AddWidget(&occlusionSecondaryCheckBox);

	vertexAOCheckBox.Create("Vertex AO: ");
	vertexAOCheckBox.SetTooltip("If enabled, vertex ambient occlusion will be enabled (if it exists)");
	vertexAOCheckBox.SetPos(XMFLOAT2(x, y += step));
	vertexAOCheckBox.SetSize(XMFLOAT2(hei, hei));
	vertexAOCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetVertexAODisabled(!args.bValue);
	}));
	AddWidget(&vertexAOCheckBox);

	windCheckBox.Create("Wind: ");
	windCheckBox.SetTooltip("If enabled, vertex wind weights will affect how much wind offset affects the subset.");
	windCheckBox.SetPos(XMFLOAT2(x, y += step));
	windCheckBox.SetSize(XMFLOAT2(hei, hei));
	windCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetUseWind(args.bValue);
	}));
	AddWidget(&windCheckBox);

	doubleSidedCheckBox.Create("Double sided: ");
	doubleSidedCheckBox.SetTooltip("Decide whether to render both sides of the material (It's also possible to set this behaviour per mesh).");
	doubleSidedCheckBox.SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetDoubleSided(args.bValue);
	}));
	AddWidget(&doubleSidedCheckBox);

	outlineCheckBox.Create("Cartoon Outline: ");
	outlineCheckBox.SetTooltip("Enable cartoon outline. The Cartoon Outline graphics setting also needs to be enabled for it to show up.");
	outlineCheckBox.SetPos(XMFLOAT2(x, y += step));
	outlineCheckBox.SetSize(XMFLOAT2(hei, hei));
	outlineCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetOutlineEnabled(args.bValue);
	}));
	AddWidget(&outlineCheckBox);

	preferUncompressedCheckBox.Create("Prefer Uncompressed Textures: ");
	preferUncompressedCheckBox.SetTooltip("For uncompressed textures (jpg, png, etc.) here it is possible to enable/disable auto block compression on importing. \nBlock compression can reduce GPU memory usage and improve performance, but it can result in degraded quality.");
	preferUncompressedCheckBox.SetPos(XMFLOAT2(x, y += step));
	preferUncompressedCheckBox.SetSize(XMFLOAT2(hei, hei));
	preferUncompressedCheckBox.OnClick(forEachSelected([this] (auto material, auto args) {
		material->SetPreferUncompressedTexturesEnabled(args.bValue);
		textureSlotComboBox.SetSelected(textureSlotComboBox.GetSelected()); // update
	}));
	AddWidget(&preferUncompressedCheckBox);

	disableStreamingCheckBox.Create("Disable Texture Streaming: ");
	disableStreamingCheckBox.SetTooltip("Disable texture streaming for this material only.");
	disableStreamingCheckBox.SetPos(XMFLOAT2(x, y += step));
	disableStreamingCheckBox.SetSize(XMFLOAT2(hei, hei));
	disableStreamingCheckBox.OnClick(forEachSelected([this] (auto material, auto args) {
		material->SetTextureStreamingDisabled(args.bValue);
		material->CreateRenderData(true);
		textureSlotComboBox.SetSelected(textureSlotComboBox.GetSelected()); // update
	}));
	AddWidget(&disableStreamingCheckBox);

	coplanarCheckBox.Create("Coplanar blending: ");
	coplanarCheckBox.SetTooltip("If polygons are coplanar to an opaque surface, then the blending can be done in the opaque pass.\nThis can enable some benefits of opaque render pass to a specific transparent surface.");
	coplanarCheckBox.SetPos(XMFLOAT2(x, y += step));
	coplanarCheckBox.SetSize(XMFLOAT2(hei, hei));
	coplanarCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetCoplanarBlending(args.bValue);
	}));
	AddWidget(&coplanarCheckBox);

	capsuleShadowCheckBox.Create("Capsule Shadow Disabled: ");
	capsuleShadowCheckBox.SetTooltip("Disable receiving capsule shadows for this material.");
	capsuleShadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	capsuleShadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	capsuleShadowCheckBox.OnClick(forEachSelected([] (auto material, auto args) {
		material->SetCapsuleShadowDisabled(args.bValue);
	}));
	AddWidget(&capsuleShadowCheckBox);


	shaderTypeComboBox.Create("Shader: ");
	shaderTypeComboBox.SetTooltip("Select a shader for this material. \nCustom shaders (*) will also show up here (see wi::renderer:RegisterCustomShader() for more info.)\nNote that custom shaders (*) can't select between blend modes, as they are created with an explicit blend mode.");
	shaderTypeComboBox.SetPos(XMFLOAT2(x, y += step));
	shaderTypeComboBox.SetSize(XMFLOAT2(wid, hei));
	shaderTypeComboBox.OnSelect(forEachSelected([this] (auto material, auto args) {
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
	}));
	shaderTypeComboBox.SetEnabled(false);
	shaderTypeComboBox.SetMaxVisibleItemCount(5);
	AddWidget(&shaderTypeComboBox);

	blendModeComboBox.Create("Blend mode: ");
	blendModeComboBox.SetPos(XMFLOAT2(x, y += step));
	blendModeComboBox.SetSize(XMFLOAT2(wid, hei));
	blendModeComboBox.OnSelect(forEachSelected([] (auto material, auto args) {
		material->userBlendMode = (wi::enums::BLENDMODE)args.userdata;
	}));
	blendModeComboBox.AddItem("Opaque", wi::enums::BLENDMODE_OPAQUE);
	blendModeComboBox.AddItem("Alpha", wi::enums::BLENDMODE_ALPHA);
	blendModeComboBox.AddItem("Premultiplied", wi::enums::BLENDMODE_PREMULTIPLIED);
	blendModeComboBox.AddItem("Additive", wi::enums::BLENDMODE_ADDITIVE);
	blendModeComboBox.AddItem("Multiply", wi::enums::BLENDMODE_MULTIPLY);
	blendModeComboBox.AddItem("Inverse", wi::enums::BLENDMODE_INVERSE);
	blendModeComboBox.SetEnabled(false);
	blendModeComboBox.SetTooltip("Set the blend mode of the material.");
	AddWidget(&blendModeComboBox);

	shadingRateComboBox.Create("Shading Rate: ");
	shadingRateComboBox.SetTooltip("Select shading rate for this material. \nSelecting larger shading rate will decrease rendering quality of this material, \nbut increases performance.\nRequires hardware support for variable shading rate");
	shadingRateComboBox.SetPos(XMFLOAT2(x, y += step));
	shadingRateComboBox.SetSize(XMFLOAT2(wid, hei));
	shadingRateComboBox.OnSelect(forEachSelected([] (auto material, auto args) {
		material->shadingRate = (ShadingRate)args.iValue;
	}));
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


	cameraComboBox.Create("Camera source: ");
	cameraComboBox.SetTooltip("Select a camera to use as texture");
	cameraComboBox.OnSelect(forEachSelected([this] (auto material, auto args) {
		material->cameraSource = (Entity)args.userdata;

		auto& scene = editor->GetCurrentScene();
		CameraComponent* camera = scene.cameras.GetComponent((Entity)args.userdata);
		if (camera != nullptr)
		{
			camera->render_to_texture.resolution = XMUINT2(256, 256);
		}
	}));
	AddWidget(&cameraComboBox);




	// Sliders:

	normalMapSlider.Create(0, 4, 1, 4000, "Normalmap: ");
	normalMapSlider.SetTooltip("How much the normal map should distort the face normals (bumpiness).");
	normalMapSlider.SetSize(XMFLOAT2(wid, hei));
	normalMapSlider.SetPos(XMFLOAT2(x, y += step));
	normalMapSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetNormalMapStrength(args.fValue);
	}));
	AddWidget(&normalMapSlider);

	roughnessSlider.Create(0, 1, 0.5f, 1000, "Roughness: ");
	roughnessSlider.SetTooltip("Adjust the surface roughness. Rough surfaces are less shiny, more matte.");
	roughnessSlider.SetSize(XMFLOAT2(wid, hei));
	roughnessSlider.SetPos(XMFLOAT2(x, y += step));
	roughnessSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetRoughness(args.fValue);
	}));
	AddWidget(&roughnessSlider);

	reflectanceSlider.Create(0, 1, 0.5f, 1000, "Reflectance: ");
	reflectanceSlider.SetTooltip("Adjust the surface [non-metal] reflectivity (also called specularFactor).\nNote: this is not available in specular-glossiness workflow");
	reflectanceSlider.SetSize(XMFLOAT2(wid, hei));
	reflectanceSlider.SetPos(XMFLOAT2(x, y += step));
	reflectanceSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetReflectance(args.fValue);
	}));
	AddWidget(&reflectanceSlider);

	metalnessSlider.Create(0, 1, 0.0f, 1000, "Metalness: ");
	metalnessSlider.SetTooltip("The more metal-like the surface is, the more the its color will contribute to the reflection color.\nNote: this is not available in specular-glossiness workflow");
	metalnessSlider.SetSize(XMFLOAT2(wid, hei));
	metalnessSlider.SetPos(XMFLOAT2(x, y += step));
	metalnessSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetMetalness(args.fValue);
	}));
	AddWidget(&metalnessSlider);

	alphaRefSlider.Create(0, 1, 1.0f, 1000, "AlphaRef: ");
	alphaRefSlider.SetTooltip("Adjust the alpha cutoff threshold. Alpha cutout can affect performance");
	alphaRefSlider.SetSize(XMFLOAT2(wid, hei));
	alphaRefSlider.SetPos(XMFLOAT2(x, y += step));
	alphaRefSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetAlphaRef(args.fValue);
	}));
	AddWidget(&alphaRefSlider);

	emissiveSlider.Create(0, 10, 0.0f, 1000, "Emissive: ");
	emissiveSlider.SetTooltip("Adjust the light emission of the surface. The color of the light emitted is that of the color of the material.");
	emissiveSlider.SetSize(XMFLOAT2(wid, hei));
	emissiveSlider.SetPos(XMFLOAT2(x, y += step));
	emissiveSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetEmissiveStrength(args.fValue);
	}));
	AddWidget(&emissiveSlider);

	saturationSlider.Create(0, 2, 1, 1000, "Saturation: ");
	saturationSlider.SetTooltip("Adjust the saturation of the material.");
	saturationSlider.SetSize(XMFLOAT2(wid, hei));
	saturationSlider.SetPos(XMFLOAT2(x, y += step));
	saturationSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetSaturation(args.fValue);
	}));
	AddWidget(&saturationSlider);

	cloakSlider.Create(0, 1.0f, 0.02f, 1000, "Cloak: ");
	cloakSlider.SetTooltip("The cloak effect is a combination of transmission, refraction and roughness, without color tinging.");
	cloakSlider.SetSize(XMFLOAT2(wid, hei));
	cloakSlider.SetPos(XMFLOAT2(x, y += step));
	cloakSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetCloakAmount(args.fValue);
	}));
	AddWidget(&cloakSlider);

	chromaticAberrationSlider.Create(0, 10.0f, 0, 1000, "Chromatic aberration: ");
	chromaticAberrationSlider.SetTooltip("Separation of RGB colors inside transmissive material.");
	chromaticAberrationSlider.SetSize(XMFLOAT2(wid, hei));
	chromaticAberrationSlider.SetPos(XMFLOAT2(x, y += step));
	chromaticAberrationSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetChromaticAberrationAmount(args.fValue);
	}));
	AddWidget(&chromaticAberrationSlider);

	transmissionSlider.Create(0, 1.0f, 0.02f, 1000, "Transmission: ");
	transmissionSlider.SetTooltip("Adjust the transmissiveness. More transmissiveness means more diffuse light is transmitted instead of absorbed.");
	transmissionSlider.SetSize(XMFLOAT2(wid, hei));
	transmissionSlider.SetPos(XMFLOAT2(x, y += step));
	transmissionSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetTransmissionAmount(args.fValue);
	}));
	AddWidget(&transmissionSlider);

	refractionSlider.Create(0, 1, 0, 1000, "Refraction: ");
	refractionSlider.SetTooltip("Adjust the refraction amount for transmissive materials.");
	refractionSlider.SetSize(XMFLOAT2(wid, hei));
	refractionSlider.SetPos(XMFLOAT2(x, y += step));
	refractionSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetRefractionAmount(args.fValue);
	}));
	AddWidget(&refractionSlider);

	pomSlider.Create(0, 1.0f, 0.0f, 1000, "Par Occl Mapping: ");
	pomSlider.SetTooltip("[Parallax Occlusion Mapping] Adjust how much the bump map should modulate the surface parallax effect. \nOnly works with PBR + Parallax shader.");
	pomSlider.SetSize(XMFLOAT2(wid, hei));
	pomSlider.SetPos(XMFLOAT2(x, y += step));
	pomSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetParallaxOcclusionMapping(args.fValue);
	}));
	AddWidget(&pomSlider);

	anisotropyStrengthSlider.Create(0, 1.0f, 0.0f, 1000, "Anisotropy Strength: ");
	anisotropyStrengthSlider.SetTooltip("Adjust anisotropy specular effect's strength. \nOnly works with PBR + Anisotropic shader.");
	anisotropyStrengthSlider.SetSize(XMFLOAT2(wid, hei));
	anisotropyStrengthSlider.SetPos(XMFLOAT2(x, y += step));
	anisotropyStrengthSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->anisotropy_strength = args.fValue;
	}));
	AddWidget(&anisotropyStrengthSlider);

	anisotropyRotationSlider.Create(0, 360, 0.0f, 360, "Anisotropy Rot: ");
	anisotropyRotationSlider.SetTooltip("Adjust anisotropy specular effect's rotation. \nOnly works with PBR + Anisotropic shader.");
	anisotropyRotationSlider.SetSize(XMFLOAT2(wid, hei));
	anisotropyRotationSlider.SetPos(XMFLOAT2(x, y += step));
	anisotropyRotationSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->anisotropy_rotation = wi::math::DegreesToRadians(args.fValue);
	}));
	AddWidget(&anisotropyRotationSlider);


	displacementMappingSlider.Create(0, 10.0f, 0.0f, 1000, "Displacement: ");
	displacementMappingSlider.SetTooltip("Adjust how much the bump map should modulate the geometry when using tessellation.");
	displacementMappingSlider.SetSize(XMFLOAT2(wid, hei));
	displacementMappingSlider.SetPos(XMFLOAT2(x, y += step));
	displacementMappingSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetDisplacementMapping(args.fValue);
	}));
	AddWidget(&displacementMappingSlider);

	subsurfaceScatteringSlider.Create(0, 2, 0.0f, 1000, "Subsurface Scattering: ");
	subsurfaceScatteringSlider.SetTooltip("Subsurface scattering amount. \nYou can also adjust the subsurface color by selecting it in the color picker");
	subsurfaceScatteringSlider.SetSize(XMFLOAT2(wid, hei));
	subsurfaceScatteringSlider.SetPos(XMFLOAT2(x, y += step));
	subsurfaceScatteringSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetSubsurfaceScatteringAmount(args.fValue);
	}));
	AddWidget(&subsurfaceScatteringSlider);

	texAnimFrameRateSlider.Create(0, 60, 0, 60, "Texcoord anim FPS: ");
	texAnimFrameRateSlider.SetTooltip("Adjust the texture animation frame rate (frames per second). Any value above 0 will make the material dynamic.");
	texAnimFrameRateSlider.SetSize(XMFLOAT2(wid, hei));
	texAnimFrameRateSlider.SetPos(XMFLOAT2(x, y += step));
	texAnimFrameRateSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->texAnimFrameRate = args.fValue;
	}));
	AddWidget(&texAnimFrameRateSlider);

	texAnimDirectionSliderU.Create(-0.05f, 0.05f, 0, 1000, "Texcoord anim U: ");
	texAnimDirectionSliderU.SetTooltip("Adjust the texture animation speed along the U direction in texture space.");
	texAnimDirectionSliderU.SetSize(XMFLOAT2(wid, hei));
	texAnimDirectionSliderU.SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderU.OnSlide(forEachSelected([] (auto material, auto args) {
		material->texAnimDirection.x = args.fValue;
	}));
	AddWidget(&texAnimDirectionSliderU);

	texAnimDirectionSliderV.Create(-0.05f, 0.05f, 0, 1000, "Texcoord anim V: ");
	texAnimDirectionSliderV.SetTooltip("Adjust the texture animation speed along the V direction in texture space.");
	texAnimDirectionSliderV.SetSize(XMFLOAT2(wid, hei));
	texAnimDirectionSliderV.SetPos(XMFLOAT2(x, y += step));
	texAnimDirectionSliderV.OnSlide(forEachSelected([] (auto material, auto args) {
		material->texAnimDirection.y = args.fValue;
	}));
	AddWidget(&texAnimDirectionSliderV);

	texMulSliderX.Create(0.01f, 10.0f, 0, 1000, "Texture TileSize X: ");
	texMulSliderX.SetTooltip("Adjust the texture mapping size.");
	texMulSliderX.SetSize(XMFLOAT2(wid, hei));
	texMulSliderX.SetPos(XMFLOAT2(x, y += step));
	texMulSliderX.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetDirty();
			material->texMulAdd.x = args.fValue;
	}));
	AddWidget(&texMulSliderX);

	texMulSliderY.Create(0.01f, 10.0f, 0, 1000, "Texture TileSize Y: ");
	texMulSliderY.SetTooltip("Adjust the texture mapping size.");
	texMulSliderY.SetSize(XMFLOAT2(wid, hei));
	texMulSliderY.SetPos(XMFLOAT2(x, y += step));
	texMulSliderY.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetDirty();
			material->texMulAdd.y = args.fValue;
	}));
	AddWidget(&texMulSliderY);


	sheenRoughnessSlider.Create(0, 1, 0, 1000, "Sheen Roughness: ");
	sheenRoughnessSlider.SetTooltip("This affects roughness of sheen layer for cloth shading.");
	sheenRoughnessSlider.SetSize(XMFLOAT2(wid, hei));
	sheenRoughnessSlider.SetPos(XMFLOAT2(x, y += step));
	sheenRoughnessSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetSheenRoughness(args.fValue);
	}));
	AddWidget(&sheenRoughnessSlider);

	clearcoatSlider.Create(0, 1, 0, 1000, "Clearcoat: ");
	clearcoatSlider.SetTooltip("This affects clearcoat layer blending.");
	clearcoatSlider.SetSize(XMFLOAT2(wid, hei));
	clearcoatSlider.SetPos(XMFLOAT2(x, y += step));
	clearcoatSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetClearcoatFactor(args.fValue);
	}));
	AddWidget(&clearcoatSlider);

	clearcoatRoughnessSlider.Create(0, 1, 0, 1000, "Clearcoat Roughness: ");
	clearcoatRoughnessSlider.SetTooltip("This affects roughness of clear coat layer.");
	clearcoatRoughnessSlider.SetSize(XMFLOAT2(wid, hei));
	clearcoatRoughnessSlider.SetPos(XMFLOAT2(x, y += step));
	clearcoatRoughnessSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetClearcoatRoughness(args.fValue);
	}));
	AddWidget(&clearcoatRoughnessSlider);

	blendTerrainSlider.Create(0, 2, 0, 1000, "Blend with terrain: ");
	blendTerrainSlider.SetTooltip("Blend with terrain height.");
	blendTerrainSlider.SetSize(XMFLOAT2(wid, hei));
	blendTerrainSlider.SetPos(XMFLOAT2(x, y += step));
	blendTerrainSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetBlendWithTerrainHeight(args.fValue);
	}));
	AddWidget(&blendTerrainSlider);

	meshblendSlider.Create(0, 2, 0, 1000, "Mesh Blend: ");
	meshblendSlider.SetTooltip("Screen space Mesh Blend post process distance falloff.");
	meshblendSlider.SetSize(XMFLOAT2(wid, hei));
	meshblendSlider.SetPos(XMFLOAT2(x, y += step));
	meshblendSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetMeshBlend(args.fValue);
	}));
	AddWidget(&meshblendSlider);

	interiorScaleXSlider.Create(1, 10, 1, 1000, "Interior Scale X: ");
	interiorScaleXSlider.SetTooltip("Set the cubemap scale for the interior mapping (if material uses interior mapping shader)");
	interiorScaleXSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingScale(XMFLOAT3(args.fValue, material->interiorMappingScale.y, material->interiorMappingScale.z));
	}));
	AddWidget(&interiorScaleXSlider);

	interiorScaleYSlider.Create(1, 10, 1, 1000, "Interior Scale Y: ");
	interiorScaleYSlider.SetTooltip("Set the cubemap scale for the interior mapping (if material uses interior mapping shader)");
	interiorScaleYSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingScale(XMFLOAT3(material->interiorMappingScale.x, args.fValue, material->interiorMappingScale.z));
	}));
	AddWidget(&interiorScaleYSlider);

	interiorScaleZSlider.Create(1, 10, 1, 1000, "Interior Scale Z: ");
	interiorScaleZSlider.SetTooltip("Set the cubemap scale for the interior mapping (if material uses interior mapping shader)");
	interiorScaleZSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingScale(XMFLOAT3(material->interiorMappingScale.x, material->interiorMappingScale.y, args.fValue));
	}));
	AddWidget(&interiorScaleZSlider);

	interiorOffsetXSlider.Create(-10, 10, 0, 2000, "Interior Offset X: ");
	interiorOffsetXSlider.SetTooltip("Set the cubemap offset for the interior mapping (if material uses interior mapping shader)");
	interiorOffsetXSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingOffset(XMFLOAT3(args.fValue, material->interiorMappingOffset.y, material->interiorMappingOffset.z));
	}));
	AddWidget(&interiorOffsetXSlider);

	interiorOffsetYSlider.Create(-10, 10, 0, 2000, "Interior Offset Y: ");
	interiorOffsetYSlider.SetTooltip("Set the cubemap offset for the interior mapping (if material uses interior mapping shader)");
	interiorOffsetYSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingOffset(XMFLOAT3(material->interiorMappingOffset.x, args.fValue, material->interiorMappingOffset.z));
	}));
	AddWidget(&interiorOffsetYSlider);

	interiorOffsetZSlider.Create(-10, 10, 0, 2000, "Interior Offset Z: ");
	interiorOffsetZSlider.SetTooltip("Set the cubemap offset for the interior mapping (if material uses interior mapping shader)");
	interiorOffsetZSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		material->SetInteriorMappingOffset(XMFLOAT3(material->interiorMappingOffset.x, material->interiorMappingOffset.y, args.fValue));
	}));
	AddWidget(&interiorOffsetZSlider);

	interiorRotationSlider.Create(0, 360, 0, 360, "Interior Rotation: ");
	interiorRotationSlider.SetTooltip("Set the cubemap horizontal rotation for the interior mapping (if material uses interior mapping shader)");
	interiorRotationSlider.OnSlide(forEachSelected([] (auto material, auto args) {
		float radians = wi::math::DegreesToRadians(args.fValue);
		material->SetInteriorMappingRotation(radians);
	}));
	AddWidget(&interiorRotationSlider);


	hei = 20;
	step = hei + 2;
	x = 10;

	materialNameField.Create("MaterialName");
	materialNameField.SetTooltip("Set a name for the material...");
	materialNameField.SetPos(XMFLOAT2(10, y += step));
	materialNameField.SetSize(XMFLOAT2(300, hei));
	materialNameField.OnInputAccepted([=](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			NameComponent* name = scene.names.GetComponent(x.entity);
			if (name == nullptr)
				continue;
			*name = args.sValue;

			editor->componentsWnd.RefreshEntityTree();
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
	colorComboBox.AddItem("Extinction color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	colorPicker.SetPos(XMFLOAT2(10, y += step));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged(forEachSelected([this] (auto material, auto args) {
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
			case 5:
				material->SetExtinctionColor(args.color.toFloat4());
				break;
			}
	}));
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
		case MaterialComponent::TRANSPARENCYMAP:
			textureSlotComboBox.AddItem("Transparency map");
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
		case MaterialComponent::TRANSPARENCYMAP:
			tooltiptext = "R: transparency.";
			break;
		default:
			break;
		}

		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material != nullptr)
		{
			textureSlotImage.SetImage(material->textures[args.iValue].resource);
			if (material->textures[args.iValue].resource.IsValid())
			{
				const Texture& texture = material->textures[args.iValue].resource.GetTexture();
				tooltiptext += "\nResolution: " + std::to_string(texture.desc.width) + " * " + std::to_string(texture.desc.height);
				if (texture.desc.array_size > 0)
				{
					tooltiptext += " * " + std::to_string(texture.desc.array_size);
				}
				if (has_flag(texture.desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
				{
					tooltiptext += " (cubemap)";
				}
				tooltiptext += "\nMip levels: " + std::to_string(texture.desc.mip_levels);
				tooltiptext += "\nFormat: ";
				tooltiptext += GetFormatString(texture.desc.format);
				tooltiptext += "\nSwizzle: ";
				tooltiptext += GetSwizzleString(texture.desc.swizzle);
				tooltiptext += "\nMemory: " + wi::helper::GetMemorySizeText(ComputeTextureMemorySizeInBytes(texture.desc));
			}
		}

		textureSlotImage.SetTooltip(tooltiptext);

	});
	textureSlotComboBox.SetSelected(0);
	textureSlotComboBox.SetTooltip("Choose the texture slot to modify.");
	AddWidget(&textureSlotComboBox);

	textureSlotLoadButton.Create("Load from file");
	textureSlotLoadButton.SetSize(XMFLOAT2(wid, hei));
	textureSlotLoadButton.SetPos(XMFLOAT2(x, y += step));
	textureSlotLoadButton.SetTooltip("Load texture from file on disk");
	textureSlotLoadButton.OnClick([this](wi::gui::EventArgs args) {
		int slot = textureSlotComboBox.GetSelected();
		wi::helper::FileDialogParams params;
		params.type = wi::helper::FileDialogParams::OPEN;
		params.description = "Texture";
		params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
		wi::helper::FileDialog(params, [this, slot](std::string fileName) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
				const wi::resourcemanager::Flags flags = material->GetTextureSlotResourceFlags(MaterialComponent::TEXTURESLOT(slot));

				wi::Archive& archive = editor->AdvanceHistory();
				archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
				editor->RecordEntity(archive, entity);

				material->textures[slot].resource = wi::resourcemanager::Load(fileName, flags);
				material->textures[slot].name = fileName;
				material->SetDirty();
				textureSlotLabel.SetText(wi::helper::GetFileNameFromPath(fileName));
				textureSlotComboBox.SetSelected(slot);
				textureSlotImage.SetImage(material->textures[slot].resource);

				editor->RecordEntity(archive, entity);
			});
		});
	});
	AddWidget(&textureSlotLoadButton);

	textureSlotSelectButton.Create("Select texture");
	textureSlotSelectButton.SetSize(XMFLOAT2(wid, hei));
	textureSlotSelectButton.SetPos(XMFLOAT2(x + wid / 3 + 1, y));
	textureSlotSelectButton.SetTooltip("Select texture from current scene");
	textureSlotSelectButton.OnClick([this](wi::gui::EventArgs args) {
		RecreateTexturePickerButtons();
		texturePickerWindow.SetVisible(true);
	});
	AddWidget(&textureSlotSelectButton);

	textureSlotClearButton.Create("Clear texture");
	textureSlotClearButton.SetSize(XMFLOAT2(wid, hei));
	textureSlotClearButton.SetPos(XMFLOAT2(x + 2 * wid / 3 + 2, y));
	textureSlotClearButton.SetTooltip("Clear texture from slot");
	textureSlotClearButton.OnClick([this](wi::gui::EventArgs args) {
		MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
		if (material == nullptr)
			return;

		const int slot = textureSlotComboBox.GetSelected();

		if (material->textures[slot].resource.IsValid())
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			material->textures[slot].resource = {};
			material->textures[slot].name = "";
			material->SetDirty();
			textureSlotLabel.SetText("");
			textureSlotImage.SetImage(wi::Resource());

			editor->RecordEntity(archive, entity);
		}
	});
	AddWidget(&textureSlotClearButton);

	textureSlotImage.Create("");
	textureSlotImage.SetSize(XMFLOAT2(180, 180));
	textureSlotImage.SetPos(XMFLOAT2(textureSlotComboBox.GetPosition().x + textureSlotComboBox.GetScale().x - textureSlotImage.GetScale().x, y += step));
	AddWidget(&textureSlotImage);

	y += textureSlotImage.GetScale().y - step + 2;

	textureSlotLabel.Create("");
	textureSlotLabel.SetPos(XMFLOAT2(x, y += step));
	textureSlotLabel.SetSize(XMFLOAT2(colorPicker.GetScale().x - hei - 2, hei));
	AddWidget(&textureSlotLabel);

	textureSlotUvsetField.Create("uvset");
	textureSlotUvsetField.SetText("");
	textureSlotUvsetField.SetTooltip("uv set number");
	textureSlotUvsetField.SetPos(XMFLOAT2(x + textureSlotLabel.GetScale().x + 2, y));
	textureSlotUvsetField.SetSize(XMFLOAT2(hei, hei));
	textureSlotUvsetField.OnInputAccepted(forEachSelected([this] (auto material, auto args) {
		int slot = textureSlotComboBox.GetSelected();
		material->textures[slot].uvset = (uint32_t)args.iValue;
	}));
	AddWidget(&textureSlotUvsetField);

	// Create texture picker
	const float screenW = editor->GetLogicalWidth();
	const float screenH = editor->GetLogicalHeight();

	auto wctrl = wi::gui::Window::WindowControls::ALL;
	wctrl &= ~wi::gui::Window::WindowControls::RESIZE_BOTTOMLEFT;
	texturePickerWindow.Create("", wctrl);
	texturePickerWindow.SetVisible(false);
	const float texturePickerWindowW = screenW / 3.20f;
	const float texturePickerWindowH = screenH / 2.0f;
	texturePickerWindow.SetSize(XMFLOAT2(texturePickerWindowW, texturePickerWindowH));
	texturePickerWindow.SetPos(XMFLOAT2(screenW / 2.0f - texturePickerWindowW / 2.0f, screenH / 2.0f - texturePickerWindowH / 2.0f));

	texturePickerWindow.OnClose([this](wi::gui::EventArgs args) {
		texturePickerWindow.SetVisible(false);
	});
	editor->GetGUI().AddWidget(&texturePickerWindow);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void MaterialWindow::SetEntity(Entity entity)
{
	const bool changed = this->entity != entity;
	this->entity = entity;

	const Scene& scene = editor->GetCurrentScene();
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
		vertexAOCheckBox.SetCheck(!material->IsVertexAODisabled());
		windCheckBox.SetCheck(material->IsUsingWind());
		doubleSidedCheckBox.SetCheck(material->IsDoubleSided());
		outlineCheckBox.SetCheck(material->IsOutlineEnabled());
		preferUncompressedCheckBox.SetCheck(material->IsPreferUncompressedTexturesEnabled());
		disableStreamingCheckBox.SetCheck(material->IsTextureStreamingDisabled());
		coplanarCheckBox.SetCheck(material->IsCoplanarBlending());
		capsuleShadowCheckBox.SetCheck(material->IsCapsuleShadowDisabled());
		normalMapSlider.SetValue(material->normalMapStrength);
		roughnessSlider.SetValue(material->roughness);
		reflectanceSlider.SetValue(material->reflectance);
		metalnessSlider.SetValue(material->metalness);
		cloakSlider.SetValue(material->cloak);
		chromaticAberrationSlider.SetValue(material->chromatic_aberration);
		transmissionSlider.SetValue(material->transmission);
		refractionSlider.SetValue(material->refraction);
		emissiveSlider.SetValue(material->emissiveColor.w);
		saturationSlider.SetValue(material->saturation);
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
		blendModeComboBox.SetSelectedWithoutCallback((int)material->userBlendMode);

		shaderTypeComboBox.ClearItems();
		shaderTypeComboBox.AddItem("PBR", MaterialComponent::SHADERTYPE_PBR);
		shaderTypeComboBox.AddItem("Planar reflections", MaterialComponent::SHADERTYPE_PBR_PLANARREFLECTION);
		shaderTypeComboBox.AddItem("Par. occl. mapping", MaterialComponent::SHADERTYPE_PBR_PARALLAXOCCLUSIONMAPPING);
		shaderTypeComboBox.AddItem("Anisotropic", MaterialComponent::SHADERTYPE_PBR_ANISOTROPIC);
		shaderTypeComboBox.AddItem("Cloth", MaterialComponent::SHADERTYPE_PBR_CLOTH);
		shaderTypeComboBox.AddItem("Clear coat", MaterialComponent::SHADERTYPE_PBR_CLEARCOAT);
		shaderTypeComboBox.AddItem("Cloth + Clear coat", MaterialComponent::SHADERTYPE_PBR_CLOTH_CLEARCOAT);
		shaderTypeComboBox.AddItem("Terrain blended", MaterialComponent::SHADERTYPE_PBR_TERRAINBLENDED);
		shaderTypeComboBox.AddItem("Water", MaterialComponent::SHADERTYPE_WATER);
		shaderTypeComboBox.AddItem("Cartoon", MaterialComponent::SHADERTYPE_CARTOON);
		shaderTypeComboBox.AddItem("Unlit", MaterialComponent::SHADERTYPE_UNLIT);
		shaderTypeComboBox.AddItem("Interior", MaterialComponent::SHADERTYPE_INTERIORMAPPING);
		for (auto& x : wi::renderer::GetCustomShaders())
		{
			shaderTypeComboBox.AddItem("*" + x.name);
		}
		if (material->GetCustomShaderID() >= 0)
		{
			shaderTypeComboBox.SetSelectedWithoutCallback(MaterialComponent::SHADERTYPE_COUNT + material->GetCustomShaderID());
		}
		else
		{
			shaderTypeComboBox.SetSelectedByUserdataWithoutCallback(material->shaderType);
		}
		shadingRateComboBox.SetSelectedWithoutCallback((int)material->shadingRate);

		cameraComboBox.ClearItems();
		cameraComboBox.AddItem("INVALID_ENTITY", (uint64_t)INVALID_ENTITY);
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			const Entity cameraEntity = scene.cameras.GetEntity(i);
			const NameComponent* name_component = scene.names.GetComponent(cameraEntity);
			if (name_component != nullptr)
			{
				cameraComboBox.AddItem(name_component->name, (uint64_t)cameraEntity);
			}
		}
		cameraComboBox.SetSelectedByUserdataWithoutCallback((uint64_t)material->cameraSource);

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
		case 5:
			colorPicker.SetPickColor(wi::Color::fromFloat4(material->extinctionColor));
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
		default:
			break;
		}
		sheenRoughnessSlider.SetValue(material->sheenRoughness);
		clearcoatSlider.SetValue(material->clearcoat);
		clearcoatRoughnessSlider.SetValue(material->clearcoatRoughness);
		blendTerrainSlider.SetValue(material->blend_with_terrain_height);
		meshblendSlider.SetValue(material->mesh_blend);
		interiorScaleXSlider.SetValue(material->interiorMappingScale.x);
		interiorScaleYSlider.SetValue(material->interiorMappingScale.y);
		interiorScaleZSlider.SetValue(material->interiorMappingScale.z);
		interiorOffsetXSlider.SetValue(material->interiorMappingOffset.x);
		interiorOffsetYSlider.SetValue(material->interiorMappingOffset.y);
		interiorOffsetZSlider.SetValue(material->interiorMappingOffset.z);
		interiorRotationSlider.SetValue(wi::math::RadiansToDegrees(material->interiorMappingRotation));

		shadingRateComboBox.SetEnabled(wi::graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING));

		if (material->IsUsingSpecularGlossinessWorkflow())
		{
			reflectanceSlider.SetEnabled(false);
			metalnessSlider.SetEnabled(false);
		}

		const int slot = textureSlotComboBox.GetSelected();
		textureSlotImage.SetImage(material->textures[slot].resource);
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

		textureSlotImage.SetImage(wi::Resource());
		textureSlotLabel.SetText("");
		textureSlotUvsetField.SetText("");
	}
}

void MaterialWindow::RecreateTexturePickerButtons()
{
	if (editor == nullptr)
		return;

	// Remove old buttons
	for (auto& button : texturePickerButtons)
	{
		texturePickerWindow.RemoveWidget(&button);
	}
	texturePickerButtons.clear();

	// Create buttons for each unique texture
	uniqueTextures.clear();
	wi::resourcemanager::CollectResources(uniqueTextures);
	for (auto& [textureName, textureResource] : uniqueTextures)
	{
		texturePickerButtons.emplace_back();
		wi::gui::Button& button = texturePickerButtons.back();

		button.Create("");
		button.SetImage(textureResource);
		button.SetTooltip(wi::helper::GetFileNameFromPath(textureName));
		texturePickerWindow.AddWidget(&button);
		button.SetVisible(false);

		std::string capturedName = textureName;
		wi::Resource capturedResource = textureResource;

		button.OnClick([this, capturedName, capturedResource](wi::gui::EventArgs args) {
			MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
			if (material == nullptr)
				return;

			const int slot = textureSlotComboBox.GetSelected();

			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			material->textures[slot].resource = capturedResource;
			material->textures[slot].name = capturedName;
			material->SetDirty();
			textureSlotLabel.SetText(wi::helper::GetFileNameFromPath(capturedName));
			textureSlotImage.SetImage(capturedResource);

			editor->RecordEntity(archive, entity);
		});
	}

	ResizeLayout();
}

void MaterialWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 150;

	Scene& scene = editor->GetCurrentScene();
	MaterialComponent* material = scene.materials.GetComponent(entity);

	if (texturePickerWindow.IsVisible())
	{
		uniqueTextures.clear();
		wi::resourcemanager::CollectResources(uniqueTextures);
		if (texturePickerButtons.size() != uniqueTextures.size())
		{
			RecreateTexturePickerButtons();
			return;
		}
	}

	layout.add_fullwidth(materialNameField);
	layout.add_right(shadowReceiveCheckBox);
	layout.add_right(shadowCasterCheckBox);
	layout.add_right(useVertexColorsCheckBox);
	layout.add_right(specularGlossinessCheckBox);
	layout.add_right(occlusionPrimaryCheckBox);
	layout.add_right(occlusionSecondaryCheckBox);
	layout.add_right(vertexAOCheckBox);
	layout.add_right(windCheckBox);
	layout.add_right(doubleSidedCheckBox);
	layout.add_right(outlineCheckBox);
	layout.add_right(preferUncompressedCheckBox);
	layout.add_right(disableStreamingCheckBox);
	layout.add_right(coplanarCheckBox);
	layout.add_right(capsuleShadowCheckBox);
	layout.add(shaderTypeComboBox);
	layout.add(blendModeComboBox);
	layout.add(shadingRateComboBox);
	layout.add(cameraComboBox);
	layout.add(alphaRefSlider);
	layout.add(normalMapSlider);
	layout.add(roughnessSlider);
	layout.add(reflectanceSlider);
	layout.add(metalnessSlider);
	layout.add(emissiveSlider);
	layout.add(saturationSlider);
	layout.add(cloakSlider);
	layout.add(chromaticAberrationSlider);
	layout.add(transmissionSlider);
	layout.add(refractionSlider);
	layout.add(pomSlider);
	layout.add(anisotropyStrengthSlider);
	layout.add(anisotropyRotationSlider);
	layout.add(displacementMappingSlider);
	layout.add(subsurfaceScatteringSlider);
	layout.add(texAnimFrameRateSlider);
	layout.add(texAnimDirectionSliderU);
	layout.add(texAnimDirectionSliderV);
	layout.add(texMulSliderX);
	layout.add(texMulSliderY);
	layout.add(sheenRoughnessSlider);
	layout.add(clearcoatSlider);
	layout.add(clearcoatRoughnessSlider);
	layout.add(blendTerrainSlider);
	layout.add(meshblendSlider);
	if (material != nullptr && material->shaderType == MaterialComponent::SHADERTYPE_INTERIORMAPPING)
	{
		interiorScaleXSlider.SetVisible(true);
		interiorScaleYSlider.SetVisible(true);
		interiorScaleZSlider.SetVisible(true);
		interiorOffsetXSlider.SetVisible(true);
		interiorOffsetYSlider.SetVisible(true);
		interiorOffsetZSlider.SetVisible(true);
		interiorRotationSlider.SetVisible(true);
		layout.add(interiorScaleXSlider);
		layout.add(interiorScaleYSlider);
		layout.add(interiorScaleZSlider);
		layout.add(interiorOffsetXSlider);
		layout.add(interiorOffsetYSlider);
		layout.add(interiorOffsetZSlider);
		layout.add(interiorRotationSlider);
	}
	else
	{
		interiorScaleXSlider.SetVisible(false);
		interiorScaleYSlider.SetVisible(false);
		interiorScaleZSlider.SetVisible(false);
		interiorOffsetXSlider.SetVisible(false);
		interiorOffsetYSlider.SetVisible(false);
		interiorOffsetZSlider.SetVisible(false);
		interiorRotationSlider.SetVisible(false);
	}
	layout.add(colorComboBox);
	layout.add_fullwidth(colorPicker);
	layout.add(textureSlotComboBox);

	const float button_width = layout.width / 3.0f - layout.padding * 2.0f / 3.0f;

	textureSlotClearButton.SetSize(XMFLOAT2(button_width, textureSlotClearButton.GetSize().y));
	textureSlotClearButton.SetPos(XMFLOAT2(layout.width - layout.padding - textureSlotClearButton.GetSize().x, layout.y));

	textureSlotSelectButton.SetSize(XMFLOAT2(button_width, textureSlotSelectButton.GetSize().y));
	textureSlotSelectButton.SetPos(XMFLOAT2(textureSlotClearButton.GetPos().x - layout.padding - textureSlotSelectButton.GetSize().x, layout.y));

	textureSlotLoadButton.SetSize(XMFLOAT2(button_width, textureSlotLoadButton.GetSize().y));
	textureSlotLoadButton.SetPos(XMFLOAT2(textureSlotSelectButton.GetPos().x - layout.padding - textureSlotLoadButton.GetSize().x, layout.y));

	layout.y += textureSlotLoadButton.GetSize().y + layout.padding;

	layout.add_fullwidth(textureSlotImage);
	layout.add_fullwidth(textureSlotLabel);
	textureSlotLabel.SetSize(XMFLOAT2(textureSlotLabel.GetSize().x - textureSlotLabel.GetSize().y - 2, textureSlotLabel.GetSize().y));
	textureSlotUvsetField.SetPos(XMFLOAT2(textureSlotLabel.GetPos().x + textureSlotLabel.GetSize().x + 2, textureSlotLabel.GetPos().y));

	// Update texture picker
	if (texturePickerWindow.IsVisible())
	{
		const NameComponent* name = scene.names.GetComponent(entity);
		std::string windowTitle = "Texture Picker";
		if (material && name != nullptr && !name->name.empty())
		{
			windowTitle += " (Entity: " + name->name + ")";
		}
		windowTitle += " [Texture slot: " + textureSlotComboBox.GetItemText(textureSlotComboBox.GetSelected()) + "]";
		texturePickerWindow.moveDragger.SetText(windowTitle);

		if (!texturePickerButtons.empty())
		{
			wi::gui::Theme theme;
			theme.image.CopyFrom(texturePickerWindow.sprites[wi::gui::IDLE].params);
			theme.image.background = false;
			theme.image.blendFlag = wi::enums::BLENDMODE_ALPHA;
			theme.image.color = wi::Color::White();
			theme.font.CopyFrom(font.params);
			theme.shadow_color = wi::Color::lerp(theme.font.color, wi::Color::Transparent(), 0.25f);
			theme.tooltipFont.CopyFrom(tooltipFont.params);
			theme.tooltipImage.CopyFrom(tooltipSprite.params);

			constexpr float preview_size = 100;
			constexpr float border = 10;
			const float window_width = texturePickerWindow.GetWidgetAreaSize().x;
			const int cells = std::max(1, int(window_width / (preview_size + border)));
			float offset_y = border;

			for (size_t i = 0; i < texturePickerButtons.size(); ++i)
			{
				wi::gui::Button& button = texturePickerButtons[i];

				button.SetTheme(theme);
				button.SetColor(wi::Color::White(), wi::gui::IDLE);
				button.SetColor(wi::Color(255, 255, 255, 150), wi::gui::FOCUS);
				button.SetShadowRadius(0);

				button.SetSize(XMFLOAT2(preview_size, preview_size));
				button.SetPos(XMFLOAT2((i % cells) * (preview_size + border) + border, offset_y));
				button.SetVisible(IsVisible() && !IsCollapsed());
				button.SetEnabled(true);

				if ((i % cells) == (cells - 1))
				{
					offset_y += preview_size + border;
				}
			}
		}
	}
}
