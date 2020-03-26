#include "stdafx.h"
#include "PaintToolWindow.h"

using namespace wiECS;
using namespace wiScene;
using namespace wiGraphics;

PaintToolWindow::PaintToolWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Paint Tool Window");
	window->SetSize(XMFLOAT2(400, 500));
	GUI->AddWidget(window);

	float x = 100;
	float y = 10;
	float step = 30;

	modeComboBox = new wiComboBox("Mode: ");
	modeComboBox->SetTooltip("Choose paint tool mode");
	modeComboBox->SetPos(XMFLOAT2(x, y += step));
	modeComboBox->SetSize(XMFLOAT2(200, 28));
	modeComboBox->AddItem("Disabled");
	modeComboBox->AddItem("Vertexcolor");
	modeComboBox->AddItem("Softbody - Pinning");
	modeComboBox->AddItem("Softbody - Physics");
	modeComboBox->AddItem("Hairparticle - Add Triangle");
	modeComboBox->AddItem("Hairparticle - Remove Triangle");
	modeComboBox->AddItem("Hairparticle - Length (Alpha)");
	modeComboBox->SetSelected(0);
	window->AddWidget(modeComboBox);

	y += step;

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

	backfaceCheckBox = new wiCheckBox("Backfaces: ");
	backfaceCheckBox->SetTooltip("Set whether to paint on backfaces of geometry or not");
	backfaceCheckBox->SetPos(XMFLOAT2(x, y += step));
	window->AddWidget(backfaceCheckBox);

	colorPicker = new wiColorPicker(GUI, "Color", false);
	colorPicker->SetPos(XMFLOAT2(10, y += step));
	window->AddWidget(colorPicker);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 550, 50, 0));
	window->SetVisible(false);
}
PaintToolWindow::~PaintToolWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void PaintToolWindow::Update(float dt)
{
	rot -= dt;
	// by default, paint tool is on center of screen, this makes it easy to tweak radius with GUI:
	pos.x = wiRenderer::GetDevice()->GetScreenWidth() * 0.5f;
	pos.y = wiRenderer::GetDevice()->GetScreenHeight() * 0.5f;
	if (GUI->HasFocus() || wiBackLog::isActive() || entity == INVALID_ENTITY)
		return;

	auto pointer = wiInput::GetPointer();
	pos.x = pointer.x;
	pos.y = pointer.y;

	const bool pointer_moved = wiMath::Distance(pos, posPrev) > 1.0f;
	const bool painting = pointer_moved && wiInput::Down(wiInput::MOUSE_BUTTON_LEFT);

	if (wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
	{
		posPrev = pos;
	}
	else
	{
		posPrev = XMFLOAT2(0, 0);
	}

	const MODE mode = GetMode();
	const float radius = radiusSlider->GetValue();
	const float amount = amountSlider->GetValue();
	const float falloff = falloffSlider->GetValue();
	const wiColor color = colorPicker->GetPickColor();
	const XMFLOAT4 color_float = color.toFloat4();
	const bool backfaces = backfaceCheckBox->GetCheck();

	Scene& scene = wiScene::GetScene();
	const CameraComponent& camera = wiRenderer::GetCamera();
	const XMVECTOR C = XMLoadFloat2(&pos);
	const XMMATRIX VP = camera.GetViewProjection();
	const XMVECTOR MUL = XMVectorSet(0.5f, -0.5f, 1, 1);
	const XMVECTOR ADD = XMVectorSet(0.5f, 0.5f, 0, 0);
	const XMVECTOR SCREEN = XMVectorSet((float)wiRenderer::GetDevice()->GetScreenWidth(), (float)wiRenderer::GetDevice()->GetScreenHeight(), 1, 1);

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

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (transform == nullptr)
			break;

		const XMMATRIX W = XMLoadFloat4x4(&transform->world);

		bool rebuild = false;
		for (size_t j = 0; j < mesh->indices.size(); j += 3)
		{
			const uint32_t triangle[] = {
				mesh->indices[j + 0],
				mesh->indices[j + 1],
				mesh->indices[j + 2],
			};

			if (painting)
			{
				XMVECTOR P[arraysize(triangle)];
				for (int k = 0; k < arraysize(triangle); ++k)
				{
					const XMFLOAT3& pos = mesh->vertex_positions[triangle[k]];
					P[k] = XMLoadFloat3(&pos);
					P[k] = XMVector3Transform(P[k], W);
					P[k] = XMVector3TransformCoord(P[k], VP);
					P[k] = P[k] * MUL + ADD;
					P[k] = P[k] * SCREEN;
				}

				bool culled = false;
				if (!backfaces)
				{
					const XMVECTOR N = XMVector3Normalize(XMVector3Cross(P[1] - P[0], P[2] - P[1]));
					culled = XMVectorGetZ(N) > 0;
				}

				if (!culled)
				{
					for (int k = 0; k < arraysize(triangle); ++k)
					{
						const float z = XMVectorGetZ(P[k]);
						const float dist = XMVectorGetX(XMVector2Length(C - P[k]));
						if (z >= 0 && z <= 1 && dist <= radius)
						{
							if (mesh->vertex_colors.empty())
							{
								mesh->vertex_colors.resize(mesh->vertex_positions.size());
								std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), 0xFFFFFFFF); // fill white
							}
							wiColor vcol = mesh->vertex_colors[triangle[k]];
							const float affection = amount * std::powf(1 - (dist / radius), falloff);
							vcol = wiColor::lerp(vcol, color, affection);
							mesh->vertex_colors[triangle[k]] = vcol.rgba;
							rebuild = true;
						}
					}
				}
			}

			wiRenderer::RenderableTriangle tri;
			XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[0]]), W));
			XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[1]]), W));
			XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[2]]), W));
			tri.colorA.w = 0.8f;
			tri.colorB.w = 0.8f;
			tri.colorC.w = 0.8f;
			wiRenderer::DrawTriangle(tri, true);
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
			wiRenderer::DrawTriangle(tri, true);
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

		const TransformComponent* transform = scene.transforms.GetComponent(entity);
		if (transform == nullptr)
			break;

		const XMMATRIX W = XMLoadFloat4x4(&transform->world);

		for (size_t j = 0; j < mesh->indices.size(); j += 3)
		{
			const uint32_t triangle[] = {
				mesh->indices[j + 0],
				mesh->indices[j + 1],
				mesh->indices[j + 2],
			};

			if (painting)
			{
				XMVECTOR P[arraysize(triangle)];
				for (int k = 0; k < arraysize(triangle); ++k)
				{
					const XMFLOAT3& pos = mesh->vertex_positions[triangle[k]];
					P[k] = XMLoadFloat3(&pos);
					P[k] = XMVector3Transform(P[k], W);
					P[k] = XMVector3TransformCoord(P[k], VP);
					P[k] = P[k] * MUL + ADD;
					P[k] = P[k] * SCREEN;
				}

				bool culled = false;
				if (!backfaces)
				{
					const XMVECTOR N = XMVector3Normalize(XMVector3Cross(P[1] - P[0], P[2] - P[1]));
					culled = XMVectorGetZ(N) > 0;
				}

				if (!culled)
				{
					for (int k = 0; k < arraysize(triangle); ++k)
					{
						const float z = XMVectorGetZ(P[k]);
						const float dist = XMVectorGetX(XMVector2Length(C - P[k]));
						if (z >= 0 && z <= 1 && dist <= radius)
						{
							switch (mode)
							{
							case MODE_HAIRPARTICLE_ADD_TRIANGLE:
								hair->vertex_lengths[triangle[k]] = 1.0f;
								break;
							case MODE_HAIRPARTICLE_REMOVE_TRIANGLE:
								hair->vertex_lengths[triangle[k]] = 0;
								break;
							case MODE_HAIRPARTICLE_LENGTH:
								const float affection = amount * std::powf(1 - (dist / radius), falloff);
								hair->vertex_lengths[triangle[k]] = wiMath::Lerp(hair->vertex_lengths[triangle[k]], color_float.w, affection);
								// don't let it "remove" the vertex by keeping its length above zero:
								//	(because if removed, distribution also changes which might be distracting)
								hair->vertex_lengths[triangle[k]] = wiMath::Clamp(hair->vertex_lengths[triangle[k]], 1.0f / 255.0f, 1.0f);
								break;
							}
							hair->_flags |= wiHairParticle::REBUILD_BUFFERS;
						}
					}
				}
			}

			wiRenderer::RenderableTriangle tri;
			XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[0]]), W));
			XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[1]]), W));
			XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[2]]), W));
			wiRenderer::DrawTriangle(tri, true);
		}

		for (size_t j = 0; j < hair->indices.size(); j += 3)
		{
			const uint32_t triangle[] = {
				hair->indices[j + 0],
				hair->indices[j + 1],
				hair->indices[j + 2],
			};

			wiRenderer::RenderableTriangle tri;
			XMStoreFloat3(&tri.positionA, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[0]]), W));
			XMStoreFloat3(&tri.positionB, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[1]]), W));
			XMStoreFloat3(&tri.positionC, XMVector3Transform(XMLoadFloat3(&mesh->vertex_positions[triangle[2]]), W));
			tri.colorA = tri.colorB = tri.colorC = XMFLOAT4(1, 0, 1, 0.9f);
			wiRenderer::DrawTriangle(tri, false);
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
void PaintToolWindow::SetEntity(wiECS::Entity value)
{
	entity = value;
}
