#include "stdafx.h"
#include "MeshWindow.h"
#include "Editor.h"

#include "Utility/stb_image.h"

#include "meshoptimizer/meshoptimizer.h"

#include <string>
#include <random>

using namespace wi::ecs;
using namespace wi::scene;

struct TerraGen : public wi::gui::Window
{
	wi::gui::Slider dimXSlider;
	wi::gui::Slider dimYSlider;
	wi::gui::Slider perlinBlendSlider;
	wi::gui::Slider perlinFrequencySlider;
	wi::gui::Slider perlinSeedSlider;
	wi::gui::Slider perlinOctavesSlider;
	wi::gui::Slider voronoiBlendSlider;
	wi::gui::Slider voronoiFrequencySlider;
	wi::gui::Slider voronoiFadeSlider;
	wi::gui::Slider voronoiShapeSlider;
	wi::gui::Slider voronoiFalloffSlider;
	wi::gui::Slider voronoiSeedSlider;
	wi::gui::Button heightmapButton;
	wi::gui::Slider heightmapBlendSlider;

	// heightmap texture:
	unsigned char* rgb = nullptr;
	const int channelCount = 4;

	TerraGen()
	{
		wi::gui::Window::Create("TerraGen");
		SetSize(XMFLOAT2(410, 400));

		float xx = 135;
		float yy = 0;
		float stepstep = 25;
		float heihei = 20;

		dimXSlider.Create(16, 1024, 64, 1024 - 16, "Chunk Width: ");
		dimXSlider.SetTooltip("Terrain mesh grid resolution on horizontal (XZ) axis");
		dimXSlider.SetSize(XMFLOAT2(200, heihei));
		dimXSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&dimXSlider);

		dimYSlider.Create(0, 100, 10, 10000, "Scale Y: ");
		dimYSlider.SetTooltip("Terrain mesh grid scale on vertical (Y) axis");
		dimYSlider.SetSize(XMFLOAT2(200, heihei));
		dimYSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&dimYSlider);

		perlinBlendSlider.Create(0, 1, 0.5f, 10000, "Perlin Blend: ");
		perlinBlendSlider.SetTooltip("Amount of perlin noise to use");
		perlinBlendSlider.SetSize(XMFLOAT2(200, heihei));
		perlinBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinBlendSlider);

		perlinFrequencySlider.Create(0.001f, 0.1f, 0.01f, 10000, "Perlin Frequency: ");
		perlinFrequencySlider.SetTooltip("Frequency for the perlin noise");
		perlinFrequencySlider.SetSize(XMFLOAT2(200, heihei));
		perlinFrequencySlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinFrequencySlider);

		perlinSeedSlider.Create(1, 12345, 1234, 12344, "Perlin Seed: ");
		perlinSeedSlider.SetTooltip("Seed for the perlin noise");
		perlinSeedSlider.SetSize(XMFLOAT2(200, heihei));
		perlinSeedSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinSeedSlider);

		perlinOctavesSlider.Create(1, 8, 6, 7, "Perlin Detail: ");
		perlinOctavesSlider.SetTooltip("Octave count for the perlin noise");
		perlinOctavesSlider.SetSize(XMFLOAT2(200, heihei));
		perlinOctavesSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&perlinOctavesSlider);

		voronoiBlendSlider.Create(0, 1, 0.5f, 10000, "Voronoi Blend: ");
		voronoiBlendSlider.SetTooltip("Amount of voronoi to use for elevation");
		voronoiBlendSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiBlendSlider);

		voronoiFrequencySlider.Create(0.001f, 0.1f, 0.01f, 200, "Voronoi Frequency: ");
		voronoiFrequencySlider.SetTooltip("Voronoi can create distinctly elevated areas, the more cells there are, smaller the consecutive areas");
		voronoiFrequencySlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFrequencySlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFrequencySlider);

		voronoiFadeSlider.Create(0, 100, 10, 10000, "Voronoi Fade: ");
		voronoiFadeSlider.SetTooltip("Fade out voronoi regions by distance from cell's center");
		voronoiFadeSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFadeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFadeSlider);

		voronoiShapeSlider.Create(0, 1, 0.2f, 10000, "Voronoi Shape: ");
		voronoiShapeSlider.SetTooltip("How much the voronoi shape will be kept");
		voronoiShapeSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiShapeSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiShapeSlider);

		voronoiFalloffSlider.Create(0, 8, 4, 10000, "Voronoi Falloff: ");
		voronoiFalloffSlider.SetTooltip("Controls the falloff of the voronoi distance fade effect");
		voronoiFalloffSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiFalloffSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiFalloffSlider);

		voronoiSeedSlider.Create(1, 12345, 4852, 12344, "Voronoi Seed: ");
		voronoiSeedSlider.SetTooltip("Voronoi can create distinctly elevated areas");
		voronoiSeedSlider.SetSize(XMFLOAT2(200, heihei));
		voronoiSeedSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&voronoiSeedSlider);


		heightmapButton.Create("Load Heightmap...");
		heightmapButton.SetTooltip("Load a heightmap texture, where the red channel corresponds to terrain height and the resolution to dimensions");
		heightmapButton.SetSize(XMFLOAT2(200, heihei));
		heightmapButton.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&heightmapButton);

		heightmapBlendSlider.Create(0, 1, 1, 10000, "Heightmap Blend: ");
		heightmapBlendSlider.SetTooltip("Amount of displacement coming from the heightmap texture");
		heightmapBlendSlider.SetSize(XMFLOAT2(200, heihei));
		heightmapBlendSlider.SetPos(XMFLOAT2(xx, yy += stepstep));
		AddWidget(&heightmapBlendSlider);



		auto generate_callback = [=](wi::gui::EventArgs args) {
			Generate();
		};
		dimXSlider.OnSlide(generate_callback);
		dimYSlider.OnSlide(generate_callback);
		perlinFrequencySlider.OnSlide(generate_callback);
		perlinBlendSlider.OnSlide(generate_callback);
		perlinSeedSlider.OnSlide(generate_callback);
		perlinOctavesSlider.OnSlide(generate_callback);
		voronoiBlendSlider.OnSlide(generate_callback);
		voronoiFrequencySlider.OnSlide(generate_callback);
		voronoiFadeSlider.OnSlide(generate_callback);
		voronoiShapeSlider.OnSlide(generate_callback);
		voronoiFalloffSlider.OnSlide(generate_callback);
		voronoiSeedSlider.OnSlide(generate_callback);
		heightmapBlendSlider.OnSlide(generate_callback);

		heightmapButton.OnClick([=](wi::gui::EventArgs args) {

			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Texture";
			params.extensions = wi::resourcemanager::GetSupportedImageExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					if (rgb != nullptr)
					{
						stbi_image_free(rgb);
						rgb = nullptr;
					}

					int bpp = 0;
					int width = 0;
					int height = 0;
					rgb = stbi_load(fileName.c_str(), &width, &height, &bpp, channelCount);
					if (rgb != nullptr)
					{
						dimXSlider.SetValue((float)width);
						perlinBlendSlider.SetValue(0);
						voronoiBlendSlider.SetValue(0);
						Generate();
					}
					});
				});
			});
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

	Entity terrainEntity = INVALID_ENTITY;
	Entity materialEntity = INVALID_ENTITY;
	struct Chunk
	{
		int x, z;
	};
	wi::unordered_map<size_t, Entity> chunks; // chunk -> object
	int chunk_dim = 8;
	wi::vector<uint32_t> chunkIndices;
	wi::noise::Perlin perlin;

	void Generate()
	{
		Scene& scene = wi::scene::GetScene();

		chunks.clear();

		scene.Entity_Remove(terrainEntity);
		terrainEntity = CreateEntity();
		scene.transforms.Create(terrainEntity);
		scene.names.Create(terrainEntity) = "terrain";

		materialEntity = scene.Entity_CreateMaterial("terrainMaterial");
		scene.Component_Attach(materialEntity, terrainEntity);
		MaterialComponent* material = scene.materials.GetComponent(materialEntity);
		material->SetUseVertexColors(true);
		material->SetRoughness(1);

		const int width = (int)dimXSlider.GetValue();
		chunkIndices.resize((width - 1) * (width - 1) * 6);
		size_t counter = 0;
		for (int x = 0; x < width - 1; x++)
		{
			for (int z = 0; z < width - 1; z++)
			{
				int lowerLeft = x + z * width;
				int lowerRight = (x + 1) + z * width;
				int topLeft = x + (z + 1) * width;
				int topRight = (x + 1) + (z + 1) * width;

				chunkIndices[counter++] = topLeft;
				chunkIndices[counter++] = lowerLeft;
				chunkIndices[counter++] = lowerRight;

				chunkIndices[counter++] = topLeft;
				chunkIndices[counter++] = lowerRight;
				chunkIndices[counter++] = topRight;
			}
		}

		const float perlinBlend = perlinBlendSlider.GetValue();
		if (perlinBlend > 0)
		{
			const uint32_t perlinSeed = (uint32_t)perlinSeedSlider.GetValue();
			perlin.init(perlinSeed);
		}

		Update();
	}

	void Update()
	{
		if (terrainEntity == INVALID_ENTITY)
			return;

		wi::Timer timer;
		Scene& scene = wi::scene::GetScene();
		const int width = (int)dimXSlider.GetValue();
		const float half_width = width * 0.5f;
		const float width_rcp = 1.0f / width;
		const float verticalScale = dimYSlider.GetValue();
		const float heightmapBlend = heightmapBlendSlider.GetValue();
		const float perlinBlend = perlinBlendSlider.GetValue();
		const uint32_t perlinSeed = (uint32_t)perlinSeedSlider.GetValue();
		const int perlinOctaves = (int)perlinOctavesSlider.GetValue();
		const float perlinFrequency = perlinFrequencySlider.GetValue();
		const float voronoiBlend = voronoiBlendSlider.GetValue();
		const float voronoiFrequency = voronoiFrequencySlider.GetValue();
		const float voronoiFade = voronoiFadeSlider.GetValue();
		const float voronoiShape = voronoiShapeSlider.GetValue();
		const float voronoiFalloff = voronoiFalloffSlider.GetValue();
		const uint32_t voronoiSeed = (uint32_t)voronoiSeedSlider.GetValue();
		const uint32_t vertexCount = width * width;

		const CameraComponent& camera = GetCamera();
		Chunk center_chunk;
		center_chunk.x = (int)std::floor((camera.Eye.x + half_width) * width_rcp);
		center_chunk.z = (int)std::floor((camera.Eye.z + half_width) * width_rcp);
		for(int growth = 0; growth < 4; ++growth)
		{
			const int probe_grid_area = 1 << growth;
			for (int i = -probe_grid_area; i <= probe_grid_area; ++i)
			{
				for (int j = -probe_grid_area; j <= probe_grid_area; ++j)
				{
					Chunk chunk = center_chunk;
					chunk.x += i;
					chunk.z += j;
					size_t key = 0;
					wi::helper::hash_combine(key, chunk.x);
					wi::helper::hash_combine(key, chunk.z);
					if (chunks.count(key) == 0)
					{
						Entity chunkObjectEntity = scene.Entity_CreateObject("chunk_object" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z));
						ObjectComponent& object = *scene.objects.GetComponent(chunkObjectEntity);
						scene.Component_Attach(chunkObjectEntity, terrainEntity);
						chunks[key] = chunkObjectEntity;

						TransformComponent& transform = *scene.transforms.GetComponent(chunkObjectEntity);
						transform.ClearTransform();
						XMFLOAT3 chunk_pos = XMFLOAT3(chunk.x * (width - 1) - half_width, 0, chunk.z * (width - 1) - half_width);
						transform.Translate(chunk_pos);

						Entity chunkMeshEntity = CreateEntity();
						MeshComponent& mesh = scene.meshes.Create(chunkMeshEntity);
						scene.names.Create(chunkMeshEntity) = "chunk_mesh" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z);
						scene.Component_Attach(chunkMeshEntity, chunkObjectEntity);
						object.meshID = chunkMeshEntity;
						mesh.indices = chunkIndices;
						mesh.SetTerrain(true);
						mesh.subsets.emplace_back();
						mesh.subsets.back().materialID = materialEntity;
						mesh.subsets.back().indexCount = (uint32_t)chunkIndices.size();
						mesh.subsets.back().indexOffset = 0;
						mesh.vertex_positions.resize(vertexCount);
						mesh.vertex_normals.resize(vertexCount);
						mesh.vertex_colors.resize(vertexCount);
						mesh.vertex_uvset_0.resize(vertexCount);
						mesh.vertex_uvset_1.resize(vertexCount);
						mesh.vertex_atlas.resize(vertexCount);
						for (uint32_t index = 0; index < vertexCount; ++index)
						{
							const float x = float(index % width);
							const float z = float(index / width);
							const XMFLOAT2 uv = XMFLOAT2(x * width_rcp, z * width_rcp);
							mesh.vertex_positions[index] = XMFLOAT3(x, 0, z);
							mesh.vertex_colors[index] = 0xFF; // vertex color is used for material blending, red means fully use the first material
							mesh.vertex_uvset_0[index] = uv;
							mesh.vertex_uvset_1[index] = uv;
							mesh.vertex_atlas[index] = uv;

							if (rgb != nullptr)
							{
								mesh.vertex_positions[index].y += ((float)rgb[index * channelCount] / 255.0f * 2 - 1) * heightmapBlend;
							}
							if (perlinBlend > 0)
							{
								XMFLOAT2 p = XMFLOAT2(x, z);
								p.x += chunk_pos.x;
								p.y += chunk_pos.z;
								p.x *= perlinFrequency;
								p.y *= perlinFrequency;
								mesh.vertex_positions[index].y += perlin.compute(p.x, p.y, 0, perlinOctaves) * perlinBlend;
							}
							if (voronoiBlend > 0)
							{
								XMFLOAT2 p = XMFLOAT2(x, z);
								p.x += chunk_pos.x;
								p.y += chunk_pos.z;
								p.x *= voronoiFrequency;
								p.y *= voronoiFrequency;
								wi::noise::voronoi::Result res = wi::noise::voronoi::compute(p.x, p.y, (float)voronoiSeed);
								float weight = std::pow(1 - wi::math::saturate((res.distance - voronoiShape) * voronoiFade), std::max(0.0001f, voronoiFalloff));
								float elevation = res.cell_id - 0.5f;
								mesh.vertex_positions[index].y += elevation * weight * voronoiBlend;
							}
							mesh.vertex_positions[index].y *= verticalScale;
						}
						mesh.ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH_FAST);

						if (timer.elapsed_milliseconds() > 10)
							return;

					}
				}
			}
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

		terragen.Generate();


		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_ADD;
		editor->RecordSelection(archive);
		
		//editor->ClearSelected();
		//wi::scene::PickResult pick;
		//pick.entity = chunkObjectEntity;
		//pick.subsetIndex = 0;
		//editor->AddSelected(pick);

		editor->RecordSelection(archive);
		editor->RecordAddedEntity(archive, terragen.terrainEntity);

		//SetEntity(object.meshID, pick.subsetIndex);

		editor->RefreshSceneGraphView();


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
	terragen.Update();
	subset = std::max(0, subset);

	this->entity = entity;
	this->subset = subset;

	Scene& scene = wi::scene::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		std::string ss;
		ss += "Mesh name: " + name.name + "\n";
		ss += "Vertex count: " + std::to_string(mesh->vertex_positions.size()) + "\n";
		ss += "Index count: " + std::to_string(mesh->indices.size()) + "\n";
		ss += "Subset count: " + std::to_string(mesh->subsets.size()) + "\n";
		ss += "GPU memory: " + std::to_string((mesh->generalBuffer.GetDesc().size + mesh->streamoutBuffer.GetDesc().size) / 1024.0f / 1024.0f) + " MB\n";
		ss += "\nVertex buffers: ";
		if (mesh->vb_pos_nor_wind.IsValid()) ss += "position; ";
		if (mesh->vb_uvs.IsValid()) ss += "uvsets; ";
		if (mesh->vb_atl.IsValid()) ss += "atlas; ";
		if (mesh->vb_col.IsValid()) ss += "color; ";
		if (mesh->so_pre.IsValid()) ss += "previous_position; ";
		if (mesh->vb_bon.IsValid()) ss += "bone; ";
		if (mesh->vb_tan.IsValid()) ss += "tangent; ";
		if (mesh->so_pos_nor_wind.IsValid()) ss += "streamout_position; ";
		if (mesh->so_tan.IsValid()) ss += "streamout_tangents; ";
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

		if (mesh->targets.empty())
		{
			morphTargetCombo.SetEnabled(false);
			morphTargetSlider.SetEnabled(false);
		}
		else
		{
			morphTargetCombo.SetEnabled(true);
			morphTargetSlider.SetEnabled(true);
		}
	}
	else
	{
		meshInfoLabel.SetText("Select a mesh...");
		SetEnabled(false);
	}

	terrainGenButton.SetEnabled(true);
}
