#include "stdafx.h"
#include "MeshWindow.h"
#include "Editor.h"

#include "Utility/stb_image.h"

#include <sstream>

using namespace std;
using namespace wiECS;
using namespace wiScene;

MeshWindow::MeshWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");


	meshWindow = new wiWindow(GUI, "Mesh Window");
	meshWindow->SetSize(XMFLOAT2(600, 620));
	GUI->AddWidget(meshWindow);

	float x = 150;
	float y = 0;
	float step = 30;
	float hei = 25;

	meshInfoLabel = new wiLabel("Mesh Info");
	meshInfoLabel->SetPos(XMFLOAT2(x - 50, y += step));
	meshInfoLabel->SetSize(XMFLOAT2(450, 150));
	meshWindow->AddWidget(meshInfoLabel);

	y += 160;

	doubleSidedCheckBox = new wiCheckBox("Double Sided: ");
	doubleSidedCheckBox->SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox->SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetDoubleSided(args.bValue);
		}
	});
	meshWindow->AddWidget(doubleSidedCheckBox);

	softbodyCheckBox = new wiCheckBox("Soft body: ");
	softbodyCheckBox->SetTooltip("Enable soft body simulation. Tip: Use the Paint Tool to control vertex pinning.");
	softbodyCheckBox->SetPos(XMFLOAT2(x, y += step));
	softbodyCheckBox->OnClick([&](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				SoftBodyPhysicsComponent& softbody = scene.softbodies.Create(entity);
				softbody.friction = frictionSlider->GetValue();
				softbody.mass = massSlider->GetValue();
			}
		}
		else
		{
			if (physicscomponent != nullptr)
			{
				scene.softbodies.Remove(entity);
			}
		}

	});
	meshWindow->AddWidget(softbodyCheckBox);

	massSlider = new wiSlider(0, 10, 0, 100000, "Mass: ");
	massSlider->SetTooltip("Set the mass amount for the physics engine.");
	massSlider->SetSize(XMFLOAT2(100, hei));
	massSlider->SetPos(XMFLOAT2(x, y += step));
	massSlider->OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
	});
	meshWindow->AddWidget(massSlider);

	frictionSlider = new wiSlider(0, 2, 0, 100000, "Friction: ");
	frictionSlider->SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider->SetSize(XMFLOAT2(100, hei));
	frictionSlider->SetPos(XMFLOAT2(x, y += step));
	frictionSlider->OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
	});
	meshWindow->AddWidget(frictionSlider);

	impostorCreateButton = new wiButton("Create Impostor");
	impostorCreateButton->SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton->SetSize(XMFLOAT2(240, hei));
	impostorCreateButton->SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			Scene& scene = wiScene::GetScene();
			scene.impostors.Create(entity).swapInDistance = impostorDistanceSlider->GetValue();
		}
	});
	meshWindow->AddWidget(impostorCreateButton);

	impostorDistanceSlider = new wiSlider(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider->SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider->SetSize(XMFLOAT2(100, hei));
	impostorDistanceSlider->SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider->OnSlide([&](wiEventArgs args) {
		ImpostorComponent* impostor = wiScene::GetScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	meshWindow->AddWidget(impostorDistanceSlider);

	tessellationFactorSlider = new wiSlider(0, 16, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider->SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider->SetSize(XMFLOAT2(100, hei));
	tessellationFactorSlider->SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider->OnSlide([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	meshWindow->AddWidget(tessellationFactorSlider);

	flipCullingButton = new wiButton("Flip Culling");
	flipCullingButton->SetTooltip("Flip faces to reverse triangle culling order.");
	flipCullingButton->SetSize(XMFLOAT2(240, hei));
	flipCullingButton->SetPos(XMFLOAT2(x - 50, y += step));
	flipCullingButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipCulling();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(flipCullingButton);

	flipNormalsButton = new wiButton("Flip Normals");
	flipNormalsButton->SetTooltip("Flip surface normals.");
	flipNormalsButton->SetSize(XMFLOAT2(240, hei));
	flipNormalsButton->SetPos(XMFLOAT2(x - 50, y += step));
	flipNormalsButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipNormals();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(flipNormalsButton);

	computeNormalsSmoothButton = new wiButton("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex.");
	computeNormalsSmoothButton->SetSize(XMFLOAT2(240, hei));
	computeNormalsSmoothButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(true);
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(computeNormalsSmoothButton);

	computeNormalsHardButton = new wiButton("Compute Normals [HARD]");
	computeNormalsHardButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face.");
	computeNormalsHardButton->SetSize(XMFLOAT2(240, hei));
	computeNormalsHardButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(false);
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(computeNormalsHardButton);

	recenterButton = new wiButton("Recenter");
	recenterButton->SetTooltip("Recenter mesh to AABB center.");
	recenterButton->SetSize(XMFLOAT2(240, hei));
	recenterButton->SetPos(XMFLOAT2(x - 50, y += step));
	recenterButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->Recenter();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(recenterButton);

	recenterToBottomButton = new wiButton("RecenterToBottom");
	recenterToBottomButton->SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton->SetSize(XMFLOAT2(240, hei));
	recenterToBottomButton->SetPos(XMFLOAT2(x - 50, y += step));
	recenterToBottomButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->RecenterToBottom();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(recenterToBottomButton);

	x = 150;
	y = 160;

	terrainCheckBox = new wiCheckBox("Terrain: ");
	terrainCheckBox->SetTooltip("If enabled, the mesh will use multiple materials and blend between them based on vertex colors.");
	terrainCheckBox->SetPos(XMFLOAT2(x, y += step));
	terrainCheckBox->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetTerrain(args.bValue);
			if (args.bValue && mesh->vertex_colors.empty())
			{
				mesh->vertex_colors.resize(mesh->vertex_positions.size());
				std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), wiColor::Red().rgba); // fill red (meaning only blend base material)
				mesh->CreateRenderData();

				for (auto& subset : mesh->subsets)
				{
					MaterialComponent* material = wiScene::GetScene().materials.GetComponent(subset.materialID);
					if (material != nullptr)
					{
						material->SetUseVertexColors(true);
					}
				}
			}
			SetEntity(entity); // refresh information label
		}
	});
	meshWindow->AddWidget(terrainCheckBox);

	terrainMat1Combo = new wiComboBox("Terrain Material 1: ");
	terrainMat1Combo->SetSize(XMFLOAT2(200, 20));
	terrainMat1Combo->SetPos(XMFLOAT2(x + 180, y));
	terrainMat1Combo->SetEnabled(false);
	terrainMat1Combo->OnSelect([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material1 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiScene::GetScene();
				mesh->terrain_material1 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat1Combo->SetTooltip("Choose a sub terrain blend material.");
	meshWindow->AddWidget(terrainMat1Combo);

	terrainMat2Combo = new wiComboBox("Terrain Material 2: ");
	terrainMat2Combo->SetSize(XMFLOAT2(200, 20));
	terrainMat2Combo->SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat2Combo->SetEnabled(false);
	terrainMat2Combo->OnSelect([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material2 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiScene::GetScene();
				mesh->terrain_material2 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat2Combo->SetTooltip("Choose a sub terrain blend material.");
	meshWindow->AddWidget(terrainMat2Combo);

	terrainMat3Combo = new wiComboBox("Terrain Material 3: ");
	terrainMat3Combo->SetSize(XMFLOAT2(200, 20));
	terrainMat3Combo->SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat3Combo->SetEnabled(false);
	terrainMat3Combo->OnSelect([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material3 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wiScene::GetScene();
				mesh->terrain_material3 = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	terrainMat3Combo->SetTooltip("Choose a sub terrain blend material.");
	meshWindow->AddWidget(terrainMat3Combo);

	terrainFromHeightMapButton = new wiButton("Create Terrain From Heightmap");
	terrainFromHeightMapButton->SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions");
	terrainFromHeightMapButton->SetSize(XMFLOAT2(200, 20));
	terrainFromHeightMapButton->SetPos(XMFLOAT2(x + 180, y += step));
	terrainFromHeightMapButton->OnClick([=](wiEventArgs args) {

		wiHelper::FileDialogParams params;
		wiHelper::FileDialogResult result;
		params.type = wiHelper::FileDialogParams::OPEN;
		params.description = "Texture";
		params.extensions.push_back("dds");
		params.extensions.push_back("png");
		params.extensions.push_back("jpg");
		params.extensions.push_back("tga");
		wiHelper::FileDialog(params, result);

		if (result.ok) {
			string fileName = result.filenames.front();

			Scene& scene = wiScene::GetScene();
			Entity entity = scene.Entity_CreateObject("editorTerrain");
			ObjectComponent& object = *scene.objects.GetComponent(entity);
			object.meshID = scene.Entity_CreateMesh("terrainMesh");
			MeshComponent& mesh = *scene.meshes.GetComponent(object.meshID);

			const int channelCount = 4;
			int width, height, bpp;
			unsigned char* rgb = stbi_load(fileName.c_str(), &width, &height, &bpp, channelCount);

			mesh.vertex_positions.resize(width * height);
			mesh.vertex_normals.resize(width * height);
			mesh.vertex_colors.resize(width * height);
			for (int i = 0; i < width; ++i)
			{
				for (int j = 0; j < height; ++j)
				{
					size_t index = size_t(i + j * width);
					mesh.vertex_positions[index] = XMFLOAT3((float)i - (float)width * 0.5f, (float)rgb[index * channelCount] - 127.0f, (float)j - (float)height * 0.5f);
					mesh.vertex_colors[index] = wiColor::Red().rgba;
				}
			}
			mesh.indices.resize((width - 1) * (height - 1) * 6);
			size_t counter = 0;
			for (int x = 0; x < width - 1; x++)
			{
				for (int y = 0; y < height - 1; y++)
				{
					int lowerLeft = x + y * width;
					int lowerRight = (x + 1) + y * width;
					int topLeft = x + (y + 1) * width;
					int topRight = (x + 1) + (y + 1) * width;

					mesh.indices[counter++] = topLeft;
					mesh.indices[counter++] = lowerLeft;
					mesh.indices[counter++] = lowerRight;

					mesh.indices[counter++] = topLeft;
					mesh.indices[counter++] = lowerRight;
					mesh.indices[counter++] = topRight;
				}
			}
			for (size_t i = 0; i < mesh.indices.size() / 3; ++i)
			{
				uint32_t index1 = mesh.indices[i * 3];
				uint32_t index2 = mesh.indices[i * 3 + 1];
				uint32_t index3 = mesh.indices[i * 3 + 2];

				XMVECTOR side1 = XMLoadFloat3(&mesh.vertex_positions[index1]) - XMLoadFloat3(&mesh.vertex_positions[index3]);
				XMVECTOR side2 = XMLoadFloat3(&mesh.vertex_positions[index1]) - XMLoadFloat3(&mesh.vertex_positions[index2]);
				XMVECTOR N = XMVector3Normalize(XMVector3Cross(side1, side2));
				XMFLOAT3 normal;
				XMStoreFloat3(&normal, N);

				mesh.vertex_normals[index1].x += normal.x;
				mesh.vertex_normals[index1].y += normal.y;
				mesh.vertex_normals[index1].z += normal.z;

				mesh.vertex_normals[index2].x += normal.x;
				mesh.vertex_normals[index2].y += normal.y;
				mesh.vertex_normals[index2].z += normal.z;

				mesh.vertex_normals[index3].x += normal.x;
				mesh.vertex_normals[index3].y += normal.y;
				mesh.vertex_normals[index3].z += normal.z;
			}
			for (auto& normal : mesh.vertex_normals)
			{
				XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));
			}
			mesh.subsets.emplace_back();
			mesh.subsets.back().materialID = scene.Entity_CreateMaterial("terrainMaterial");
			mesh.subsets.back().indexOffset = 0;
			mesh.subsets.back().indexCount = (uint32_t)mesh.indices.size();

			mesh.CreateRenderData();

			editor->ClearSelected();
			wiScene::PickResult pick;
			pick.entity = entity;
			pick.subsetIndex = 0;
			editor->AddSelected(pick);
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(terrainFromHeightMapButton);



	meshWindow->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 1000, 80, 0));
	meshWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


MeshWindow::~MeshWindow()
{
	meshWindow->RemoveWidgets(true);
	GUI->RemoveWidget(meshWindow);
	delete meshWindow;
}

void MeshWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene&scene = wiScene::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		stringstream ss("");
		ss << "Mesh name: " << name.name << endl;
		ss << "Vertex count: " << mesh->vertex_positions.size() << endl;
		ss << "Index count: " << mesh->indices.size() << endl;
		ss << "Subset count: " << mesh->subsets.size() << endl;
		ss << endl << "Vertex buffers: ";
		if (mesh->vertexBuffer_POS.IsValid()) ss << "position; ";
		if (mesh->vertexBuffer_UV0.IsValid()) ss << "uvset_0; ";
		if (mesh->vertexBuffer_UV1.IsValid()) ss << "uvset_1; ";
		if (mesh->vertexBuffer_ATL.IsValid()) ss << "atlas; ";
		if (mesh->vertexBuffer_COL.IsValid()) ss << "color; ";
		if (mesh->vertexBuffer_PRE.IsValid()) ss << "prevPos; ";
		if (mesh->vertexBuffer_BON.IsValid()) ss << "bone; ";
		if (mesh->streamoutBuffer_POS.IsValid()) ss << "streamout; ";
		if (mesh->IsTerrain()) ss << endl << endl << "Terrain will use 4 blend materials and blend by vertex colors, the default one is always the subset material, the other 3 are selectable below.";
		meshInfoLabel->SetText(ss.str());

		terrainCheckBox->SetCheck(mesh->IsTerrain());

		terrainMat1Combo->ClearItems();
		terrainMat1Combo->AddItem("OFF (Use subset)");
		terrainMat2Combo->ClearItems();
		terrainMat2Combo->AddItem("OFF (Use subset)");
		terrainMat3Combo->ClearItems();
		terrainMat3Combo->AddItem("OFF (Use subset)");
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			Entity entity = scene.materials.GetEntity(i);
			const NameComponent& name = *scene.names.GetComponent(entity);
			terrainMat1Combo->AddItem(name.name);
			terrainMat2Combo->AddItem(name.name);
			terrainMat3Combo->AddItem(name.name);

			if (mesh->terrain_material1 == entity)
			{
				terrainMat1Combo->SetSelected((int)i + 1);
			}
			if (mesh->terrain_material2 == entity)
			{
				terrainMat2Combo->SetSelected((int)i + 1);
			}
			if (mesh->terrain_material3 == entity)
			{
				terrainMat3Combo->SetSelected((int)i + 1);
			}
		}

		doubleSidedCheckBox->SetCheck(mesh->IsDoubleSided());

		const ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostorDistanceSlider->SetValue(impostor->swapInDistance);
		}
		tessellationFactorSlider->SetValue(mesh->GetTessellationFactor());

		softbodyCheckBox->SetCheck(false);

		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			softbodyCheckBox->SetCheck(true);
			massSlider->SetValue(physicscomponent->mass);
			frictionSlider->SetValue(physicscomponent->friction);
		}
		meshWindow->SetEnabled(true);
	}
	else
	{
		meshInfoLabel->SetText("Select a mesh...");
		meshWindow->SetEnabled(false);
	}

	terrainFromHeightMapButton->SetEnabled(true);
}
