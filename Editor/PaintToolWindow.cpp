#include "stdafx.h"
#include "PaintToolWindow.h"
#include "shaders/ShaderInterop_Renderer.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void PaintToolWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Paint Tool", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(360, 800));

	float x = 105;
	float y = 0;
	float hei = 20;
	float step = hei + 4;
	float wid = 160;

	colorPicker.Create("Color", wi::gui::Window::WindowControls::NONE);
	float mod_wid = colorPicker.GetScale().x;

	modeComboBox.Create("Mode: ");
	modeComboBox.SetTooltip("Choose paint tool mode");
	modeComboBox.SetPos(XMFLOAT2(x, y));
	modeComboBox.SetSize(XMFLOAT2(wid, hei));
	modeComboBox.AddItem(ICON_DISABLED " Disabled");
	modeComboBox.AddItem(ICON_MATERIAL " Texture");
	modeComboBox.AddItem(ICON_MESH " Vertexcolor");
	modeComboBox.AddItem(ICON_MESH " Sculpting - Add");
	modeComboBox.AddItem(ICON_MESH " Sculpting - Subtract");
	modeComboBox.AddItem(ICON_SOFTBODY " Softbody - Pinning");
	modeComboBox.AddItem(ICON_SOFTBODY " Softbody - Physics");
	modeComboBox.AddItem(ICON_HAIR " Hairparticle - Add Triangle");
	modeComboBox.AddItem(ICON_HAIR " Hairparticle - Remove Triangle");
	modeComboBox.AddItem(ICON_HAIR " Hairparticle - Length (Alpha)");
	modeComboBox.AddItem(ICON_MESH " Wind weight (Alpha)");
	modeComboBox.SetSelected(0);
	modeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		switch (args.iValue)
		{
		case MODE_DISABLED:
			infoLabel.SetText("Paint Tool is disabled.");
			break;
		case MODE_TEXTURE:
			infoLabel.SetText("In texture paint mode, you can paint on textures. Brush will be applied in texture space.\nREMEMBER to save texture when finished to save texture file!\nREMEMBER to save scene to retain new texture bindings on materials!");
			break;
		case MODE_VERTEXCOLOR:
			infoLabel.SetText("In vertex color mode, you can paint colors on selected geometry (per vertex). \"Use vertex colors\" will be automatically enabled for the selected material, or all materials if the whole object is selected. If there is no vertexcolors vertex buffer, one will be created with white as default for every vertex.");
			break;
		case MODE_SCULPTING_ADD:
			infoLabel.SetText("In sculpt - ADD mode, you can modify vertex positions by ADD operation along normal vector (average normal of vertices touched by brush).");
			break;
		case MODE_SCULPTING_SUBTRACT:
			infoLabel.SetText("In sculpt - SUBTRACT mode, you can modify vertex positions by SUBTRACT operation along normal vector (average normal of vertices touched by brush).");
			break;
		case MODE_SOFTBODY_PINNING:
			infoLabel.SetText("In soft body pinning mode, the selected object's soft body vertices can be pinned down (so they will be fixed and drive physics)");
			break;
		case MODE_SOFTBODY_PHYSICS:
			infoLabel.SetText("In soft body physics mode, the selected object's soft body vertices can be unpinned (so they will be simulated by physics)");
			break;
		case MODE_HAIRPARTICLE_ADD_TRIANGLE:
			infoLabel.SetText("In hair particle add triangle mode, you can add triangles to the hair base mesh.\nThis will modify random distribution of hair!");
			break;
		case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
			infoLabel.SetText("In hair particle remove triangle mode, you can remove triangles from the hair base mesh.\nThis will modify random distribution of hair!");
			break;
		case MODE_HAIRPARTICLE_LENGTH:
			infoLabel.SetText("In hair particle length mode, you can adjust length of hair particles with the colorpicker Alpha channel (A). The Alpha channel is 0-255, but the length will be normalized to 0-1 range.\nThis will NOT modify random distribution of hair!");
			break;
		case MODE_WIND:
			infoLabel.SetText("Paint the wind affection amount onto the vertices. Use the Alpha channel to control the amount.");
			break;
		}
	});
	AddWidget(&modeComboBox);

	y += step + 5;

	infoLabel.Create("Paint Tool is disabled.");
	infoLabel.SetSize(XMFLOAT2(mod_wid - 10, 100));
	infoLabel.SetPos(XMFLOAT2(5, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);

	y += infoLabel.GetScale().y - step + 5;

	radiusSlider.Create(1.0f, 500.0f, 50, 10000, "Brush Radius: ");
	radiusSlider.SetTooltip("Set the brush radius in pixel units");
	radiusSlider.SetSize(XMFLOAT2(wid, hei));
	radiusSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&radiusSlider);

	amountSlider.Create(0, 1, 1, 10000, "Power: ");
	amountSlider.SetTooltip("Set the brush power amount. 0 = minimum affection, 1 = maximum affection");
	amountSlider.SetSize(XMFLOAT2(wid, hei));
	amountSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&amountSlider);

	smoothnessSlider.Create(0, 2, 1, 10000, "Smoothness: ");
	smoothnessSlider.SetTooltip("Set the brush smoothness. 0 = hard, increase for more smoothness");
	smoothnessSlider.SetSize(XMFLOAT2(wid, hei));
	smoothnessSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&smoothnessSlider);

	spacingSlider.Create(0, 500, 1, 500, "Spacing: ");
	spacingSlider.SetTooltip("Brush spacing means how much brush movement (in pixels) starts a new stroke. 0 = new stroke every frame, 100 = every 100 pixel movement since last stroke will start a new stroke.");
	spacingSlider.SetSize(XMFLOAT2(wid, hei));
	spacingSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&spacingSlider);

	rotationSlider.Create(0, 1, 0, 10000, "Rotation: ");
	rotationSlider.SetTooltip("Brush rotation randomness. This will affect the splat mode brush texture.");
	rotationSlider.SetSize(XMFLOAT2(wid, hei));
	rotationSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&rotationSlider);

	stabilizerSlider.Create(1, 15, 2, 14, "Stabilizer: ");
	stabilizerSlider.SetTooltip("The stabilizer generates a small delay between user input and painting, which will be used to compute a smoother paint stroke..");
	stabilizerSlider.SetSize(XMFLOAT2(wid, hei));
	stabilizerSlider.SetPos(XMFLOAT2(x, y += step));
	if (editor->main->config.GetSection("paint_tool").Has("stabilizer"))
	{
		stabilizerSlider.SetValue((float)editor->main->config.GetSection("paint_tool").GetInt("stabilizer"));
	}
	stabilizerSlider.OnSlide([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("paint_tool").Set("stabilizer", args.iValue);
		editor->main->config.Commit();
		});
	AddWidget(&stabilizerSlider);

	backfaceCheckBox.Create("Backfaces: ");
	backfaceCheckBox.SetTooltip("Set whether to paint on backfaces of geometry or not");
	backfaceCheckBox.SetSize(XMFLOAT2(hei, hei));
	backfaceCheckBox.SetPos(XMFLOAT2(x - 20, y += step));
	if (editor->main->config.GetSection("paint_tool").Has("backfaces"))
	{
		backfaceCheckBox.SetCheck(editor->main->config.GetSection("paint_tool").GetBool("backfaces"));
	}
	backfaceCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("paint_tool").Set("backfaces", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&backfaceCheckBox);

	wireCheckBox.Create("Wireframe: ");
	wireCheckBox.SetTooltip("Set whether to draw wireframe on top of geometry or not");
	wireCheckBox.SetSize(XMFLOAT2(hei, hei));
	wireCheckBox.SetPos(XMFLOAT2(x - 20 + 100, y));
	wireCheckBox.SetCheck(true);
	if (editor->main->config.GetSection("paint_tool").Has("wireframe"))
	{
		wireCheckBox.SetCheck(editor->main->config.GetSection("paint_tool").GetBool("wireframe"));
	}
	wireCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("paint_tool").Set("wireframe", args.bValue);
		editor->main->config.Commit();
		});
	AddWidget(&wireCheckBox);

	pressureCheckBox.Create("Pressure: ");
	pressureCheckBox.SetTooltip("Set whether to use pressure sensitivity (for example pen tablet)");
	pressureCheckBox.SetSize(XMFLOAT2(hei, hei));
	pressureCheckBox.SetPos(XMFLOAT2(x - 20 + 200, y));
	if (editor->main->config.GetSection("paint_tool").Has("pressure"))
	{
		pressureCheckBox.SetCheck(editor->main->config.GetSection("paint_tool").GetBool("pressure"));
	}
	pressureCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->main->config.GetSection("paint_tool").Set("pressure", args.bValue);
		editor->main->config.Commit();
		});
	pressureCheckBox.SetCheckText(ICON_PEN);
	AddWidget(&pressureCheckBox);

	axisCombo.Create("Axis Lock: ");
	axisCombo.SetTooltip("You can lock modification to an axis here.");
	axisCombo.SetPos(XMFLOAT2(x, y));
	axisCombo.SetSize(XMFLOAT2(wid, hei));
	axisCombo.AddItem(ICON_DISABLED, (uint64_t)AxisLock::Disabled);
	axisCombo.AddItem("X " ICON_LEFT_RIGHT, (uint64_t)AxisLock::X);
	axisCombo.AddItem("Y " ICON_UP_DOWN, (uint64_t)AxisLock::Y);
	axisCombo.AddItem("Z " ICON_UPRIGHT_DOWNLEFT, (uint64_t)AxisLock::Z);
	AddWidget(&axisCombo);

	colorPicker.SetPos(XMFLOAT2(5, y += step));
	AddWidget(&colorPicker);

	y += colorPicker.GetScale().y;

	textureSlotComboBox.Create("Texture Slot: ");
	textureSlotComboBox.SetTooltip("Choose texture slot of the selected material to paint (texture paint mode only)");
	textureSlotComboBox.SetPos(XMFLOAT2(x, y += step));
	textureSlotComboBox.SetSize(XMFLOAT2(wid, hei));
	textureSlotComboBox.AddItem("BaseColor (RGBA)", MaterialComponent::BASECOLORMAP);
	textureSlotComboBox.AddItem("Normal (RGB)", MaterialComponent::NORMALMAP);
	textureSlotComboBox.AddItem("SurfaceMap (RGBA)", MaterialComponent::SURFACEMAP);
	textureSlotComboBox.AddItem("DisplacementMap (R)", MaterialComponent::DISPLACEMENTMAP);
	textureSlotComboBox.AddItem("EmissiveMap (RGBA)", MaterialComponent::EMISSIVEMAP);
	textureSlotComboBox.AddItem("OcclusionMap (R)", MaterialComponent::OCCLUSIONMAP);
	textureSlotComboBox.AddItem("TransmissionMap (R)", MaterialComponent::TRANSMISSIONMAP);
	textureSlotComboBox.AddItem("SheenColorMap (R)", MaterialComponent::SHEENCOLORMAP);
	textureSlotComboBox.AddItem("SheenRoughMap (R)", MaterialComponent::SHEENROUGHNESSMAP);
	textureSlotComboBox.AddItem("ClearcoatMap (R)", MaterialComponent::CLEARCOATMAP);
	textureSlotComboBox.AddItem("ClearcoatRoughMap (R)", MaterialComponent::CLEARCOATROUGHNESSMAP);
	textureSlotComboBox.AddItem("ClearcoatNormMap (R)", MaterialComponent::CLEARCOATNORMALMAP);
	textureSlotComboBox.SetSelected(0);
	AddWidget(&textureSlotComboBox);

	brushShapeComboBox.Create("Brush Shape: ");
	brushShapeComboBox.SetTooltip("Choose shape for brush masking effect");
	brushShapeComboBox.SetPos(XMFLOAT2(x, y += step));
	brushShapeComboBox.SetSize(XMFLOAT2(wid, hei));
	brushShapeComboBox.AddItem(ICON_CIRCLE);
	brushShapeComboBox.AddItem(ICON_SQUARE);
	brushShapeComboBox.SetSelected(0);
	AddWidget(&brushShapeComboBox);

	saveTextureButton.Create("Save Texture");
	saveTextureButton.SetTooltip("Save edited texture.");
	saveTextureButton.SetSize(XMFLOAT2(wid, hei));
	saveTextureButton.SetPos(XMFLOAT2(x, y += step));
	saveTextureButton.OnClick([this] (wi::gui::EventArgs args) {

		Scene& scene = editor->GetCurrentScene();
		for (auto& selected : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				continue;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
				continue;

			MaterialComponent* material = selected.subsetIndex >= 0 && selected.subsetIndex < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[selected.subsetIndex].materialID) : nullptr;
			if (material == nullptr)
				continue;

			TextureSlot editTexture = GetEditTextureSlot(*material);

			uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());

			wi::vector<uint8_t> texturefiledata;
			if (wi::helper::saveTextureToMemoryFile(editTexture.texture, "PNG", texturefiledata))
			{
				material->textures[sel].resource.SetFileData(texturefiledata);
			}
			else
			{
				wi::helper::messageBox("Saving texture failed! :(");
			}
		}

	});
	AddWidget(&saveTextureButton);

	brushTextureButton.Create("");
	brushTextureButton.SetDescription("Brush tex: ");
	brushTextureButton.SetTooltip("Open an image to use as brush texture (splatting mode).\nSplat mode means that the texture will be relative to the brush position");
	brushTextureButton.SetSize(XMFLOAT2(hei*2, hei*2));
	brushTextureButton.SetPos(XMFLOAT2(x, y += step));
	brushTextureButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	brushTextureButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	brushTextureButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	brushTextureButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	brushTextureButton.OnClick([this](wi::gui::EventArgs args) {
		if (brushTex.IsValid())
		{
			brushTex = {};
			brushTextureButton.SetImage({});
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					brushTex = wi::resourcemanager::Load(fileName);
					brushTextureButton.SetImage(brushTex);
					});
				});
		}
		});
	AddWidget(&brushTextureButton);

	revealTextureButton.Create("");
	revealTextureButton.SetDescription("Reveal tex: ");
	revealTextureButton.SetTooltip("Open an image to use as reveal mode texture.\nReveal mode means that the texture will use the UV of the mesh. It will be multiplied by brush tex.");
	revealTextureButton.SetSize(XMFLOAT2(hei * 2, hei * 2));
	revealTextureButton.SetPos(XMFLOAT2(x + 150, y));
	revealTextureButton.sprites[wi::gui::IDLE].params.color = wi::Color::White();
	revealTextureButton.sprites[wi::gui::FOCUS].params.color = wi::Color::Gray();
	revealTextureButton.sprites[wi::gui::ACTIVE].params.color = wi::Color::White();
	revealTextureButton.sprites[wi::gui::DEACTIVATING].params.color = wi::Color::Gray();
	revealTextureButton.OnClick([this](wi::gui::EventArgs args) {
		if (revealTex.IsValid())
		{
			revealTex = {};
			revealTextureButton.SetImage({});
		}
		else
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [this](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					revealTex = wi::resourcemanager::Load(fileName);
					revealTextureButton.SetImage(revealTex);
					});
				});
		}
		});
	AddWidget(&revealTextureButton);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 550, 50, 0));

	SetMinimized(true);
}

void PaintToolWindow::Update(float dt)
{
	RecordHistory(false);

	if (GetMode() == MODE_TEXTURE)
	{
		rotationSlider.SetVisible(true);
		textureSlotComboBox.SetVisible(true);
		brushShapeComboBox.SetVisible(true);
		saveTextureButton.SetVisible(true);
		brushTextureButton.SetVisible(true);
		revealTextureButton.SetVisible(true);
	}
	else
	{
		rotationSlider.SetVisible(false);
		textureSlotComboBox.SetVisible(false);
		brushShapeComboBox.SetVisible(false);
		saveTextureButton.SetVisible(false);
		brushTextureButton.SetVisible(false);
		revealTextureButton.SetVisible(false);
	}

	if (GetMode() == MODE_SCULPTING_ADD || GetMode() == MODE_SCULPTING_SUBTRACT)
	{
		axisCombo.SetVisible(true);
	}
	else
	{
		axisCombo.SetVisible(false);
	}

	rot -= dt;
	// by default, paint tool is on center of screen, this makes it easy to tweak radius with GUI:
	XMFLOAT2 posNew;
	posNew.x = editor->GetLogicalWidth() * 0.5f;
	posNew.y = editor->GetLogicalHeight() * 0.5f;
	if (editor->GetGUI().HasFocus() || wi::backlog::isActive())
	{
		pos = posNew;
		strokes.clear();
		return;
	}

	const bool pointer_down = wi::input::Down(wi::input::MOUSE_BUTTON_LEFT);
	if (!pointer_down)
	{
		stroke_dist = FLT_MAX;
		sculpting_normal = XMFLOAT3(0, 0, 0);
		if (!strokes.empty())
		{
			strokes.pop_front();
		}
	}

	auto pointer = wi::input::GetPointer();
	posNew = XMFLOAT2(pointer.x, pointer.y);
	float pressureNew = pressureCheckBox.GetCheck() ? pointer.w : 1.0f;

	const MODE mode = GetMode();
	const float radius = radiusSlider.GetValue();
	const float amount = amountSlider.GetValue();
	const float smoothness = smoothnessSlider.GetValue();
	const wi::Color color = colorPicker.GetPickColor();
	const XMFLOAT4 color_float = color.toFloat4();
	const bool backfaces = backfaceCheckBox.GetCheck();
	const bool wireframe = wireCheckBox.GetCheck();
	const float spacing = spacingSlider.GetValue();
	const size_t stabilizer = (size_t)stabilizerSlider.GetValue();
	CommandList cmd;

	if (pointer_down)
	{
		Stroke stroke;
		stroke.position = posNew;
		stroke.pressure = pressureNew;
		strokes.push_back(stroke);
	}

	if (strokes.empty())
	{
		pos = posNew;
	}
	else
	{
		pos = strokes.front().position;
	}

	const XMVECTOR spline_p0 = strokes.empty() ? XMVectorSet(posNew.x, posNew.y, pressureNew, 0) : XMVectorSet(strokes[0].position.x, strokes[0].position.y, strokes[0].pressure, 0);
	const XMVECTOR spline_p1 = strokes.size() < 2 ? spline_p0 : XMVectorSet(strokes[1].position.x, strokes[1].position.y, strokes[1].pressure, 0);
	const XMVECTOR spline_p2 = strokes.size() < 3 ? spline_p1 : XMVectorSet(strokes[2].position.x, strokes[2].position.y, strokes[2].pressure, 0);
	const XMVECTOR spline_p3 = strokes.size() < 4 ? spline_p2 : XMVectorSet(strokes[3].position.x, strokes[3].position.y, strokes[3].pressure, 0);

	int substep_count = (int)std::ceil(wi::math::Distance(pos, posNew) / (radius * pressureNew));
	substep_count = std::max(1, std::min(100, substep_count));

	for (int substep = 0; substep < substep_count; ++substep)
	{
		const float t = float(substep) / float(substep_count);

		float pressure = 1;
		const XMVECTOR spline_p = XMVectorCatmullRom(
			spline_p0,
			spline_p1,
			spline_p2,
			spline_p3,
			t
		);
		XMFLOAT2 pos_eval;
		XMStoreFloat2(&pos_eval, spline_p);
		stroke_dist += wi::math::Distance(pos, pos_eval);
		pos = pos_eval;
		pressure = XMVectorGetZ(spline_p);
		const float pressure_radius = radius * pressure;
		bool pointer_moved = false;
		if (stroke_dist >= spacing)
		{
			pointer_moved = true;
			stroke_dist = 0;
		}
		const bool painting = pointer_moved && strokes.size() >= stabilizer;

		Scene& scene = editor->GetCurrentScene();
		const CameraComponent& camera = editor->GetCurrentEditorScene().camera;
		const XMVECTOR C = XMLoadFloat2(&pos);
		const XMMATRIX VP = camera.GetViewProjection();
		const XMVECTOR MUL = XMVectorSet(0.5f, -0.5f, 1, 1);
		const XMVECTOR ADD = XMVectorSet(0.5f, 0.5f, 0, 0);
		const XMVECTOR SCREEN = XMVectorSet((float)editor->GetLogicalWidth(), (float)editor->GetLogicalHeight(), 1, 1);
		const XMVECTOR F = camera.GetAt();
		const float brush_rotation = wi::random::GetRandom(0.0f, rotationSlider.GetValue() * XM_2PI);

		for (auto& selected : editor->translator.selected)
		{
			switch (mode)
			{
			case MODE_TEXTURE:
			{
				wi::primitive::Ray pickRay = wi::renderer::GetPickRay((long)pos.x, (long)pos.y, *editor, camera);
				const wi::scene::PickResult intersect = wi::scene::Pick(pickRay, ~0u, ~0u, scene);
				if (intersect.entity != selected.entity)
					break;

				ObjectComponent* object = scene.objects.GetComponent(selected.entity);
				if (object == nullptr || object->meshID == INVALID_ENTITY)
					break;

				MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
					break;

				MaterialComponent* material = selected.subsetIndex >= 0 && selected.subsetIndex < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[selected.subsetIndex].materialID) : nullptr;
				if (material == nullptr)
					break;

				int uvset = 0;
				TextureSlot editTexture = GetEditTextureSlot(*material, &uvset);
				if (!editTexture.texture.IsValid())
					break;
				const TextureDesc& desc = editTexture.texture.GetDesc();
				auto& vertex_uvset = uvset == 0 ? mesh->vertex_uvset_0 : mesh->vertex_uvset_1;

				const float u = intersect.bary.x;
				const float v = intersect.bary.y;
				const float w = 1 - u - v;
				XMFLOAT2 uv;
				uv.x = vertex_uvset[intersect.vertexID0].x * w +
					vertex_uvset[intersect.vertexID1].x * u +
					vertex_uvset[intersect.vertexID2].x * v;
				uv.y = vertex_uvset[intersect.vertexID0].y * w +
					vertex_uvset[intersect.vertexID1].y * u +
					vertex_uvset[intersect.vertexID2].y * v;
				uv.x = uv.x * material->texMulAdd.x + material->texMulAdd.z;
				uv.y = uv.y * material->texMulAdd.y + material->texMulAdd.w;
				uint2 center = XMUINT2(uint32_t(uv.x * desc.width), uint32_t(uv.y * desc.height));

				if (painting)
				{
					GraphicsDevice* device = wi::graphics::GetDevice();
					if (!cmd.IsValid())
					{
						cmd = device->BeginCommandList();
					}

					RecordHistory(true, cmd);

					// Need to requery this because RecordHistory might swap textures on material:
					editTexture = GetEditTextureSlot(*material, &uvset);

					device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_PAINT_TEXTURE), cmd);

					if (brushTex.IsValid())
					{
						device->BindResource(&brushTex.GetTexture(), 0, cmd);
					}
					else
					{
						device->BindResource(wi::texturehelper::getWhite(), 0, cmd);
					}
					if (revealTex.IsValid())
					{
						device->BindResource(&revealTex.GetTexture(), 1, cmd);
					}
					else
					{
						device->BindResource(wi::texturehelper::getWhite(), 1, cmd);
					}
					device->BindUAV(&editTexture.texture, 0, cmd);

					PaintTextureCB cb;
					cb.xPaintBrushCenter = center;
					cb.xPaintBrushRadius = (uint32_t)pressure_radius;
					if (brushShapeComboBox.GetSelected() == 1)
					{
						cb.xPaintBrushRadius = (uint)std::ceil((float(cb.xPaintBrushRadius) * 2 / std::sqrt(2.0f))); // square shape, diagonal dispatch size
					}
					cb.xPaintBrushAmount = amount;
					cb.xPaintBrushSmoothness = smoothness;
					cb.xPaintBrushColor = color.rgba;
					cb.xPaintReveal = revealTex.IsValid() ? 1 : 0;
					cb.xPaintBrushRotation = brush_rotation;
					cb.xPaintBrushShape = (uint)brushShapeComboBox.GetSelected();
					device->PushConstants(&cb, sizeof(cb), cmd);

					const uint diameter = cb.xPaintBrushRadius * 2;
					const uint dispatch_dim = (diameter + PAINT_TEXTURE_BLOCKSIZE - 1) / PAINT_TEXTURE_BLOCKSIZE;
					device->Dispatch(dispatch_dim, dispatch_dim, 1, cmd);

					GPUBarrier barriers[] = {
						GPUBarrier::Memory(),
					};
					device->Barrier(barriers, arraysize(barriers), cmd);

					if (editTexture.texture.desc.mip_levels > 1)
					{
						wi::renderer::GenerateMipChain(editTexture.texture, wi::renderer::MIPGENFILTER::MIPGENFILTER_LINEAR, cmd);
					}
				}
				if(substep == substep_count - 1)
				{
					wi::renderer::PaintRadius paintrad;
					paintrad.objectEntity = selected.entity;
					paintrad.subset = selected.subsetIndex;
					paintrad.radius = radius;
					paintrad.center = center;
					paintrad.uvset = uvset;
					paintrad.dimensions.x = desc.width;
					paintrad.dimensions.y = desc.height;
					paintrad.rotation = brush_rotation;
					paintrad.shape = (uint)brushShapeComboBox.GetSelected();
					wi::renderer::DrawPaintRadius(paintrad);
				}
			}
			break;

			case MODE_VERTEXCOLOR:
			case MODE_WIND:
			{
				ObjectComponent* object = scene.objects.GetComponent(selected.entity);
				if (object == nullptr || object->meshID == INVALID_ENTITY)
					break;

				MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh == nullptr)
					break;

				MaterialComponent* material = selected.subsetIndex >= 0 && selected.subsetIndex < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[selected.subsetIndex].materialID) : nullptr;
				if (material == nullptr)
				{
					for (auto& x : mesh->subsets)
					{
						material = scene.materials.GetComponent(x.materialID);
						if (material != nullptr)
						{
							switch (mode)
							{
							case MODE_VERTEXCOLOR:
								material->SetUseVertexColors(true);
								break;
							case MODE_WIND:
								material->SetUseWind(true);
								break;
							}
						}
					}
					material = nullptr;
				}
				else
				{
					switch (mode)
					{
					case MODE_VERTEXCOLOR:
						material->SetUseVertexColors(true);
						break;
					case MODE_WIND:
						material->SetUseWind(true);
						break;
					}
				}

				const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

				const TransformComponent* transform = scene.transforms.GetComponent(selected.entity);
				if (transform == nullptr)
					break;

				const XMMATRIX W = XMLoadFloat4x4(&transform->world);

				bool rebuild = false;

				switch (mode)
				{
				case MODE_VERTEXCOLOR:
					if (mesh->vertex_colors.empty())
					{
						mesh->vertex_colors.resize(mesh->vertex_positions.size());
						std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), wi::Color::White().rgba); // fill white
						rebuild = true;
					}
					break;
				case MODE_WIND:
					if (mesh->vertex_windweights.empty())
					{
						mesh->vertex_windweights.resize(mesh->vertex_positions.size());
						std::fill(mesh->vertex_windweights.begin(), mesh->vertex_windweights.end(), 0xFF); // fill max affection
						rebuild = true;
					}
					break;
				}

				if (painting)
				{
					for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
					{
						XMVECTOR P, N;
						if (armature == nullptr)
						{
							P = XMLoadFloat3(&mesh->vertex_positions[j]);
							N = XMLoadFloat3(&mesh->vertex_normals[j]);
						}
						else
						{
							P = wi::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
						}
						P = XMVector3Transform(P, W);
						N = XMVector3Normalize(XMVector3TransformNormal(N, W));

						if (!backfaces && XMVectorGetX(XMVector3Dot(F, N)) > 0)
							continue;

						P = XMVector3TransformCoord(P, VP);
						P = P * MUL + ADD;
						P = P * SCREEN;

						const float z = XMVectorGetZ(P);
						const float dist = XMVectorGetX(XMVector2Length(C - P));
						if (z >= 0 && z <= 1 && dist <= pressure_radius)
						{
							RecordHistory(true);
							rebuild = true;
							const float affection = amount * wi::math::SmoothStep(0, smoothness, 1 - dist / pressure_radius);

							switch (mode)
							{
							case MODE_VERTEXCOLOR:
							{
								wi::Color vcol = mesh->vertex_colors[j];
								vcol = wi::Color::lerp(vcol, color, affection);
								mesh->vertex_colors[j] = vcol.rgba;
							}
							break;
							case MODE_WIND:
							{
								wi::Color vcol = wi::Color(0, 0, 0, mesh->vertex_windweights[j]);
								vcol = wi::Color::lerp(vcol, color, affection);
								mesh->vertex_windweights[j] = vcol.getA();
							}
							break;
							}
						}
					}
				}

				if (wireframe)
				{
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(0, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						for (size_t j = 0; j < subset.indexCount; j += 3)
						{
							const uint32_t triangle[] = {
								mesh->indices[j + 0],
								mesh->indices[j + 1],
								mesh->indices[j + 2],
							};

							const XMVECTOR P[arraysize(triangle)] = {
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
							};

							wi::renderer::RenderableTriangle tri;
							XMStoreFloat3(&tri.positionA, P[0]);
							XMStoreFloat3(&tri.positionB, P[1]);
							XMStoreFloat3(&tri.positionC, P[2]);
							if (mode == MODE_WIND)
							{
								tri.colorA = wi::Color(mesh->vertex_windweights[triangle[0]], 0, 0, 255);
								tri.colorB = wi::Color(mesh->vertex_windweights[triangle[1]], 0, 0, 255);
								tri.colorC = wi::Color(mesh->vertex_windweights[triangle[2]], 0, 0, 255);
							}
							else
							{
								tri.colorA.w = 0.8f;
								tri.colorB.w = 0.8f;
								tri.colorC.w = 0.8f;
							}
							wi::renderer::DrawTriangle(tri, true);
						}
					}
				}

				if (rebuild)
				{
					mesh->CreateRenderData();
				}
			}
			break;

			case MODE_SCULPTING_ADD:
			case MODE_SCULPTING_SUBTRACT:
			{
				ObjectComponent* object = scene.objects.GetComponent(selected.entity);
				if (object == nullptr || object->meshID == INVALID_ENTITY)
					break;

				MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh == nullptr)
					break;

				const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

				const TransformComponent* transform = scene.transforms.GetComponent(selected.entity);
				if (transform == nullptr)
					break;

				const XMMATRIX W = XMLoadFloat4x4(&transform->world);

				XMVECTOR sculptDir;
				AxisLock axis_lock = (AxisLock)axisCombo.GetItemUserData(axisCombo.GetSelected());
				switch (axis_lock)
				{
				default:
				case PaintToolWindow::AxisLock::Disabled:
					if (sculpting_normal.x < FLT_EPSILON && sculpting_normal.y < FLT_EPSILON && sculpting_normal.z < FLT_EPSILON)
					{
						sculpting_normal = editor->hovered.normal;
					}
					sculptDir = XMVector3TransformNormal(XMVector3Normalize(XMLoadFloat3(&sculpting_normal)), XMMatrixInverse(nullptr, W));
					break;
				case PaintToolWindow::AxisLock::X:
					sculpting_normal = XMFLOAT3(1, 0, 0);
					sculptDir = XMLoadFloat3(&sculpting_normal);
					break;
				case PaintToolWindow::AxisLock::Y:
					sculpting_normal = XMFLOAT3(0, 1, 0);
					sculptDir = XMLoadFloat3(&sculpting_normal);
					break;
				case PaintToolWindow::AxisLock::Z:
					sculpting_normal = XMFLOAT3(0, 0, 1);
					sculptDir = XMLoadFloat3(&sculpting_normal);
					break;
				}


				bool rebuild = false;
				if (painting)
				{
					sculpting_indices.clear();
					sculpting_indices.reserve(mesh->vertex_positions.size());

					for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
					{
						XMVECTOR P, N;
						if (armature == nullptr)
						{
							P = XMLoadFloat3(&mesh->vertex_positions[j]);
							N = XMLoadFloat3(&mesh->vertex_normals[j]);
						}
						else
						{
							P = wi::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
						}
						P = XMVector3Transform(P, W);
						N = XMVector3Normalize(XMVector3TransformNormal(N, W));

						const float dotN = XMVectorGetX(XMVector3Dot(F, N));
						if (!backfaces && dotN > 0)
							continue;

						P = XMVector3TransformCoord(P, VP);
						P = P * MUL + ADD;
						P = P * SCREEN;

						const float z = XMVectorGetZ(P);
						const float dist = XMVectorGetX(XMVector2Length(C - P));
						if (z >= 0 && z <= 1 && dist <= pressure_radius)
						{
							const float affection = amount * wi::math::SmoothStep(0, smoothness, 1 - dist / pressure_radius);
							sculpting_indices.push_back({ j, affection });
						}
					}

					if (!sculpting_indices.empty())
					{
						RecordHistory(true);
						rebuild = true;

						for (auto& x : sculpting_indices)
						{
							XMVECTOR PL = XMLoadFloat3(&mesh->vertex_positions[x.ind]);
							switch (mode)
							{
							case MODE_SCULPTING_ADD:
								PL += sculptDir * x.affection;
								break;
							case MODE_SCULPTING_SUBTRACT:
								PL -= sculptDir * x.affection;
								break;
							}
							XMStoreFloat3(&mesh->vertex_positions[x.ind], PL);
						}
					}
				}

				if (wireframe)
				{
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(0, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						for (size_t j = 0; j < subset.indexCount; j += 3)
						{
							const uint32_t triangle[] = {
								mesh->indices[j + 0],
								mesh->indices[j + 1],
								mesh->indices[j + 2],
							};
							const XMVECTOR P[arraysize(triangle)] = {
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
							};

							wi::renderer::RenderableTriangle tri;
							XMStoreFloat3(&tri.positionA, P[0]);
							XMStoreFloat3(&tri.positionB, P[1]);
							XMStoreFloat3(&tri.positionC, P[2]);
							tri.colorA.w = 0.8f;
							tri.colorB.w = 0.8f;
							tri.colorC.w = 0.8f;
							wi::renderer::DrawTriangle(tri, true);
						}

						wi::renderer::RenderableLine sculpt_dir_line;
						sculpt_dir_line.color_start = XMFLOAT4(0, 1, 0, 1);
						sculpt_dir_line.color_end = XMFLOAT4(0, 1, 0, 1);
						sculpt_dir_line.start = editor->hovered.position;
						XMStoreFloat3(
							&sculpt_dir_line.end,
							XMLoadFloat3(&sculpt_dir_line.start) +
							XMVector3Normalize(XMLoadFloat3(&sculpting_normal))
						);
						wi::renderer::DrawLine(sculpt_dir_line);
					}
				}

				if (rebuild)
				{
					mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);
				}
			}
			break;

			case MODE_SOFTBODY_PINNING:
			case MODE_SOFTBODY_PHYSICS:
			{
				ObjectComponent* object = scene.objects.GetComponent(selected.entity);
				if (object == nullptr || object->meshID == INVALID_ENTITY)
					break;

				const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
				if (mesh == nullptr)
					break;

				SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
				if (softbody == nullptr || !softbody->HasVertices())
					break;

				// Painting:
				if (painting)
				{
					size_t j = 0;
					for (auto& ind : softbody->physicsToGraphicsVertexMapping)
					{
						XMVECTOR P = softbody->vertex_positions_simulation[ind].LoadPOS();
						P = XMVector3TransformCoord(P, VP);
						P = P * MUL + ADD;
						P = P * SCREEN;
						const float z = XMVectorGetZ(P);
						if (z >= 0 && z <= 1 && XMVectorGetX(XMVector2Length(C - P)) <= pressure_radius)
						{
							RecordHistory(true);
							softbody->weights[j] = (mode == MODE_SOFTBODY_PINNING ? 0.0f : 1.0f);
							softbody->_flags |= SoftBodyPhysicsComponent::FORCE_RESET;
						}
						j++;
					}
				}

				// Visualizing:
				const XMMATRIX W = XMLoadFloat4x4(&softbody->worldMatrix);
				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh->GetLODSubsetRange(0, first_subset, last_subset);
				for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					for (size_t j = 0; j < subset.indexCount; j += 3)
					{
						const uint32_t graphicsIndex0 = mesh->indices[j + 0];
						const uint32_t graphicsIndex1 = mesh->indices[j + 1];
						const uint32_t graphicsIndex2 = mesh->indices[j + 2];
						const uint32_t physicsIndex0 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex0];
						const uint32_t physicsIndex1 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex1];
						const uint32_t physicsIndex2 = softbody->graphicsToPhysicsVertexMapping[graphicsIndex2];
						const float weight0 = softbody->weights[physicsIndex0];
						const float weight1 = softbody->weights[physicsIndex1];
						const float weight2 = softbody->weights[physicsIndex2];
						wi::renderer::RenderableTriangle tri;
						if (softbody->HasVertices())
						{
							tri.positionA = softbody->vertex_positions_simulation[graphicsIndex0].GetPOS();
							tri.positionB = softbody->vertex_positions_simulation[graphicsIndex1].GetPOS();
							tri.positionC = softbody->vertex_positions_simulation[graphicsIndex2].GetPOS();
						}
						else
						{
							XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex0]), W));
							XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex1]), W));
							XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex2]), W));
						}
						if (weight0 == 0)
							tri.colorA = XMFLOAT4(1, 1, 0, 1);
						else
							tri.colorA = XMFLOAT4(1, 1, 1, 1);
						if (weight1 == 0)
							tri.colorB = XMFLOAT4(1, 1, 0, 1);
						else
							tri.colorB = XMFLOAT4(1, 1, 1, 1);
						if (weight2 == 0)
							tri.colorC = XMFLOAT4(1, 1, 0, 1);
						else
							tri.colorC = XMFLOAT4(1, 1, 1, 1);
						if (wireframe)
						{
							wi::renderer::DrawTriangle(tri, true);
						}
						if (weight0 == 0 && weight1 == 0 && weight2 == 0)
						{
							tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 0, 0.8f);
							wi::renderer::DrawTriangle(tri);
						}
					}
				}
			}
			break;

			case MODE_HAIRPARTICLE_ADD_TRIANGLE:
			case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
			case MODE_HAIRPARTICLE_LENGTH:
			{
				wi::HairParticleSystem* hair = scene.hairs.GetComponent(selected.entity);
				if (hair == nullptr || hair->meshID == INVALID_ENTITY)
					break;

				MeshComponent* mesh = scene.meshes.GetComponent(hair->meshID);
				if (mesh == nullptr)
					break;

				const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

				const TransformComponent* transform = scene.transforms.GetComponent(selected.entity);
				if (transform == nullptr)
					break;

				const XMMATRIX W = XMLoadFloat4x4(&transform->world);

				if (painting)
				{
					for (size_t j = 0; j < mesh->vertex_positions.size(); ++j)
					{
						XMVECTOR P, N;
						if (armature == nullptr)
						{
							P = XMLoadFloat3(&mesh->vertex_positions[j]);
							N = XMLoadFloat3(&mesh->vertex_normals[j]);
						}
						else
						{
							P = wi::scene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
						}
						P = XMVector3Transform(P, W);
						N = XMVector3Normalize(XMVector3TransformNormal(N, W));

						if (!backfaces && XMVectorGetX(XMVector3Dot(F, N)) > 0)
							continue;

						P = XMVector3TransformCoord(P, VP);
						P = P * MUL + ADD;
						P = P * SCREEN;

						const float z = XMVectorGetZ(P);
						const float dist = XMVectorGetX(XMVector2Length(C - P));
						if (z >= 0 && z <= 1 && dist <= pressure_radius)
						{
							RecordHistory(true);
							switch (mode)
							{
							case MODE_HAIRPARTICLE_ADD_TRIANGLE:
								hair->vertex_lengths[j] = 1.0f;
								break;
							case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
								hair->vertex_lengths[j] = 0;
								break;
							case MODE_HAIRPARTICLE_LENGTH:
								if (hair->vertex_lengths[j] > 0) // don't change distribution
								{
									const float affection = amount * wi::math::SmoothStep(0, smoothness, 1 - dist / pressure_radius);
									hair->vertex_lengths[j] = wi::math::Lerp(hair->vertex_lengths[j], color_float.w, affection);
									// don't let it "remove" the vertex by keeping its length above zero:
									//	(because if removed, distribution also changes which might be distracting)
									hair->vertex_lengths[j] = wi::math::Clamp(hair->vertex_lengths[j], 1.0f / 255.0f, 1.0f);
								}
								break;
							}
							hair->_flags |= wi::HairParticleSystem::REBUILD_BUFFERS;
						}
					}
				}

				if (wireframe)
				{
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(0, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						for (size_t j = 0; j < subset.indexCount; j += 3)
						{
							const uint32_t triangle[] = {
								mesh->indices[j + 0],
								mesh->indices[j + 1],
								mesh->indices[j + 2],
							};
							const XMVECTOR P[arraysize(triangle)] = {
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
								XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
							};

							wi::renderer::RenderableTriangle tri;
							XMStoreFloat3(&tri.positionA, P[0]);
							XMStoreFloat3(&tri.positionB, P[1]);
							XMStoreFloat3(&tri.positionC, P[2]);
							wi::renderer::DrawTriangle(tri, true);
						}
					}

					for (size_t j = 0; j < hair->indices.size() && wireframe; j += 3)
					{
						const uint32_t triangle[] = {
							hair->indices[j + 0],
							hair->indices[j + 1],
							hair->indices[j + 2],
						};
						const XMVECTOR P[arraysize(triangle)] = {
							XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[0]), W),
							XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[1]), W),
							XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wi::scene::SkinVertex(*mesh, *armature, triangle[2]), W),
						};

						wi::renderer::RenderableTriangle tri;
						XMStoreFloat3(&tri.positionA, P[0]);
						XMStoreFloat3(&tri.positionB, P[1]);
						XMStoreFloat3(&tri.positionC, P[2]);
						tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 1, 0.9f);
						wi::renderer::DrawTriangle(tri, false);
					}
				}
			}
			break;

			}
		}
	}

	while (strokes.size() > stabilizer)
	{
		strokes.pop_front();
	}
}


namespace PaintTool_Internal
{
	PipelineState pso;

	void LoadShaders()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		{
			PipelineStateDesc desc;

			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::TRIANGLELIST;

			device->CreatePipelineState(&desc, &pso);
		}
	}

	struct Vertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};
}
using namespace PaintTool_Internal;

void PaintToolWindow::DrawBrush(const wi::Canvas& canvas, CommandList cmd) const
{
	const MODE mode = GetMode();
	if (mode == MODE_DISABLED || mode == MODE_TEXTURE || wi::backlog::isActive())
		return;

	static bool shaders_loaded = false;
	if (!shaders_loaded)
	{
		shaders_loaded = true;
		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { LoadShaders(); });
		LoadShaders();
	}

	GraphicsDevice* device = wi::graphics::GetDevice();

	device->EventBegin("Paint Tool", cmd);
	device->BindPipelineState(&pso, cmd);

	const uint32_t segmentCount = 36;
	const uint32_t circle_triangleCount = segmentCount * 2;
	uint32_t vertexCount = circle_triangleCount * 3;
	GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);
	uint8_t* dst = (uint8_t*)mem.data;
	for (uint32_t i = 0; i < segmentCount; ++i)
	{
		const float angle0 = (float)i / (float)segmentCount * XM_2PI;
		const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;

		// circle:
		const float radius = radiusSlider.GetValue();
		const float radius_outer = radius + 8;
		float brightness = i % 2 == 0 ? 1.0f : 0.0f;
		XMFLOAT4 color_inner = XMFLOAT4(brightness, brightness, brightness, 1);
		XMFLOAT4 color_outer = XMFLOAT4(brightness, brightness, brightness, 0);
		const Vertex verts[] = {
			{XMFLOAT4(std::sin(angle0) * radius, std::cos(angle0) * radius, 0, 1), color_inner},
			{XMFLOAT4(std::sin(angle1) * radius, std::cos(angle1) * radius, 0, 1), color_inner},
			{XMFLOAT4(std::sin(angle0) * radius_outer, std::cos(angle0) * radius_outer, 0, 1), color_outer},
			{XMFLOAT4(std::sin(angle0) * radius_outer, std::cos(angle0) * radius_outer, 0, 1), color_outer},
			{XMFLOAT4(std::sin(angle1) * radius_outer, std::cos(angle1) * radius_outer, 0, 1), color_outer},
			{XMFLOAT4(std::sin(angle1) * radius, std::cos(angle1) * radius, 0, 1), color_inner},
		};
		std::memcpy(dst, verts, sizeof(verts));
		dst += sizeof(verts);
	}

	const GPUBuffer* vbs[] = {
		&mem.buffer,
	};
	const uint32_t strides[] = {
		sizeof(Vertex),
	};
	const uint64_t offsets[] = {
		mem.offset,
	};
	device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

	MiscCB sb;
	XMStoreFloat4x4(&sb.g_xTransform, XMMatrixRotationZ(rot) * XMMatrixTranslation(pos.x, pos.y, 0) * canvas.GetProjection());
	sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
	device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
	device->Draw(vertexCount, 0, cmd);

	device->EventEnd(cmd);

}

PaintToolWindow::MODE PaintToolWindow::GetMode() const
{
	return (MODE)modeComboBox.GetSelected();
}

void PaintToolWindow::RecordHistory(bool start, CommandList cmd)
{
	if (start)
	{
		if (history_needs_recording_start)
			return;
		history_needs_recording_start = true;
		history_needs_recording_end = true;

		// Start history recording (undo)
		currentHistory = &editor->AdvanceHistory();
		wi::Archive& archive = *currentHistory;
		archive << EditorComponent::HISTORYOP_PAINTTOOL;
		archive << (uint32_t)GetMode();
		history_redo_jump_position = archive.WriteUnknownJumpPosition();
	}
	else
	{
		if (!history_needs_recording_end || !strokes.empty())
			return;
		history_needs_recording_end = false;
		history_needs_recording_start = false;

		// End history recording (redo)
	}

	wi::Archive& archive = *currentHistory;
	Scene& scene = editor->GetCurrentScene();

	for (auto& selected : editor->translator.selected)
	{
		switch (GetMode())
		{
		case PaintToolWindow::MODE_TEXTURE:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
				break;

			MaterialComponent* material = selected.subsetIndex >= 0 && selected.subsetIndex < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[selected.subsetIndex].materialID) : nullptr;
			if (material == nullptr)
				break;

			auto editTexture = GetEditTextureSlot(*material);

			archive << textureSlotComboBox.GetSelected();

			if (history_textureIndex >= history_textures.size())
			{
				history_textures.resize((history_textureIndex + 1) * 2);
			}
			history_textures[history_textureIndex] = editTexture;
			archive << history_textureIndex;
			history_textureIndex++;

			if (start)
			{
				// Make a copy of texture to edit and replace material resource:
				GraphicsDevice* device = wi::graphics::GetDevice();
				TextureSlot newslot;
				TextureDesc desc = editTexture.texture.GetDesc();
				desc.format = Format::R8G8B8A8_UNORM; // force format to one that is writable by GPU
				desc.bind_flags |= BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
				desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;
				device->CreateTexture(&desc, nullptr, &newslot.texture);
				for (uint32_t i = 0; i < newslot.texture.GetDesc().mip_levels; ++i)
				{
					int subresource_index;
					subresource_index = device->CreateSubresource(&newslot.texture, SubresourceType::SRV, 0, 1, i, 1);
					assert(subresource_index == i);
					subresource_index = device->CreateSubresource(&newslot.texture, SubresourceType::UAV, 0, 1, i, 1);
					assert(subresource_index == i);
				}
				// This part must be AFTER mip level subresource creation:
				int srgb_subresource = -1;
				{
					Format srgb_format = GetFormatSRGB(desc.format);
					newslot.srgb_subresource = device->CreateSubresource(
						&newslot.texture,
						SubresourceType::SRV,
						0, -1,
						0, -1,
						&srgb_format
					);
				}
				assert(cmd.IsValid());
				wi::renderer::CopyTexture2D(
					newslot.texture, 0, 0, 0,
					editTexture.texture, 0, 0, 0,
					cmd
				); // custom copy with format conversion and decompression capability!

				ReplaceEditTextureSlot(*material, newslot);
			}

		}
		break;
		case PaintToolWindow::MODE_VERTEXCOLOR:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			archive << mesh->vertex_colors;
		}
		break;
		case PaintToolWindow::MODE_WIND:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			archive << mesh->vertex_windweights;
		}
		break;
		case PaintToolWindow::MODE_SCULPTING_ADD:
		case PaintToolWindow::MODE_SCULPTING_SUBTRACT:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			archive << mesh->vertex_positions;
			archive << mesh->vertex_normals;
		}
		break;
		case PaintToolWindow::MODE_SOFTBODY_PINNING:
		case PaintToolWindow::MODE_SOFTBODY_PHYSICS:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
			if (softbody == nullptr)
				break;

			archive << softbody->weights;
		}
		break;
		case PaintToolWindow::MODE_HAIRPARTICLE_ADD_TRIANGLE:
		case PaintToolWindow::MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
		case PaintToolWindow::MODE_HAIRPARTICLE_LENGTH:
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(selected.entity);
			if (hair == nullptr)
				break;

			archive << hair->vertex_lengths;
		}
		break;
		default:
			assert(0);
			break;
		}
	}

	if (start)
	{
		archive.PatchUnknownJumpPosition(history_redo_jump_position);
	}
}
void PaintToolWindow::ConsumeHistoryOperation(wi::Archive& archive, bool undo)
{
	MODE historymode;
	archive >> (uint32_t&)historymode;

	uint64_t jump_pos = 0;
	archive >> jump_pos;
	if (!undo)
	{
		archive.Jump(jump_pos);
	}

	modeComboBox.SetSelected(historymode);

	Scene& scene = editor->GetCurrentScene();

	for (auto& selected : editor->translator.selected)
	{
		switch (historymode)
		{
		case PaintToolWindow::MODE_TEXTURE:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
				break;

			MaterialComponent* material = selected.subsetIndex >= 0 && selected.subsetIndex < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[selected.subsetIndex].materialID) : nullptr;
			if (material == nullptr)
				break;

			int slot;
			archive >> slot;
			size_t textureindex;
			archive >> textureindex;

			textureSlotComboBox.SetSelected(slot);
			history_textureIndex = textureindex;
			ReplaceEditTextureSlot(*material, history_textures[history_textureIndex]);

		}
		break;
		case PaintToolWindow::MODE_VERTEXCOLOR:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			MeshComponent archive_mesh;
			archive >> archive_mesh.vertex_colors;

			mesh->vertex_colors = archive_mesh.vertex_colors;

			mesh->CreateRenderData();
		}
		break;
		case PaintToolWindow::MODE_WIND:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			MeshComponent archive_mesh;
			archive >> archive_mesh.vertex_windweights;

			mesh->vertex_windweights = archive_mesh.vertex_windweights;

			mesh->CreateRenderData();
		}
		break;
		case PaintToolWindow::MODE_SCULPTING_ADD:
		case PaintToolWindow::MODE_SCULPTING_SUBTRACT:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				break;

			MeshComponent archive_mesh;
			archive >> archive_mesh.vertex_positions;
			archive >> archive_mesh.vertex_normals;

			mesh->vertex_positions = archive_mesh.vertex_positions;
			mesh->vertex_normals = archive_mesh.vertex_normals;

			mesh->CreateRenderData();
		}
		break;
		case PaintToolWindow::MODE_SOFTBODY_PINNING:
		case PaintToolWindow::MODE_SOFTBODY_PHYSICS:
		{
			ObjectComponent* object = scene.objects.GetComponent(selected.entity);
			if (object == nullptr || object->meshID == INVALID_ENTITY)
				break;

			SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
			if (softbody == nullptr)
				break;

			SoftBodyPhysicsComponent archive_softbody;
			archive >> archive_softbody.weights;

			softbody->weights = archive_softbody.weights;

			softbody->_flags |= SoftBodyPhysicsComponent::FORCE_RESET;
		}
		break;
		case PaintToolWindow::MODE_HAIRPARTICLE_ADD_TRIANGLE:
		case PaintToolWindow::MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
		case PaintToolWindow::MODE_HAIRPARTICLE_LENGTH:
		{
			wi::HairParticleSystem* hair = scene.hairs.GetComponent(selected.entity);
			if (hair == nullptr)
				break;

			wi::HairParticleSystem archive_hair;
			archive >> archive_hair.vertex_lengths;

			hair->vertex_lengths = archive_hair.vertex_lengths;

			hair->_flags |= wi::HairParticleSystem::REBUILD_BUFFERS;
		}
		break;
		default:
			assert(0);
			break;
		}
	}
}
PaintToolWindow::TextureSlot PaintToolWindow::GetEditTextureSlot(const MaterialComponent& material, int* uvset)
{
	uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());
	if (!material.textures[sel].resource.IsValid())
	{
		return {};
	}
	if (uvset)
		*uvset = material.textures[sel].uvset;
	TextureSlot retVal;
	retVal.texture = material.textures[sel].resource.GetTexture();
	retVal.srgb_subresource = material.textures[sel].resource.GetTextureSRGBSubresource();
	return retVal;
}
void PaintToolWindow::ReplaceEditTextureSlot(wi::scene::MaterialComponent& material, const TextureSlot& textureslot)
{
	uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());
	material.textures[sel].resource.SetTexture(textureslot.texture, textureslot.srgb_subresource);
	material.SetDirty();
}

void PaintToolWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 110;
		const float margin_right = 30;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 30;
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

	add(modeComboBox);
	add_fullwidth(infoLabel);
	add_fullwidth(colorPicker);
	add(radiusSlider);
	add(amountSlider);
	add(smoothnessSlider);
	add(spacingSlider);
	add(rotationSlider);
	add(stabilizerSlider);
	add_right(backfaceCheckBox);
	wireCheckBox.SetPos(XMFLOAT2(backfaceCheckBox.GetPos().x - wireCheckBox.GetSize().x - 100, backfaceCheckBox.GetPos().y));
	add_right(pressureCheckBox);
	add(textureSlotComboBox);
	add(brushShapeComboBox);
	add(axisCombo);
	add(saveTextureButton);
	add_right(brushTextureButton);
	add_right(revealTextureButton);
}
