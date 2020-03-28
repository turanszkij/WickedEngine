#include "stdafx.h"
#include "Editor.h"
#include "PaintToolWindow.h"

using namespace wiECS;
using namespace wiScene;
using namespace wiGraphics;

PaintToolWindow::PaintToolWindow(EditorComponent* editor) : editor(editor)
{
	window = new wiWindow(&editor->GetGUI(), "Paint Tool Window");
	window->SetSize(XMFLOAT2(400, 600));
	editor->GetGUI().AddWidget(window);

	float x = 100;
	float y = 10;
	float step = 30;

	modeComboBox = new wiComboBox("Mode: ");
	modeComboBox->SetTooltip("Choose paint tool mode");
	modeComboBox->SetPos(XMFLOAT2(x, y += step));
	modeComboBox->SetSize(XMFLOAT2(200, 28));
	modeComboBox->AddItem("Disabled");
	modeComboBox->AddItem("Vertexcolor");
	modeComboBox->AddItem("Sculpting - Add");
	modeComboBox->AddItem("Sculpting - Subtract");
	modeComboBox->AddItem("Softbody - Pinning");
	modeComboBox->AddItem("Softbody - Physics");
	modeComboBox->AddItem("Hairparticle - Add Triangle");
	modeComboBox->AddItem("Hairparticle - Remove Triangle");
	modeComboBox->AddItem("Hairparticle - Length (Alpha)");
	modeComboBox->SetMaxVisibleItemCount(16);
	modeComboBox->SetSelected(0);
	modeComboBox->OnSelect([&](wiEventArgs args) {
		switch (args.iValue)
		{
		case MODE_DISABLED:
			infoLabel->SetText("Paint Tool is disabled.");
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
		default:
			assert(0);
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
	if (GetMode() == MODE_DISABLED || entity == INVALID_ENTITY || wiBackLog::isActive())
		return;

	const float radius = radiusSlider->GetValue();
	const int segmentcount = 36;
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

	Scene& scene = wiScene::GetScene();

	switch (historymode)
	{
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
