#include "stdafx.h"
#include "MeshWindow.h"
#include "Editor.h"

#include "Utility/stb_image.h"

#include <sstream>

using namespace std;
using namespace wiECS;
using namespace wiScene;

struct TerraGen : public wiWindow
{
	wiSlider dimXSlider;
	wiSlider dimYSlider;
	wiSlider dimZSlider;
	wiButton heightmapButton;

	// heightmap texture:
	unsigned char* rgb = nullptr;
	const int channelCount = 4;
	int width = 0, height = 0;

	TerraGen()
	{
		wiWindow::Create("TerraGen");
		SetSize(XMFLOAT2(260, 130));

		float xx = 20;
		float yy = 0;
		float stepstep = 25;
		float heihei = 20;

		dimXSlider.Create(16, 1024, 128, 1024 - 16, "X: ");
		dimXSlider.SetTooltip("Terrain mesh grid resolution on X axis");
		dimXSlider.SetSize(XMFLOAT2(200, heihei));
		dimXSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&dimXSlider);

		dimYSlider.Create(0, 1, 0.5f, 10000, "Y: ");
		dimYSlider.SetTooltip("Terrain mesh grid heightmap scale on Y axis");
		dimYSlider.SetSize(XMFLOAT2(200, heihei));
		dimYSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&dimYSlider);

		dimZSlider.Create(16, 1024, 128, 1024 - 16, "Z: ");
		dimZSlider.SetTooltip("Terrain mesh grid resolution on Z axis");
		dimZSlider.SetSize(XMFLOAT2(200, heihei));
		dimZSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&dimZSlider);


		heightmapButton.Create("Load Heightmap...");
		heightmapButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions");
		heightmapButton.SetSize(XMFLOAT2(200, heihei));
		heightmapButton.SetPos(XMFLOAT2(xx, yy += stepstep));

		AddWidget(&heightmapButton);
	}
	~TerraGen()
	{
		Cleanup();
	}

	void Cleanup()
	{
		if (rgb != nullptr)
		{
			stbi_image_free(rgb);
			rgb = nullptr;
		}
	}
} terragen;

void MeshWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Mesh Window");
	SetSize(XMFLOAT2(580, 500));

	float x = 150;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	meshInfoLabel.Create("Mesh Info");
	meshInfoLabel.SetPos(XMFLOAT2(x - 50, y += step));
	meshInfoLabel.SetSize(XMFLOAT2(450, 180));
	AddWidget(&meshInfoLabel);

	y += 190;

	doubleSidedCheckBox.Create("Double Sided: ");
	doubleSidedCheckBox.SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetDoubleSided(args.bValue);
		}
	});
	AddWidget(&doubleSidedCheckBox);

	softbodyCheckBox.Create("Soft body: ");
	softbodyCheckBox.SetTooltip("Enable soft body simulation. Tip: Use the Paint Tool to control vertex pinning.");
	softbodyCheckBox.SetSize(XMFLOAT2(hei, hei));
	softbodyCheckBox.SetPos(XMFLOAT2(x, y += step));
	softbodyCheckBox.OnClick([&](wiEventArgs args) {

		Scene& scene = wiScene::GetScene();
		SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				SoftBodyPhysicsComponent& softbody = scene.softbodies.Create(entity);
				softbody.friction = frictionSlider.GetValue();
				softbody.mass = massSlider.GetValue();
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
	AddWidget(&softbodyCheckBox);

	massSlider.Create(0, 10, 0, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.SetSize(XMFLOAT2(100, hei));
	massSlider.SetPos(XMFLOAT2(x, y += step));
	massSlider.OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
	});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 2, 0, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.SetSize(XMFLOAT2(100, hei));
	frictionSlider.SetPos(XMFLOAT2(x, y += step));
	frictionSlider.OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
	});
	AddWidget(&frictionSlider);

	impostorCreateButton.Create("Create Impostor");
	impostorCreateButton.SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton.SetSize(XMFLOAT2(240, hei));
	impostorCreateButton.SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			Scene& scene = wiScene::GetScene();
			scene.impostors.Create(entity).swapInDistance = impostorDistanceSlider.GetValue();
		}
	});
	AddWidget(&impostorCreateButton);

	impostorDistanceSlider.Create(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider.SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider.SetSize(XMFLOAT2(100, hei));
	impostorDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider.OnSlide([&](wiEventArgs args) {
		ImpostorComponent* impostor = wiScene::GetScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	AddWidget(&impostorDistanceSlider);

	tessellationFactorSlider.Create(0, 16, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider.SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider.SetSize(XMFLOAT2(100, hei));
	tessellationFactorSlider.SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider.OnSlide([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	AddWidget(&tessellationFactorSlider);

	flipCullingButton.Create("Flip Culling");
	flipCullingButton.SetTooltip("Flip faces to reverse triangle culling order.");
	flipCullingButton.SetSize(XMFLOAT2(240, hei));
	flipCullingButton.SetPos(XMFLOAT2(x - 50, y += step));
	flipCullingButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipCulling();
			SetEntity(entity);
		}
	});
	AddWidget(&flipCullingButton);

	flipNormalsButton.Create("Flip Normals");
	flipNormalsButton.SetTooltip("Flip surface normals.");
	flipNormalsButton.SetSize(XMFLOAT2(240, hei));
	flipNormalsButton.SetPos(XMFLOAT2(x - 50, y += step));
	flipNormalsButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipNormals();
			SetEntity(entity);
		}
	});
	AddWidget(&flipNormalsButton);

	computeNormalsSmoothButton.Create("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex. This can reduce vertex count, but is slow.");
	computeNormalsSmoothButton.SetSize(XMFLOAT2(240, hei));
	computeNormalsSmoothButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH);
			SetEntity(entity);
		}
	});
	AddWidget(&computeNormalsSmoothButton);

	computeNormalsHardButton.Create("Compute Normals [HARD]");
	computeNormalsHardButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face. This can increase vertex count.");
	computeNormalsHardButton.SetSize(XMFLOAT2(240, hei));
	computeNormalsHardButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_HARD);
			SetEntity(entity);
		}
	});
	AddWidget(&computeNormalsHardButton);

	recenterButton.Create("Recenter");
	recenterButton.SetTooltip("Recenter mesh to AABB center.");
	recenterButton.SetSize(XMFLOAT2(240, hei));
	recenterButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->Recenter();
			SetEntity(entity);
		}
	});
	AddWidget(&recenterButton);

	recenterToBottomButton.Create("RecenterToBottom");
	recenterToBottomButton.SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton.SetSize(XMFLOAT2(240, hei));
	recenterToBottomButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterToBottomButton.OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiScene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->RecenterToBottom();
			SetEntity(entity);
		}
	});
	AddWidget(&recenterToBottomButton);

	x = 150;
	y = 190;

	terrainCheckBox.Create("Terrain: ");
	terrainCheckBox.SetTooltip("If enabled, the mesh will use multiple materials and blend between them based on vertex colors.");
	terrainCheckBox.SetSize(XMFLOAT2(hei, hei));
	terrainCheckBox.SetPos(XMFLOAT2(x, y += step));
	terrainCheckBox.OnClick([&](wiEventArgs args) {
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
	AddWidget(&terrainCheckBox);

	terrainMat1Combo.Create("Terrain Material 1: ");
	terrainMat1Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat1Combo.SetPos(XMFLOAT2(x + 180, y));
	terrainMat1Combo.SetEnabled(false);
	terrainMat1Combo.OnSelect([&](wiEventArgs args) {
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
	terrainMat1Combo.SetTooltip("Choose a sub terrain blend material. (GREEN vertex color mask)");
	AddWidget(&terrainMat1Combo);

	terrainMat2Combo.Create("Terrain Material 2: ");
	terrainMat2Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat2Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat2Combo.SetEnabled(false);
	terrainMat2Combo.OnSelect([&](wiEventArgs args) {
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
	terrainMat2Combo.SetTooltip("Choose a sub terrain blend material. (BLUE vertex color mask)");
	AddWidget(&terrainMat2Combo);

	terrainMat3Combo.Create("Terrain Material 3: ");
	terrainMat3Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat3Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat3Combo.SetEnabled(false);
	terrainMat3Combo.OnSelect([&](wiEventArgs args) {
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
	terrainMat3Combo.SetTooltip("Choose a sub terrain blend material. (ALPHA vertex color mask)");
	AddWidget(&terrainMat3Combo);

	terrainGenButton.Create("Generate Terrain...");
	terrainGenButton.SetTooltip("Generate terrain meshes.");
	terrainGenButton.SetSize(XMFLOAT2(200, hei));
	terrainGenButton.SetPos(XMFLOAT2(x + 180, y += step));
	terrainGenButton.OnClick([=](wiEventArgs args) {

		terragen.Cleanup();
		terragen.SetVisible(true);

		editor->GetGUI().RemoveWidget(&terragen);
		editor->GetGUI().AddWidget(&terragen);

		terragen.SetPos(XMFLOAT2(
			terrainGenButton.translation.x + terrainGenButton.scale.x + 10,
			terrainGenButton.translation.y)
		);

		Scene& scene = wiScene::GetScene();
		Entity entity = scene.Entity_CreateObject("editorTerrain");
		ObjectComponent& object = *scene.objects.GetComponent(entity);
		object.meshID = scene.Entity_CreateMesh("terrainMesh");
		MeshComponent* mesh = scene.meshes.GetComponent(object.meshID);
		mesh->SetTerrain(true);
		mesh->subsets.emplace_back();
		mesh->subsets.back().materialID = scene.Entity_CreateMaterial("terrainMaterial");
		mesh->subsets.back().indexOffset = 0;
		MaterialComponent* material = scene.materials.GetComponent(mesh->subsets.back().materialID);
		material->SetUseVertexColors(true);

		auto generate_mesh = [=] (int width, int height, unsigned char* rgb = nullptr, 
			int channelCount = 4, float heightmap_scale = 1) 
		{
			mesh->vertex_positions.resize(width * height);
			mesh->vertex_normals.resize(width * height);
			mesh->vertex_colors.resize(width * height);
			mesh->vertex_uvset_0.resize(width* height);
			mesh->vertex_uvset_1.resize(width* height);
			mesh->vertex_atlas.resize(width* height);
			for (int i = 0; i < width; ++i)
			{
				for (int j = 0; j < height; ++j)
				{
					size_t index = size_t(i + j * width);
					mesh->vertex_positions[index] = XMFLOAT3((float)i - (float)width * 0.5f, 0, (float)j - (float)height * 0.5f);
					if (rgb != nullptr)
						mesh->vertex_positions[index].y = ((float)rgb[index * channelCount] - 127.0f) * heightmap_scale;
					mesh->vertex_colors[index] = wiColor::Red().rgba;
					XMFLOAT2 uv = XMFLOAT2((float)i / (float)width, (float)j / (float)height);
					mesh->vertex_uvset_0[index] = uv;
					mesh->vertex_uvset_1[index] = uv;
					mesh->vertex_atlas[index] = uv;
				}
			}
			mesh->indices.resize((width - 1) * (height - 1) * 6);
			size_t counter = 0;
			for (int x = 0; x < width - 1; x++)
			{
				for (int y = 0; y < height - 1; y++)
				{
					int lowerLeft = x + y * width;
					int lowerRight = (x + 1) + y * width;
					int topLeft = x + (y + 1) * width;
					int topRight = (x + 1) + (y + 1) * width;

					mesh->indices[counter++] = topLeft;
					mesh->indices[counter++] = lowerLeft;
					mesh->indices[counter++] = lowerRight;

					mesh->indices[counter++] = topLeft;
					mesh->indices[counter++] = lowerRight;
					mesh->indices[counter++] = topRight;
				}
			}
			mesh->subsets.back().indexCount = (uint32_t)mesh->indices.size();

			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);
		};
		generate_mesh(128, 128);
		
		editor->ClearSelected();
		wiScene::PickResult pick;
		pick.entity = entity;
		pick.subsetIndex = 0;
		editor->AddSelected(pick);
		SetEntity(object.meshID);





		terragen.dimXSlider.OnSlide([=](wiEventArgs args) {
			terragen.width = (int)terragen.dimXSlider.GetValue();
			terragen.height = (int)terragen.dimZSlider.GetValue();
			generate_mesh(terragen.width, terragen.height);
		});
		terragen.dimZSlider.OnSlide([=](wiEventArgs args) {
			terragen.width = (int)terragen.dimXSlider.GetValue();
			terragen.height = (int)terragen.dimZSlider.GetValue();
			generate_mesh(terragen.width, terragen.height);
		});
		terragen.dimYSlider.OnSlide([=](wiEventArgs args) {
			generate_mesh(terragen.width, terragen.height, terragen.rgb, terragen.channelCount, args.fValue);
		});

		terragen.heightmapButton.OnClick([=](wiEventArgs args) {

			wiHelper::FileDialogParams params;
			params.type = wiHelper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions.push_back("dds");
			params.extensions.push_back("png");
			params.extensions.push_back("jpg");
			params.extensions.push_back("tga");
			wiHelper::FileDialog(params, [=](std::string fileName) {
				wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					if (terragen.rgb != nullptr)
					{
						stbi_image_free(terragen.rgb);
						terragen.rgb = nullptr;
					}

					int bpp;
					terragen.rgb = stbi_load(fileName.c_str(), &terragen.width, &terragen.height, &bpp, terragen.channelCount);

					generate_mesh(terragen.width, terragen.height, terragen.rgb, terragen.channelCount, terragen.dimYSlider.GetValue());
				});
			});
		});

	});
	AddWidget(&terrainGenButton);



	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 1000, 80, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void MeshWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene&scene = wiScene::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh == nullptr || !mesh->IsTerrain())
	{
		terragen.Cleanup();
		terragen.SetVisible(false);
	}

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
		if (mesh->IsTerrain()) ss << endl << endl << "Terrain will use 4 blend materials and blend by vertex colors, the default one is always the subset material and uses RED vertex color channel mask, the other 3 are selectable below.";
		meshInfoLabel.SetText(ss.str());

		terrainCheckBox.SetCheck(mesh->IsTerrain());

		terrainMat1Combo.ClearItems();
		terrainMat1Combo.AddItem("OFF (Use subset)");
		terrainMat2Combo.ClearItems();
		terrainMat2Combo.AddItem("OFF (Use subset)");
		terrainMat3Combo.ClearItems();
		terrainMat3Combo.AddItem("OFF (Use subset)");
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			Entity entity = scene.materials.GetEntity(i);
			const NameComponent& name = *scene.names.GetComponent(entity);
			terrainMat1Combo.AddItem(name.name);
			terrainMat2Combo.AddItem(name.name);
			terrainMat3Combo.AddItem(name.name);

			if (mesh->terrain_material1 == entity)
			{
				terrainMat1Combo.SetSelected((int)i + 1);
			}
			if (mesh->terrain_material2 == entity)
			{
				terrainMat2Combo.SetSelected((int)i + 1);
			}
			if (mesh->terrain_material3 == entity)
			{
				terrainMat3Combo.SetSelected((int)i + 1);
			}
		}

		doubleSidedCheckBox.SetCheck(mesh->IsDoubleSided());

		const ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostorDistanceSlider.SetValue(impostor->swapInDistance);
		}
		tessellationFactorSlider.SetValue(mesh->GetTessellationFactor());

		softbodyCheckBox.SetCheck(false);

		SoftBodyPhysicsComponent* physicscomponent = wiScene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			softbodyCheckBox.SetCheck(true);
			massSlider.SetValue(physicscomponent->mass);
			frictionSlider.SetValue(physicscomponent->friction);
		}
		SetEnabled(true);
	}
	else
	{
		meshInfoLabel.SetText("Select a mesh...");
		SetEnabled(false);
	}

	terrainGenButton.SetEnabled(true);
}
