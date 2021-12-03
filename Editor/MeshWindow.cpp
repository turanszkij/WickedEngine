#include "stdafx.h"
#include "MeshWindow.h"
#include "Editor.h"

#include "Utility/stb_image.h"

#include "meshoptimizer/meshoptimizer.h"

#include <string>

using namespace wi::ecs;
using namespace wi::scene;

struct TerraGen : public wi::gui::Window
{
	wi::gui::Slider dimXSlider;
	wi::gui::Slider dimYSlider;
	wi::gui::Slider dimZSlider;
	wi::gui::Button heightmapButton;

	// heightmap texture:
	unsigned char* rgb = nullptr;
	const int channelCount = 4;
	int width = 0, height = 0;

	TerraGen()
	{
		wi::gui::Window::Create("TerraGen");
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
	wi::gui::Window::Create("Mesh Window");
	SetSize(XMFLOAT2(580, 580));

	float x = 150;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	meshInfoLabel.Create("Mesh Info");
	meshInfoLabel.SetPos(XMFLOAT2(x - 50, y += step));
	meshInfoLabel.SetSize(XMFLOAT2(450, 190));
	meshInfoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&meshInfoLabel);

	// Left side:
	y = meshInfoLabel.GetScale().y + 5;

	subsetComboBox.Create("Selected subset: ");
	subsetComboBox.SetSize(XMFLOAT2(40, hei));
	subsetComboBox.SetPos(XMFLOAT2(x, y += step));
	subsetComboBox.SetEnabled(false);
	subsetComboBox.OnSelect([=](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			subset = args.iValue;
			if (!editor->translator.selected.empty())
			{
				editor->translator.selected.back().subsetIndex = subset;
			}
		}
	});
	subsetComboBox.SetTooltip("Select a subset. A subset can also be selected by picking it in the 3D scene.");
	AddWidget(&subsetComboBox);

	terrainCheckBox.Create("Terrain: ");
	terrainCheckBox.SetTooltip("If enabled, the mesh will use multiple materials and blend between them based on vertex colors.");
	terrainCheckBox.SetSize(XMFLOAT2(hei, hei));
	terrainCheckBox.SetPos(XMFLOAT2(x, y += step));
	terrainCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetTerrain(args.bValue);
			if (args.bValue && mesh->vertex_colors.empty())
			{
				mesh->vertex_colors.resize(mesh->vertex_positions.size());
				std::fill(mesh->vertex_colors.begin(), mesh->vertex_colors.end(), wi::Color::Red().rgba); // fill red (meaning only blend base material)
				mesh->CreateRenderData();

				for (auto& subset : mesh->subsets)
				{
					MaterialComponent* material = wi::scene::GetScene().materials.GetComponent(subset.materialID);
					if (material != nullptr)
					{
						material->SetUseVertexColors(true);
					}
				}
			}
			SetEntity(entity, subset); // refresh information label
		}
		});
	AddWidget(&terrainCheckBox);

	doubleSidedCheckBox.Create("Double Sided: ");
	doubleSidedCheckBox.SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox.SetSize(XMFLOAT2(hei, hei));
	doubleSidedCheckBox.SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
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
	softbodyCheckBox.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();
		SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				SoftBodyPhysicsComponent& softbody = scene.softbodies.Create(entity);
				softbody.friction = frictionSlider.GetValue();
				softbody.restitution = restitutionSlider.GetValue();
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

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.SetSize(XMFLOAT2(100, hei));
	massSlider.SetPos(XMFLOAT2(x, y += step));
	massSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
	});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.SetSize(XMFLOAT2(100, hei));
	frictionSlider.SetPos(XMFLOAT2(x, y += step));
	frictionSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
	});
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.SetSize(XMFLOAT2(100, hei));
	restitutionSlider.SetPos(XMFLOAT2(x, y += step));
	restitutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->restitution = args.fValue;
		}
		});
	AddWidget(&restitutionSlider);

	impostorCreateButton.Create("Create Impostor");
	impostorCreateButton.SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton.SetSize(XMFLOAT2(240, hei));
	impostorCreateButton.SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton.OnClick([&](wi::gui::EventArgs args) {
	    Scene& scene = wi::scene::GetScene();
		ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
	    if (impostor == nullptr)
		{
		    impostorCreateButton.SetText("Delete Impostor");
			scene.impostors.Create(entity).swapInDistance = impostorDistanceSlider.GetValue();
		}
	    else
		{
			impostorCreateButton.SetText("Create Impostor");
			scene.impostors.Remove(entity);
		}
	});
	AddWidget(&impostorCreateButton);

	impostorDistanceSlider.Create(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider.SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider.SetSize(XMFLOAT2(100, hei));
	impostorDistanceSlider.SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider.OnSlide([&](wi::gui::EventArgs args) {
		ImpostorComponent* impostor = wi::scene::GetScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	AddWidget(&impostorDistanceSlider);

	tessellationFactorSlider.Create(0, 100, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider.SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider.SetSize(XMFLOAT2(100, hei));
	tessellationFactorSlider.SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider.OnSlide([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
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
	flipCullingButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipCulling();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&flipCullingButton);

	flipNormalsButton.Create("Flip Normals");
	flipNormalsButton.SetTooltip("Flip surface normals.");
	flipNormalsButton.SetSize(XMFLOAT2(240, hei));
	flipNormalsButton.SetPos(XMFLOAT2(x - 50, y += step));
	flipNormalsButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipNormals();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&flipNormalsButton);

	computeNormalsSmoothButton.Create("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex. This can reduce vertex count, but is slow.");
	computeNormalsSmoothButton.SetSize(XMFLOAT2(240, hei));
	computeNormalsSmoothButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH);
			SetEntity(entity, subset);
		}
	});
	AddWidget(&computeNormalsSmoothButton);

	computeNormalsHardButton.Create("Compute Normals [HARD]");
	computeNormalsHardButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face. This can increase vertex count.");
	computeNormalsHardButton.SetSize(XMFLOAT2(240, hei));
	computeNormalsHardButton.SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_HARD);
			SetEntity(entity, subset);
		}
	});
	AddWidget(&computeNormalsHardButton);

	recenterButton.Create("Recenter");
	recenterButton.SetTooltip("Recenter mesh to AABB center.");
	recenterButton.SetSize(XMFLOAT2(240, hei));
	recenterButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->Recenter();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&recenterButton);

	recenterToBottomButton.Create("RecenterToBottom");
	recenterToBottomButton.SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton.SetSize(XMFLOAT2(240, hei));
	recenterToBottomButton.SetPos(XMFLOAT2(x - 50, y += step));
	recenterToBottomButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->RecenterToBottom();
			SetEntity(entity, subset);
		}
	});
	AddWidget(&recenterToBottomButton);

	optimizeButton.Create("Optimize");
	optimizeButton.SetTooltip("Run the meshoptimizer library.");
	optimizeButton.SetSize(XMFLOAT2(240, hei));
	optimizeButton.SetPos(XMFLOAT2(x - 50, y += step));
	optimizeButton.OnClick([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			// https://github.com/zeux/meshoptimizer#vertex-cache-optimization

			size_t index_count = mesh->indices.size();
			size_t vertex_count = mesh->vertex_positions.size();

			wi::vector<uint32_t> indices(index_count);
			meshopt_optimizeVertexCache(indices.data(), mesh->indices.data(), index_count, vertex_count);

			mesh->indices = indices;

			mesh->CreateRenderData();
			SetEntity(entity, subset);
		}
		});
	AddWidget(&optimizeButton);


	// Right side:

	x = 150;
	y = meshInfoLabel.GetScale().y + 5;

	subsetMaterialComboBox.Create("Subset Material: ");
	subsetMaterialComboBox.SetSize(XMFLOAT2(200, hei));
	subsetMaterialComboBox.SetPos(XMFLOAT2(x + 180, y += step));
	subsetMaterialComboBox.SetEnabled(false);
	subsetMaterialComboBox.OnSelect([&](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr && subset >= 0 && subset < mesh->subsets.size())
		{
			MeshComponent::MeshSubset& meshsubset = mesh->subsets[subset];
			if (args.iValue == 0)
			{
				meshsubset.materialID = INVALID_ENTITY;
			}
			else
			{
				MeshComponent::MeshSubset& meshsubset = mesh->subsets[subset];
				meshsubset.materialID = scene.materials.GetEntity(args.iValue - 1);
			}
		}
		});
	subsetMaterialComboBox.SetTooltip("Set the base material of the selected MeshSubset");
	AddWidget(&subsetMaterialComboBox);

	terrainMat1Combo.Create("Terrain Material 1: ");
	terrainMat1Combo.SetSize(XMFLOAT2(200, hei));
	terrainMat1Combo.SetPos(XMFLOAT2(x + 180, y += step));
	terrainMat1Combo.SetEnabled(false);
	terrainMat1Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material1 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
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
	terrainMat2Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material2 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
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
	terrainMat3Combo.OnSelect([&](wi::gui::EventArgs args) {
		MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			if (args.iValue == 0)
			{
				mesh->terrain_material3 = INVALID_ENTITY;
			}
			else
			{
				Scene& scene = wi::scene::GetScene();
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
	terrainGenButton.OnClick([=](wi::gui::EventArgs args) {

		terragen.Cleanup();
		terragen.SetVisible(true);

		editor->GetGUI().RemoveWidget(&terragen);
		editor->GetGUI().AddWidget(&terragen);

		terragen.SetPos(XMFLOAT2(
			terrainGenButton.translation.x + terrainGenButton.scale.x + 10,
			terrainGenButton.translation.y)
		);

		Scene& scene = wi::scene::GetScene();
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
					mesh->vertex_colors[index] = wi::Color::Red().rgba;
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
		wi::scene::PickResult pick;
		pick.entity = entity;
		pick.subsetIndex = 0;
		editor->AddSelected(pick);
		SetEntity(object.meshID, pick.subsetIndex);





		terragen.dimXSlider.OnSlide([=](wi::gui::EventArgs args) {
			terragen.width = (int)terragen.dimXSlider.GetValue();
			terragen.height = (int)terragen.dimZSlider.GetValue();
			generate_mesh(terragen.width, terragen.height);
		});
		terragen.dimZSlider.OnSlide([=](wi::gui::EventArgs args) {
			terragen.width = (int)terragen.dimXSlider.GetValue();
			terragen.height = (int)terragen.dimZSlider.GetValue();
			generate_mesh(terragen.width, terragen.height);
		});
		terragen.dimYSlider.OnSlide([=](wi::gui::EventArgs args) {
			generate_mesh(terragen.width, terragen.height, terragen.rgb, terragen.channelCount, args.fValue);
		});

		terragen.heightmapButton.OnClick([=](wi::gui::EventArgs args) {

			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
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


	morphTargetCombo.Create("Morph Target:");
	morphTargetCombo.SetSize(XMFLOAT2(100, hei));
	morphTargetCombo.SetPos(XMFLOAT2(x + 280, y += step));
	morphTargetCombo.OnSelect([&](wi::gui::EventArgs args) {
	    MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
	    if (mesh != nullptr && args.iValue < (int)mesh->targets.size())
	    {
			morphTargetSlider.SetValue(mesh->targets[args.iValue].weight);
	    }
	});
	morphTargetCombo.SetTooltip("Choose a morph target to edit weight.");
	AddWidget(&morphTargetCombo);

	morphTargetSlider.Create(0, 1, 0, 100000, "Weight: ");
	morphTargetSlider.SetTooltip("Set the weight for morph target");
	morphTargetSlider.SetSize(XMFLOAT2(100, hei));
	morphTargetSlider.SetPos(XMFLOAT2(x + 280, y += step));
	morphTargetSlider.OnSlide([&](wi::gui::EventArgs args) {
	    MeshComponent* mesh = wi::scene::GetScene().meshes.GetComponent(entity);
	    if (mesh != nullptr && morphTargetCombo.GetSelected() < (int)mesh->targets.size())
	    {
			mesh->targets[morphTargetCombo.GetSelected()].weight = args.fValue;
			mesh->dirty_morph = true;
	    }
	});
	AddWidget(&morphTargetSlider);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 1000, 80, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY, -1);
}

void MeshWindow::SetEntity(Entity entity, int subset)
{
	subset = std::max(0, subset);

	this->entity = entity;
	this->subset = subset;

	Scene& scene = wi::scene::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh == nullptr || !mesh->IsTerrain())
	{
		terragen.Cleanup();
		terragen.SetVisible(false);
	}

	if (mesh != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		std::string ss;
		ss += "Mesh name: " + name.name + "\n";
		ss += "Vertex count: " + std::to_string(mesh->vertex_positions.size()) + "\n";
		ss += "Index count: " + std::to_string(mesh->indices.size()) + "\n";
		ss += "Subset count: " + std::to_string(mesh->subsets.size()) + "\n";
		ss += "\nVertex buffers: ";
		if (mesh->vertexBuffer_POS.IsValid()) ss += "position; ";
		if (mesh->vertexBuffer_UV0.IsValid()) ss += "uvset_0; ";
		if (mesh->vertexBuffer_UV1.IsValid()) ss += "uvset_1; ";
		if (mesh->vertexBuffer_ATL.IsValid()) ss += "atlas; ";
		if (mesh->vertexBuffer_COL.IsValid()) ss += "color; ";
		if (mesh->vertexBuffer_PRE.IsValid()) ss += "previous_position; ";
		if (mesh->vertexBuffer_BON.IsValid()) ss += "bone; ";
		if (mesh->vertexBuffer_TAN.IsValid()) ss += "tangent; ";
		if (mesh->streamoutBuffer_POS.IsValid()) ss += "streamout_position; ";
		if (mesh->streamoutBuffer_TAN.IsValid()) ss += "streamout_tangents; ";
		if (mesh->subsetBuffer.IsValid()) ss += "subset; ";
		if (mesh->IsTerrain()) ss += "\n\nTerrain will use 4 blend materials and blend by vertex colors, the default one is always the subset material and uses RED vertex color channel mask, the other 3 are selectable below.";
		meshInfoLabel.SetText(ss);

		terrainCheckBox.SetCheck(mesh->IsTerrain());

		subsetComboBox.ClearItems();
		for (size_t i = 0; i < mesh->subsets.size(); ++i)
		{
			subsetComboBox.AddItem(std::to_string(i));
		}
		if (subset >= 0)
		{
			subsetComboBox.SetSelected(subset);
		}

		subsetMaterialComboBox.ClearItems();
		subsetMaterialComboBox.AddItem("NO MATERIAL");
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
			subsetMaterialComboBox.AddItem(name.name);
			terrainMat1Combo.AddItem(name.name);
			terrainMat2Combo.AddItem(name.name);
			terrainMat3Combo.AddItem(name.name);

			if (subset >= 0 && subset < mesh->subsets.size() && mesh->subsets[subset].materialID == entity)
			{
				subsetMaterialComboBox.SetSelected((int)i + 1);
			}
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

		SoftBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			softbodyCheckBox.SetCheck(true);
			massSlider.SetValue(physicscomponent->mass);
			frictionSlider.SetValue(physicscomponent->friction);
			restitutionSlider.SetValue(physicscomponent->restitution);
		}

		uint8_t selected = morphTargetCombo.GetSelected();
		morphTargetCombo.ClearItems();
		for (size_t i = 0; i < mesh->targets.size(); i++)
		{
		    morphTargetCombo.AddItem(std::to_string(i).c_str());
		}
		if (selected < mesh->targets.size())
		{
		    morphTargetCombo.SetSelected(selected);
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
