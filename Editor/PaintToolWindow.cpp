#include "stdafx.h"
#include "Editor.h"
#include "PaintToolWindow.h"
#include "shaders/ShaderInterop_Renderer.h"

#include <cmath>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void PaintToolWindow::Create(EditorComponent* editor)
{
	this->editor = editor;

	wi::gui::Window::Create("Paint Tool Window");
	SetSize(XMFLOAT2(400, 600));

	float x = 100;
	float y = 5;
	float hei = 20;
	float step = hei + 4;

	modeComboBox.Create("Mode: ");
	modeComboBox.SetTooltip("Choose paint tool mode");
	modeComboBox.SetPos(XMFLOAT2(x, y += step));
	modeComboBox.SetSize(XMFLOAT2(200, hei));
	modeComboBox.AddItem("Disabled");
	modeComboBox.AddItem("Texture");
	modeComboBox.AddItem("Vertexcolor");
	modeComboBox.AddItem("Sculpting - Add");
	modeComboBox.AddItem("Sculpting - Subtract");
	modeComboBox.AddItem("Softbody - Pinning");
	modeComboBox.AddItem("Softbody - Physics");
	modeComboBox.AddItem("Hairparticle - Add Triangle");
	modeComboBox.AddItem("Hairparticle - Remove Triangle");
	modeComboBox.AddItem("Hairparticle - Length (Alpha)");
	modeComboBox.AddItem("Wind weight (Alpha)");
	modeComboBox.SetSelected(0);
	modeComboBox.OnSelect([&](wi::gui::EventArgs args) {
		textureSlotComboBox.SetEnabled(false);
		saveTextureButton.SetEnabled(false);
		switch (args.iValue)
		{
		case MODE_DISABLED:
			infoLabel.SetText("Paint Tool is disabled.");
			break;
		case MODE_TEXTURE:
			infoLabel.SetText("In texture paint mode, you can paint on textures. Brush will be applied in texture space.\nREMEMBER to save texture when finished to save texture file!\nREMEMBER to save scene to retain new texture bindings on materials!");
			textureSlotComboBox.SetEnabled(true);
			saveTextureButton.SetEnabled(true);
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
	infoLabel.SetSize(XMFLOAT2(400 - 20, 100));
	infoLabel.SetPos(XMFLOAT2(10, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);

	y += infoLabel.GetScale().y - step + 5;

	radiusSlider.Create(1.0f, 500.0f, 50, 10000, "Brush Radius: ");
	radiusSlider.SetTooltip("Set the brush radius in pixel units");
	radiusSlider.SetSize(XMFLOAT2(200, hei));
	radiusSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&radiusSlider);

	amountSlider.Create(0, 1, 1, 10000, "Brush Amount: ");
	amountSlider.SetTooltip("Set the brush amount. 0 = minimum affection, 1 = maximum affection");
	amountSlider.SetSize(XMFLOAT2(200, hei));
	amountSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&amountSlider);

	falloffSlider.Create(0, 16, 0, 10000, "Brush Falloff: ");
	falloffSlider.SetTooltip("Set the brush power. 0 = no falloff, 1 = linear falloff, more = falloff power");
	falloffSlider.SetSize(XMFLOAT2(200, hei));
	falloffSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&falloffSlider);

	spacingSlider.Create(0, 500, 1, 500, "Brush Spacing: ");
	spacingSlider.SetTooltip("Brush spacing means how much brush movement (in pixels) starts a new stroke. 0 = new stroke every frame, 100 = every 100 pixel movement since last stroke will start a new stroke.");
	spacingSlider.SetSize(XMFLOAT2(200, hei));
	spacingSlider.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&spacingSlider);

	backfaceCheckBox.Create("Backfaces: ");
	backfaceCheckBox.SetTooltip("Set whether to paint on backfaces of geometry or not");
	backfaceCheckBox.SetSize(XMFLOAT2(hei, hei));
	backfaceCheckBox.SetPos(XMFLOAT2(x, y += step));
	AddWidget(&backfaceCheckBox);

	wireCheckBox.Create("Wireframe: ");
	wireCheckBox.SetTooltip("Set whether to draw wireframe on top of geometry or not");
	wireCheckBox.SetSize(XMFLOAT2(hei, hei));
	wireCheckBox.SetPos(XMFLOAT2(x + 100, y));
	wireCheckBox.SetCheck(true);
	AddWidget(&wireCheckBox);

	pressureCheckBox.Create("Pressure: ");
	pressureCheckBox.SetTooltip("Set whether to use pressure sensitivity (for example pen tablet)");
	pressureCheckBox.SetSize(XMFLOAT2(hei, hei));
	pressureCheckBox.SetPos(XMFLOAT2(x + 200, y));
	pressureCheckBox.SetCheck(false);
	AddWidget(&pressureCheckBox);

	textureSlotComboBox.Create("Texture Slot: ");
	textureSlotComboBox.SetTooltip("Choose texture slot of the selected material to paint (texture paint mode only)");
	textureSlotComboBox.SetPos(XMFLOAT2(x, y += step));
	textureSlotComboBox.SetSize(XMFLOAT2(200, hei));
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
	textureSlotComboBox.SetEnabled(false);
	AddWidget(&textureSlotComboBox);

	saveTextureButton.Create("Save Texture");
	saveTextureButton.SetTooltip("Save edited texture.");
	saveTextureButton.SetSize(XMFLOAT2(200, hei));
	saveTextureButton.SetPos(XMFLOAT2(x, y += step));
	saveTextureButton.SetEnabled(false);
	saveTextureButton.OnClick([this] (wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			return;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
			return;

		MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
		if (material == nullptr)
			return;

		Texture editTexture = GetEditTextureSlot(*material);

		uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());

		wi::vector<uint8_t> texturefiledata;
		if (wi::helper::saveTextureToMemoryFile(editTexture, "PNG", texturefiledata))
		{
			material->textures[sel].resource.SetFileData(texturefiledata);
		}
		else
		{
			wi::helper::messageBox("Saving texture failed! :(");
		}

	});
	AddWidget(&saveTextureButton);

	colorPicker.Create("Color", false);
	colorPicker.SetPos(XMFLOAT2(10, y += step));
	AddWidget(&colorPicker);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 550, 50, 0));
	SetVisible(false);
}

void PaintToolWindow::Update(float dt)
{
	RecordHistory(false);

	rot -= dt;
	// by default, paint tool is on center of screen, this makes it easy to tweak radius with GUI:
	XMFLOAT2 posNew;
	posNew.x = editor->GetLogicalWidth() * 0.5f;
	posNew.y = editor->GetLogicalHeight() * 0.5f;
	if (editor->GetGUI().HasFocus() || wi::backlog::isActive() || entity == INVALID_ENTITY)
	{
		pos = posNew;
		return;
	}

	const bool pointer_down = wi::input::Down(wi::input::MOUSE_BUTTON_LEFT);
	if (!pointer_down)
	{
		stroke_dist = FLT_MAX;
	}

	auto pointer = wi::input::GetPointer();
	posNew = XMFLOAT2(pointer.x, pointer.y);
	stroke_dist += wi::math::Distance(pos, posNew);
	const float pressure = pressureCheckBox.GetCheck() ? pointer.w : 1.0f;

	const float spacing = spacingSlider.GetValue();
	const bool pointer_moved = stroke_dist >= spacing;
	const bool painting = pointer_moved && pointer_down;

	if (painting)
	{
		stroke_dist = 0;
	}

	pos = posNew;

	const MODE mode = GetMode();
	const float radius = radiusSlider.GetValue();
	const float pressure_radius = radius * pressure;
	const float amount = amountSlider.GetValue();
	const float falloff = falloffSlider.GetValue();
	const wi::Color color = colorPicker.GetPickColor();
	const XMFLOAT4 color_float = color.toFloat4();
	const bool backfaces = backfaceCheckBox.GetCheck();
	const bool wireframe = wireCheckBox.GetCheck();

	Scene& scene = wi::scene::GetScene();
	const CameraComponent& camera = wi::scene::GetCamera();
	const XMVECTOR C = XMLoadFloat2(&pos);
	const XMMATRIX VP = camera.GetViewProjection();
	const XMVECTOR MUL = XMVectorSet(0.5f, -0.5f, 1, 1);
	const XMVECTOR ADD = XMVectorSet(0.5f, 0.5f, 0, 0);
	const XMVECTOR SCREEN = XMVectorSet((float)editor->GetLogicalWidth(), (float)editor->GetLogicalHeight(), 1, 1);
	const XMVECTOR F = camera.GetAt();

	switch (mode)
	{
	case MODE_TEXTURE:
	{
		const wi::scene::PickResult& intersect = editor->hovered;
		if (intersect.entity != entity)
			break;

		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
			break;

		MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
		if (material == nullptr)
			break;

		int uvset = 0;
		Texture editTexture = GetEditTextureSlot(*material, &uvset);
		if (!editTexture.IsValid())
			break;
		const TextureDesc& desc = editTexture.GetDesc();
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
		uint2 center = XMUINT2(uint32_t(uv.x * desc.width), uint32_t(uv.y * desc.height));

		if (painting)
		{
			GraphicsDevice* device = wi::graphics::GetDevice();
			CommandList cmd = device->BeginCommandList();

			RecordHistory(true, cmd);

			// Need to requery this because RecordHistory might swap textures on material:
			editTexture = GetEditTextureSlot(*material, &uvset);

			device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_PAINT_TEXTURE), cmd);

			device->BindResource(wi::texturehelper::getWhite(), 0, cmd);
			device->BindUAV(&editTexture, 0, cmd);

			PaintTextureCB cb;
			cb.xPaintBrushCenter = center;
			cb.xPaintBrushRadius = (uint32_t)pressure_radius;
			cb.xPaintBrushAmount = amount;
			cb.xPaintBrushFalloff = falloff;
			cb.xPaintBrushColor = color.rgba;
			device->PushConstants(&cb, sizeof(cb), cmd);

			const uint diameter = cb.xPaintBrushRadius * 2;
			const uint dispatch_dim = (diameter + PAINT_TEXTURE_BLOCKSIZE - 1) / PAINT_TEXTURE_BLOCKSIZE;
			device->Dispatch(dispatch_dim, dispatch_dim, 1, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Memory(),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);


			wi::renderer::GenerateMipChain(editTexture, wi::renderer::MIPGENFILTER::MIPGENFILTER_LINEAR, cmd);
		}

		wi::renderer::PaintRadius paintrad;
		paintrad.objectEntity = entity;
		paintrad.subset = subset;
		paintrad.radius = radius;
		paintrad.center = center;
		paintrad.uvset = uvset;
		paintrad.dimensions.x = desc.width;
		paintrad.dimensions.y = desc.height;
		wi::renderer::DrawPaintRadius(paintrad);
	}
	break;

	case MODE_VERTEXCOLOR:
	case MODE_WIND:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
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

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
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

				if (subset >= 0 && mesh->vertex_subsets[j] != subset)
					continue;

				const float z = XMVectorGetZ(P);
				const float dist = XMVectorGetX(XMVector2Length(C - P));
				if (z >= 0 && z <= 1 && dist <= pressure_radius)
				{
					RecordHistory(true);
					rebuild = true;
					const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);

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
			for (size_t j = 0; j < mesh->indices.size(); j += 3)
			{
				const uint32_t triangle[] = {
					mesh->indices[j + 0],
					mesh->indices[j + 1],
					mesh->indices[j + 2],
				};
				if (subset >= 0 && (mesh->vertex_subsets[triangle[0]] != subset|| mesh->vertex_subsets[triangle[1]] != subset|| mesh->vertex_subsets[triangle[2]] != subset))
					continue;

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

		if (rebuild)
		{
			mesh->CreateRenderData();
		}
	}
	break;

	case MODE_SCULPTING_ADD:
	case MODE_SCULPTING_SUBTRACT:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (transform == nullptr)
			break;

		const XMMATRIX W = XMLoadFloat4x4(&transform->world);

		bool rebuild = false;
		if (painting)
		{
			XMVECTOR averageNormal = XMVectorZero();
			struct PaintVert
			{
				size_t ind;
				float affection;
			};
			static wi::vector<PaintVert> paintindices;
			paintindices.clear();
			paintindices.reserve(mesh->vertex_positions.size());

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
					averageNormal += N;
					const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);
					paintindices.push_back({ j, affection });
				}
			}

			if (!paintindices.empty())
			{
				RecordHistory(true);
				rebuild = true;
				averageNormal = XMVector3Normalize(averageNormal);

				for (auto& x : paintindices)
				{
					XMVECTOR PL = XMLoadFloat3(&mesh->vertex_positions[x.ind]);
					switch (mode)
					{
					case MODE_SCULPTING_ADD:
						PL += averageNormal * x.affection;
						break;
					case MODE_SCULPTING_SUBTRACT:
						PL -= averageNormal * x.affection;
						break;
					}
					XMStoreFloat3(&mesh->vertex_positions[x.ind], PL);
				}
			}
		}

		if (wireframe)
		{
			for (size_t j = 0; j < mesh->indices.size(); j += 3)
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
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
		if (softbody == nullptr || softbody->vertex_positions_simulation.empty())
			break;

		// Painting:
		if (pointer_moved && wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
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
		for (size_t j = 0; j < mesh->indices.size(); j += 3)
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
			if (softbody->vertex_positions_simulation.empty())
			{
				XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex0]), W));
				XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex1]), W));
				XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[graphicsIndex2]), W));
			}
			else
			{
				tri.positionA = softbody->vertex_positions_simulation[graphicsIndex0].pos;
				tri.positionB = softbody->vertex_positions_simulation[graphicsIndex1].pos;
				tri.positionC = softbody->vertex_positions_simulation[graphicsIndex2].pos;
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
	break;

	case MODE_HAIRPARTICLE_ADD_TRIANGLE:
	case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
	case MODE_HAIRPARTICLE_LENGTH:
	{
		wi::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
		if (hair == nullptr || hair->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(hair->meshID);
		if (mesh == nullptr)
			break;

		const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
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
							const float affection = amount * std::pow(1.0f - (dist / pressure_radius), falloff);
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
			for (size_t j = 0; j < mesh->indices.size(); j += 3)
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
void PaintToolWindow::DrawBrush() const
{
	const MODE mode = GetMode();
	if (mode == MODE_DISABLED || mode == MODE_TEXTURE || entity == INVALID_ENTITY || wi::backlog::isActive())
		return;

	const int segmentcount = 36;
	const float radius = radiusSlider.GetValue();

	for (int i = 0; i < segmentcount; i += 1)
	{
		const float angle0 = rot + (float)i / (float)segmentcount * XM_2PI;
		const float angle1 = rot + (float)(i + 1) / (float)segmentcount * XM_2PI;
		wi::renderer::RenderableLine2D line;
		line.start.x = pos.x + sinf(angle0) * radius;
		line.start.y = pos.y + cosf(angle0) * radius;
		line.end.x = pos.x + sinf(angle1) * radius;
		line.end.y = pos.y + cosf(angle1) * radius;
		line.color_end = line.color_start = i%2 == 0 ? XMFLOAT4(0, 0, 0, 0.8f): XMFLOAT4(1,1,1,1);
		wi::renderer::DrawLine(line);
	}

}

PaintToolWindow::MODE PaintToolWindow::GetMode() const
{
	return (MODE)modeComboBox.GetSelected();
}
void PaintToolWindow::SetEntity(wi::ecs::Entity value, int subsetindex)
{
	entity = value;
	subset = subsetindex;

	if (entity == INVALID_ENTITY)
	{
		saveTextureButton.SetEnabled(false);
	}
	else if (GetMode() == MODE_TEXTURE)
	{
		saveTextureButton.SetEnabled(true);
	}
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
		archive << entity;
	}
	else
	{
		if (!history_needs_recording_end || wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
			return;
		history_needs_recording_end = false;
		history_needs_recording_start = false;

		// End history recording (redo)
	}

	wi::Archive& archive = *currentHistory;
	Scene& scene = wi::scene::GetScene();

	switch (GetMode())
	{
	case PaintToolWindow::MODE_TEXTURE:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
			break;

		MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
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
			Texture newTex;
			TextureDesc desc = editTexture.GetDesc();
			desc.format = Format::R8G8B8A8_UNORM; // force format to one that is writable by GPU
			device->CreateTexture(&desc, nullptr, &newTex);
			for (uint32_t i = 0; i < newTex.GetDesc().mip_levels; ++i)
			{
				int subresource_index;
				subresource_index = device->CreateSubresource(&newTex, SubresourceType::SRV, 0, 1, i, 1);
				assert(subresource_index == i);
				subresource_index = device->CreateSubresource(&newTex, SubresourceType::UAV, 0, 1, i, 1);
				assert(subresource_index == i);
			}
			assert(cmd != wi::graphics::INVALID_COMMANDLIST);
			wi::renderer::CopyTexture2D(newTex, -1, 0, 0, editTexture, 0, cmd);
			ReplaceEditTextureSlot(*material, newTex);
		}

	}
	break;
	case PaintToolWindow::MODE_VERTEXCOLOR:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
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
		ObjectComponent* object = scene.objects.GetComponent(entity);
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
		ObjectComponent* object = scene.objects.GetComponent(entity);
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
		ObjectComponent* object = scene.objects.GetComponent(entity);
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
		wi::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
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
void PaintToolWindow::ConsumeHistoryOperation(wi::Archive& archive, bool undo)
{
	MODE historymode;
	archive >> (uint32_t&)historymode;
	archive >> entity;

	modeComboBox.SetSelected(historymode);

	Scene& scene = wi::scene::GetScene();

	switch (historymode)
	{
	case PaintToolWindow::MODE_TEXTURE:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr || (mesh->vertex_uvset_0.empty() && mesh->vertex_uvset_1.empty()))
			break;

		MaterialComponent* material = subset >= 0 && subset < (int)mesh->subsets.size() ? scene.materials.GetComponent(mesh->subsets[subset].materialID) : nullptr;
		if (material == nullptr)
			break;

		int undo_slot;
		archive >> undo_slot;
		size_t undo_textureindex;
		archive >> undo_textureindex;
		int redo_slot;
		archive >> redo_slot;
		size_t redo_textureindex;
		archive >> redo_textureindex;

		if (undo)
		{
			textureSlotComboBox.SetSelected(undo_slot);
			history_textureIndex = undo_textureindex;
		}
		else
		{
			textureSlotComboBox.SetSelected(redo_slot);
			history_textureIndex = redo_textureindex;
		}
		ReplaceEditTextureSlot(*material, history_textures[history_textureIndex]);

	}
	break;
	case PaintToolWindow::MODE_VERTEXCOLOR:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		MeshComponent undo_mesh;
		archive >> undo_mesh.vertex_colors;
		MeshComponent redo_mesh;
		archive >> redo_mesh.vertex_colors;

		if (undo)
		{
			mesh->vertex_colors = undo_mesh.vertex_colors;
		}
		else
		{
			mesh->vertex_colors = redo_mesh.vertex_colors;
		}

		mesh->CreateRenderData();
	}
	break;
	case PaintToolWindow::MODE_WIND:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		MeshComponent undo_mesh;
		archive >> undo_mesh.vertex_windweights;
		MeshComponent redo_mesh;
		archive >> redo_mesh.vertex_windweights;

		if (undo)
		{
			mesh->vertex_windweights = undo_mesh.vertex_windweights;
		}
		else
		{
			mesh->vertex_windweights = redo_mesh.vertex_windweights;
		}

		mesh->CreateRenderData();
	}
	break;
	case PaintToolWindow::MODE_SCULPTING_ADD:
	case PaintToolWindow::MODE_SCULPTING_SUBTRACT:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
		if (mesh == nullptr)
			break;

		MeshComponent undo_mesh;
		archive >> undo_mesh.vertex_positions;
		archive >> undo_mesh.vertex_normals;
		MeshComponent redo_mesh;
		archive >> redo_mesh.vertex_positions;
		archive >> redo_mesh.vertex_normals;

		if (undo)
		{
			mesh->vertex_positions = undo_mesh.vertex_positions;
			mesh->vertex_normals = undo_mesh.vertex_normals;
		}
		else
		{
			mesh->vertex_positions = redo_mesh.vertex_positions;
			mesh->vertex_normals = redo_mesh.vertex_normals;
		}

		mesh->CreateRenderData();
	}
	break;
	case PaintToolWindow::MODE_SOFTBODY_PINNING:
	case PaintToolWindow::MODE_SOFTBODY_PHYSICS:
	{
		ObjectComponent* object = scene.objects.GetComponent(entity);
		if (object == nullptr || object->meshID == INVALID_ENTITY)
			break;

		SoftBodyPhysicsComponent* softbody = scene.softbodies.GetComponent(object->meshID);
		if (softbody == nullptr)
			break;

		SoftBodyPhysicsComponent undo_softbody;
		archive >> undo_softbody.weights;
		SoftBodyPhysicsComponent redo_softbody;
		archive >> redo_softbody.weights;

		if (undo)
		{
			softbody->weights = undo_softbody.weights;
		}
		else
		{
			softbody->weights = redo_softbody.weights;
		}

		softbody->_flags |= SoftBodyPhysicsComponent::FORCE_RESET;
	}
	break;
	case PaintToolWindow::MODE_HAIRPARTICLE_ADD_TRIANGLE:
	case PaintToolWindow::MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
	case PaintToolWindow::MODE_HAIRPARTICLE_LENGTH:
	{
		wi::HairParticleSystem* hair = scene.hairs.GetComponent(entity);
		if (hair == nullptr)
			break;

		wi::HairParticleSystem undo_hair;
		archive >> undo_hair.vertex_lengths;
		wi::HairParticleSystem redo_hair;
		archive >> redo_hair.vertex_lengths;

		if (undo)
		{
			hair->vertex_lengths = undo_hair.vertex_lengths;
		}
		else
		{
			hair->vertex_lengths = redo_hair.vertex_lengths;
		}

		hair->_flags |= wi::HairParticleSystem::REBUILD_BUFFERS;
	}
	break;
	default:
		assert(0);
		break;
	}
}
Texture PaintToolWindow::GetEditTextureSlot(const MaterialComponent& material, int* uvset)
{
	uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());
	if (!material.textures[sel].resource.IsValid())
	{
		return Texture();
	}
	if (uvset)
		*uvset = material.textures[sel].uvset;
	return material.textures[sel].resource.GetTexture();
}
void PaintToolWindow::ReplaceEditTextureSlot(wi::scene::MaterialComponent& material, const Texture& texture)
{
	uint64_t sel = textureSlotComboBox.GetItemUserData(textureSlotComboBox.GetSelected());
	material.textures[sel].resource.SetTexture(texture);
	material.SetDirty();
}
