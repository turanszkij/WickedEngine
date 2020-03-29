#include "stdafx.h"
#include "Editor.h"
#include "PaintToolWindow.h"
#include "ShaderInterop_Paint.h"

using namespace wiECS;
using namespace wiScene;
using namespace wiGraphics;

PaintToolWindow::PaintToolWindow(EditorComponent* editor) : editor(editor)
{
	window = new wiWindow(&editor->GetGUI(), "Paint Tool Window");
	window->SetSize(XMFLOAT2(400, 630));
	editor->GetGUI().AddWidget(window);

	float x = 100;
	float y = 10;
	float step = 30;

	modeComboBox = new wiComboBox("Mode: ");
	modeComboBox->SetTooltip("Choose paint tool mode");
	modeComboBox->SetPos(XMFLOAT2(x, y += step));
	modeComboBox->SetSize(XMFLOAT2(200, 28));
	modeComboBox->AddItem("Disabled");
	modeComboBox->AddItem("Texture");
	modeComboBox->AddItem("Vertexcolor");
	modeComboBox->AddItem("Sculpting - Add");
	modeComboBox->AddItem("Sculpting - Subtract");
	modeComboBox->AddItem("Softbody - Pinning");
	modeComboBox->AddItem("Softbody - Physics");
	modeComboBox->AddItem("Hairparticle - Add Triangle");
	modeComboBox->AddItem("Hairparticle - Remove Triangle");
	modeComboBox->AddItem("Hairparticle - Length (Alpha)");
	modeComboBox->SetSelected(0);
	modeComboBox->OnSelect([&](wiEventArgs args) {
		textureDestComboBox->SetEnabled(false);
		switch (args.iValue)
		{
		case MODE_DISABLED:
			infoLabel->SetText("Paint Tool is disabled.");
			break;
		case MODE_TEXTURE:
			infoLabel->SetText("In texture paint mode, you can paint on textures. Brush will be applied in texture space.\nWORK IN PROGRESS! EDITED TEXTURES ARE NOT SAVED YET!");
			textureDestComboBox->SetEnabled(true);
			break;
		case MODE_VERTEXCOLOR:
			infoLabel->SetText("In vertex color mode, you can paint colors on selected geometry (per vertex). \"Use vertex colors\" will be automatically enabled for the selected material, or all materials if the whole object is selected. If there is no vertexcolors vertex buffer, one will be created with white as default for every vertex.");
			break;
		case MODE_SCULPTING_ADD:
			infoLabel->SetText("In sculpt - ADD mode, you can modify vertex positions by ADD operation along normal vector. Normals are not modified, so recompute them in MeshWindow if needed.");
			break;
		case MODE_SCULPTING_SUBTRACT:
			infoLabel->SetText("In sculpt - SUBTRACT mode, you can modify vertex positions by SUBTRACT operation along normal vector. Normals are not modified, so recompute them in MeshWindow if needed.");
			break;
		case MODE_SOFTBODY_PINNING:
			infoLabel->SetText("In soft body pinning mode, the selected object's soft body vertices can be pinned down (so they will be fixed and drive physics)");
			break;
		case MODE_SOFTBODY_PHYSICS:
			infoLabel->SetText("In soft body physics mode, the selected object's soft body vertices can be unpinned (so they will be simulated by physics)");
			break;
		case MODE_HAIRPARTICLE_ADD_TRIANGLE:
			infoLabel->SetText("In hair particle add triangle mode, you can add triangles to the hair base mesh.\nThis will modify random distribution of hair!");
			break;
		case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
			infoLabel->SetText("In hair particle remove triangle mode, you can remove triangles from the hair base mesh.\nThis will modify random distribution of hair!");
			break;
		case MODE_HAIRPARTICLE_LENGTH:
			infoLabel->SetText("In hair particle length mode, you can adjust length of hair particles with the colorpicker Alpha channel (A). The Alpha channel is 0-255, but the length will be normalized to 0-1 range.\nThis will NOT modify random distribution of hair!");
			break;
		}
		});
	window->AddWidget(modeComboBox);

	y += step + 5;

	infoLabel = new wiLabel("Paint Tool is disabled.");
	infoLabel->SetSize(XMFLOAT2(400 - 20, 100));
	infoLabel->SetPos(XMFLOAT2(10, y));
	infoLabel->SetColor(wiColor::Transparent());
	window->AddWidget(infoLabel);
	y += infoLabel->GetScale().y - step + 5;

	radiusSlider = new wiSlider(1.0f, 500.0f, 50, 10000, "Brush Radius: ");
	radiusSlider->SetTooltip("Set the brush radius in pixel units");
	radiusSlider->SetSize(XMFLOAT2(200, 20));
	radiusSlider->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(radiusSlider);

	amountSlider = new wiSlider(0, 1, 1, 10000, "Brush Amount: ");
	amountSlider->SetTooltip("Set the brush amount. 0 = minimum affection, 1 = maximum affection");
	amountSlider->SetSize(XMFLOAT2(200, 20));
	amountSlider->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(amountSlider);

	falloffSlider = new wiSlider(0, 16, 0, 10000, "Brush Falloff: ");
	falloffSlider->SetTooltip("Set the brush power. 0 = no falloff, 1 = linear falloff, more = falloff power");
	falloffSlider->SetSize(XMFLOAT2(200, 20));
	falloffSlider->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(falloffSlider);

	spacingSlider = new wiSlider(0, 500, 1, 500, "Brush Spacing: ");
	spacingSlider->SetTooltip("Brush spacing means how much brush movement (in pixels) starts a new stroke. 0 = new stroke every frame, 100 = every 100 pixel movement since last stroke will start a new stroke.");
	spacingSlider->SetSize(XMFLOAT2(200, 20));
	spacingSlider->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(spacingSlider);

	backfaceCheckBox = new wiCheckBox("Backfaces: ");
	backfaceCheckBox->SetTooltip("Set whether to paint on backfaces of geometry or not");
	backfaceCheckBox->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(backfaceCheckBox);

	wireCheckBox = new wiCheckBox("Wireframe: ");
	wireCheckBox->SetTooltip("Set whether to draw wireframe on top of geometry or not");
	wireCheckBox->SetPos(XMFLOAT2(x + 100, y));
	wireCheckBox->SetCheck(true);
	window->AddWidget(wireCheckBox);

	textureDestComboBox = new wiComboBox("Texture Dest: ");
	textureDestComboBox->SetTooltip("Choose texture slot to paint (texture paint mode only)");
	textureDestComboBox->SetPos(XMFLOAT2(x, y += step));
	textureDestComboBox->SetSize(XMFLOAT2(200, 28));
	textureDestComboBox->AddItem("BaseColor");
	textureDestComboBox->AddItem("Normal");
	textureDestComboBox->AddItem("SurfaceMap");
	textureDestComboBox->AddItem("DisplacementMap");
	textureDestComboBox->AddItem("EmissiveMap");
	textureDestComboBox->AddItem("OcclusionMap");
	textureDestComboBox->SetSelected(0);
	textureDestComboBox->SetEnabled(false);
	window->AddWidget(textureDestComboBox);

	colorPicker = new wiColorPicker(&editor->GetGUI(), "Color", false);
	colorPicker->SetPos(XMFLOAT2(10, y += step));
	window->AddWidget(colorPicker);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 550, 50, 0));
	window->SetVisible(false);
}
PaintToolWindow::~PaintToolWindow()
{
	window->RemoveWidgets(true);
	editor->GetGUI().RemoveWidget(window);
	delete window;
}

void PaintToolWindow::Update(float dt)
{
	RecordHistory(false);

	rot -= dt;
	// by default, paint tool is on center of screen, this makes it easy to tweak radius with GUI:
	XMFLOAT2 posNew;
	posNew.x = wiRenderer::GetDevice()->GetScreenWidth() * 0.5f;
	posNew.y = wiRenderer::GetDevice()->GetScreenHeight() * 0.5f;
	if (editor->GetGUI().HasFocus() || wiBackLog::isActive() || entity == INVALID_ENTITY)
	{
		pos = posNew;
		return;
	}

	const bool pointer_down = wiInput::Down(wiInput::MOUSE_BUTTON_LEFT);
	if (!pointer_down)
	{
		stroke_dist = FLT_MAX;
	}

	auto pointer = wiInput::GetPointer();
	posNew = XMFLOAT2(pointer.x, pointer.y);
	stroke_dist += wiMath::Distance(pos, posNew);

	const float spacing = spacingSlider->GetValue();
	const bool pointer_moved = stroke_dist >= spacing;
	const bool painting = pointer_moved && pointer_down;

	if (painting)
	{
		stroke_dist = 0;
	}

	pos = posNew;

	const MODE mode = GetMode();
	const float radius = radiusSlider->GetValue();
	const float amount = amountSlider->GetValue();
	const float falloff = falloffSlider->GetValue();
	const wiColor color = colorPicker->GetPickColor();
	const XMFLOAT4 color_float = color.toFloat4();
	const bool backfaces = backfaceCheckBox->GetCheck();
	const bool wireframe = wireCheckBox->GetCheck();

	Scene& scene = wiScene::GetScene();
	const CameraComponent& camera = wiRenderer::GetCamera();
	const XMVECTOR C = XMLoadFloat2(&pos);
	const XMMATRIX VP = camera.GetViewProjection();
	const XMVECTOR MUL = XMVectorSet(0.5f, -0.5f, 1, 1);
	const XMVECTOR ADD = XMVectorSet(0.5f, 0.5f, 0, 0);
	const XMVECTOR SCREEN = XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 1, 1);
	const XMVECTOR F = camera.GetAt();

	switch (mode)
	{
	case MODE_TEXTURE:
	{
		const wiScene::PickResult& intersect = editor->hovered;
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
		const Texture* texture = GetEditTextureSlot(*material, &uvset);
		const TextureDesc& desc = texture->GetDesc();
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
		uint2 center = XMUINT2(uint32_t(uv.x * desc.Width), uint32_t(uv.y * desc.Height));

		if (painting)
		{
			RecordHistory(true);

			GraphicsDevice* device = wiRenderer::GetDevice();

			static GPUBuffer cbuf;
			if (!cbuf.IsValid())
			{
				GPUBufferDesc desc;
				desc.BindFlags = BIND_CONSTANT_BUFFER;
				desc.Usage = USAGE_DYNAMIC;
				desc.CPUAccessFlags = CPU_ACCESS_WRITE;
				desc.ByteWidth = sizeof(PaintTextureCB);
				device->CreateBuffer(&desc, nullptr, &cbuf);
			}

			CommandList cmd = device->BeginCommandList();
			device->BindComputeShader(wiRenderer::GetComputeShader(CSTYPE_PAINT_TEXTURE), cmd);

			wiRenderer::BindCommonResources(cmd);
			device->BindResource(CS, wiTextureHelper::getWhite(), TEXSLOT_ONDEMAND0, cmd);
			device->BindUAV(CS, texture, 0, cmd);

			PaintTextureCB cb;
			cb.xPaintBrushCenter = center;
			cb.xPaintBrushRadius = (uint32_t)radius;
			cb.xPaintBrushAmount = amount;
			cb.xPaintBrushFalloff = falloff;
			cb.xPaintBrushColor = color.rgba;
			device->UpdateBuffer(&cbuf, &cb, cmd);
			device->BindConstantBuffer(CS, &cbuf, CB_GETBINDSLOT(PaintTextureCB), cmd);

			const uint diameter = cb.xPaintBrushRadius * 2;
			const uint dispatch_dim = (diameter + PAINT_TEXTURE_BLOCKSIZE - 1) / PAINT_TEXTURE_BLOCKSIZE;
			device->Dispatch(dispatch_dim, dispatch_dim, 1, cmd);

			device->Barrier(&GPUBarrier::Memory(), 1, cmd);
			device->UnbindUAVs(0, 1, cmd);

			wiRenderer::GenerateMipChain(*texture, wiRenderer::MIPGENFILTER::MIPGENFILTER_LINEAR, cmd);
		}

		wiRenderer::PaintRadius paintrad;
		paintrad.objectEntity = entity;
		paintrad.subset = subset;
		paintrad.radius = radius;
		paintrad.center = center;
		paintrad.uvset = uvset;
		wiRenderer::DrawPaintRadius(paintrad);
	}
	break;

	case MODE_VERTEXCOLOR:
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
					material->SetUseVertexColors(true);
				}
			}
			material = nullptr;
		}
		else
		{
			material->SetUseVertexColors(true);
		}
		
		const ArmatureComponent* armature = mesh->IsSkinned() ? scene.armatures.GetComponent(mesh->armatureID) : nullptr;

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (transform == nullptr)
			break;

		const XMMATRIX W = XMLoadFloat4x4(&transform->world);

		bool rebuild = false;
		if (mesh->vertex_colors.empty())
		{
			mesh->vertex_colors.resize(mesh->vertex_positions.size());
			std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), 0xFFFFFFFF); // fill white
			rebuild = true;
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
					P = wiScene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
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
				if (z >= 0 && z <= 1 && dist <= radius)
				{
					RecordHistory(true);
					wiColor vcol = mesh->vertex_colors[j];
					const float affection = amount * std::powf(1 - (dist / radius), falloff);
					vcol = wiColor::lerp(vcol, color, affection);
					mesh->vertex_colors[j] = vcol.rgba;
					rebuild = true;
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
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wiScene::SkinVertex(*mesh, *armature, triangle[0]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wiScene::SkinVertex(*mesh, *armature, triangle[1]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wiScene::SkinVertex(*mesh, *armature, triangle[2]), W),
				};

				wiRenderer::RenderableTriangle tri;
				XMStoreFloat3(&tri.positionA, P[0]);
				XMStoreFloat3(&tri.positionB, P[1]);
				XMStoreFloat3(&tri.positionC, P[2]);
				tri.colorA.w = 0.8f;
				tri.colorB.w = 0.8f;
				tri.colorC.w = 0.8f;
				wiRenderer::DrawTriangle(tri, true);
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
					P = wiScene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
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
				if (z >= 0 && z <= 1 && dist <= radius)
				{
					RecordHistory(true);
					XMVECTOR PL = XMLoadFloat3(&mesh->vertex_positions[j]);
					const XMVECTOR NL = XMLoadFloat3(&mesh->vertex_normals[j]);
					const float affection = amount * std::powf(1 - (dist / radius), falloff);
					switch (mode)
					{
					case MODE_SCULPTING_ADD:
						PL += NL * affection;
						break;
					case MODE_SCULPTING_SUBTRACT:
						PL -= NL * affection;
						break;
					}
					XMStoreFloat3(&mesh->vertex_positions[j], PL);
					rebuild = true;
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
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wiScene::SkinVertex(*mesh, *armature, triangle[0]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wiScene::SkinVertex(*mesh, *armature, triangle[1]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wiScene::SkinVertex(*mesh, *armature, triangle[2]), W),
				};

				wiRenderer::RenderableTriangle tri;
				XMStoreFloat3(&tri.positionA, P[0]);
				XMStoreFloat3(&tri.positionB, P[1]);
				XMStoreFloat3(&tri.positionC, P[2]);
				tri.colorA.w = 0.8f;
				tri.colorB.w = 0.8f;
				tri.colorC.w = 0.8f;
				wiRenderer::DrawTriangle(tri, true);
			}
		}

		if (rebuild)
		{
			mesh->CreateRenderData();
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
		if (pointer_moved && wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
		{
			size_t j = 0;
			for (auto& ind : softbody->physicsToGraphicsVertexMapping)
			{
				XMVECTOR P = softbody->vertex_positions_simulation[ind].LoadPOS();
				P = XMVector3TransformCoord(P, VP);
				P = P * MUL + ADD;
				P = P * SCREEN;
				const float z = XMVectorGetZ(P);
				if (z >= 0 && z <= 1 && XMVectorGetX(XMVector2Length(C - P)) <= radius)
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
			wiRenderer::RenderableTriangle tri;
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
				wiRenderer::DrawTriangle(tri, true);
			}
			if (weight0 == 0 && weight1 == 0 && weight2 == 0)
			{
				tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 0, 0.8f);
				wiRenderer::DrawTriangle(tri);
			}
		}
	}
	break;

	case MODE_HAIRPARTICLE_ADD_TRIANGLE:
	case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
	case MODE_HAIRPARTICLE_LENGTH:
	{
		wiHairParticle* hair = scene.hairs.GetComponent(entity);
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
					P = wiScene::SkinVertex(*mesh, *armature, (uint32_t)j, &N);
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
				if (z >= 0 && z <= 1 && dist <= radius)
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
							const float affection = amount * std::powf(1 - (dist / radius), falloff);
							hair->vertex_lengths[j] = wiMath::Lerp(hair->vertex_lengths[j], color_float.w, affection);
							// don't let it "remove" the vertex by keeping its length above zero:
							//	(because if removed, distribution also changes which might be distracting)
							hair->vertex_lengths[j] = wiMath::Clamp(hair->vertex_lengths[j], 1.0f / 255.0f, 1.0f);
						}
						break;
					}
					hair->_flags |= wiHairParticle::REBUILD_BUFFERS;
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
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wiScene::SkinVertex(*mesh, *armature, triangle[0]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wiScene::SkinVertex(*mesh, *armature, triangle[1]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wiScene::SkinVertex(*mesh, *armature, triangle[2]), W),
				};

				wiRenderer::RenderableTriangle tri;
				XMStoreFloat3(&tri.positionA, P[0]);
				XMStoreFloat3(&tri.positionB, P[1]);
				XMStoreFloat3(&tri.positionC, P[2]);
				wiRenderer::DrawTriangle(tri, true);
			}

			for (size_t j = 0; j < hair->indices.size() && wireframe; j += 3)
			{
				const uint32_t triangle[] = {
					hair->indices[j + 0],
					hair->indices[j + 1],
					hair->indices[j + 2],
				};
				const XMVECTOR P[arraysize(triangle)] = {
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[0]]) : wiScene::SkinVertex(*mesh, *armature, triangle[0]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[1]]) : wiScene::SkinVertex(*mesh, *armature, triangle[1]), W),
					XMVector3Transform(armature == nullptr ? XMLoadFloat3(&mesh->vertex_positions[triangle[2]]) : wiScene::SkinVertex(*mesh, *armature, triangle[2]), W),
				};

				wiRenderer::RenderableTriangle tri;
				XMStoreFloat3(&tri.positionA, P[0]);
				XMStoreFloat3(&tri.positionB, P[1]);
				XMStoreFloat3(&tri.positionC, P[2]);
				tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 1, 0.9f);
				wiRenderer::DrawTriangle(tri, false);
			}
		}
	}
	break;

	}
}
void PaintToolWindow::DrawBrush() const
{
	const MODE mode = GetMode();
	if (mode == MODE_DISABLED || mode == MODE_TEXTURE || entity == INVALID_ENTITY || wiBackLog::isActive())
		return;

	const int segmentcount = 36;
	const float radius = radiusSlider->GetValue();

	for (int i = 0; i < segmentcount; i += 1)
	{
		const float angle0 = (float)i / (float)segmentcount * XM_2PI;
		const float angle1 = (float)(i + 1) / (float)segmentcount * XM_2PI;
		wiRenderer::RenderableLine2D line;
		line.start.x = pos.x + sinf(angle0) * radius;
		line.start.y = pos.y + cosf(angle0) * radius;
		line.end.x = pos.x + sinf(angle1) * radius;
		line.end.y = pos.y + cosf(angle1) * radius;
		line.color_end = line.color_start = XMFLOAT4(0, 0, 0, 0.8f);
		wiRenderer::DrawLine(line);
	}

	for (int i = 0; i < segmentcount; i += 2)
	{
		const float angle0 = rot + (float)i / (float)segmentcount * XM_2PI;
		const float angle1 = rot + (float)(i + 1) / (float)segmentcount * XM_2PI;
		wiRenderer::RenderableLine2D line;
		line.start.x = pos.x + sinf(angle0) * radius;
		line.start.y = pos.y + cosf(angle0) * radius;
		line.end.x = pos.x + sinf(angle1) * radius;
		line.end.y = pos.y + cosf(angle1) * radius;
		wiRenderer::DrawLine(line);
	}

}

PaintToolWindow::MODE PaintToolWindow::GetMode() const
{
	return (MODE)modeComboBox->GetSelected();
}
void PaintToolWindow::SetEntity(wiECS::Entity value, int subsetindex)
{
	entity = value;
	subset = subsetindex;
}

void PaintToolWindow::RecordHistory(bool start)
{
	if (start)
	{
		if (history_needs_recording_start)
			return;
		history_needs_recording_start = true;
		history_needs_recording_end = true;

		// Start history recording (undo)
		currentHistory = &editor->AdvanceHistory();
		wiArchive& archive = *currentHistory;
		archive << EditorComponent::HISTORYOP_PAINTTOOL;
		archive << (uint32_t)GetMode();
		archive << entity;
	}
	else
	{
		if (!history_needs_recording_end || wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
			return;
		history_needs_recording_end = false;
		history_needs_recording_start = false;

		// End history recording (redo)
	}

	wiArchive& archive = *currentHistory;
	Scene& scene = wiScene::GetScene();

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

		const Texture* texture = GetEditTextureSlot(*material);

		archive << textureDestComboBox->GetSelected();

		std::vector<uint8_t> data;
		wiHelper::saveTextureToMemory(*texture, data);
		archive << data;
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
		wiHairParticle* hair = scene.hairs.GetComponent(entity);
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
void PaintToolWindow::ConsumeHistoryOperation(wiArchive& archive, bool undo)
{
	MODE historymode;
	archive >> (uint32_t&)historymode;
	archive >> entity;

	modeComboBox->SetSelected(historymode);

	Scene& scene = wiScene::GetScene();

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

		int undo_dest;
		archive >> undo_dest;
		std::vector<uint8_t> undo_data;
		archive >> undo_data;
		int redo_dest;
		archive >> redo_dest;
		std::vector<uint8_t> redo_data;
		archive >> redo_data;

		if (undo)
		{
			textureDestComboBox->SetSelected(undo_dest);
		}
		else
		{
			textureDestComboBox->SetSelected(redo_dest);
		}

		GraphicsDevice* device = wiRenderer::GetDevice();

		Texture* texture = GetEditTextureSlot(*material);
		const TextureDesc& desc = texture->GetDesc();

		std::vector<SubresourceData> InitData(desc.MipLevels);
		uint32_t mipwidth = desc.Width;
		for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
		{
			// attention! we don't fill the mips here correctly, just always point to the mip0 data by default. Mip levels will be created using compute shader when needed!
			if (undo)
			{
				InitData[mip].pSysMem = undo_data.data();
			}
			else
			{
				InitData[mip].pSysMem = redo_data.data();
			}
			InitData[mip].SysMemPitch = static_cast<uint32_t>(mipwidth * 4);
			mipwidth = std::max(1u, mipwidth / 2);
		}
		device->CreateTexture(&desc, InitData.data(), texture);

		for (uint32_t i = 0; i < texture->GetDesc().MipLevels; ++i)
		{
			int subresource_index;
			subresource_index = device->CreateSubresource(texture, SRV, 0, 1, i, 1);
			assert(subresource_index == i);
			subresource_index = device->CreateSubresource(texture, UAV, 0, 1, i, 1);
			assert(subresource_index == i);
		}

		wiRenderer::AddDeferredMIPGen(texture);

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
		MeshComponent redo_mesh;
		archive >> redo_mesh.vertex_positions;

		if (undo)
		{
			mesh->vertex_positions = undo_mesh.vertex_positions;
		}
		else
		{
			mesh->vertex_positions = redo_mesh.vertex_positions;
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
		wiHairParticle* hair = scene.hairs.GetComponent(entity);
		if (hair == nullptr)
			break;

		wiHairParticle undo_hair;
		archive >> undo_hair.vertex_lengths;
		wiHairParticle redo_hair;
		archive >> redo_hair.vertex_lengths;

		if (undo)
		{
			hair->vertex_lengths = undo_hair.vertex_lengths;
		}
		else
		{
			hair->vertex_lengths = redo_hair.vertex_lengths;
		}

		hair->_flags |= wiHairParticle::REBUILD_BUFFERS;
	}
	break;
	default:
		assert(0);
		break;
	}
}
Texture* PaintToolWindow::GetEditTextureSlot(const MaterialComponent& material, int* uvset)
{
	// Think about refactoring this and remove const_casts!
	int sel = textureDestComboBox->GetSelected();
	switch (sel)
	{
	default:
	case 0:
		if (uvset) *uvset = material.uvset_baseColorMap;
		return const_cast<Texture*>(material.GetBaseColorMap());
	case 1:
		if (uvset) *uvset = material.uvset_normalMap;
		return const_cast<Texture*>(material.GetNormalMap());
	case 2:
		if (uvset) *uvset = material.uvset_surfaceMap;
		return const_cast<Texture*>(material.GetSurfaceMap());
	case 3:
		if (uvset) *uvset = material.uvset_displacementMap;
		return const_cast<Texture*>(material.GetDisplacementMap());
	case 4:
		if (uvset) *uvset = material.uvset_emissiveMap;
		return const_cast<Texture*>(material.GetEmissiveMap());
	case 5:
		if (uvset) *uvset = material.uvset_occlusionMap;
		return const_cast<Texture*>(material.GetOcclusionMap());
	}
}
