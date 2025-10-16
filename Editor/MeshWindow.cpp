#include "stdafx.h"
#include "MeshWindow.h"

#include "Utility/stb_image.h"
#include "Utility/meshoptimizer/meshoptimizer.h"

using namespace wi::ecs;
using namespace wi::scene;

MeshComponent* get_mesh(Scene& scene, PickResult x)
{
	MeshComponent* mesh = scene.meshes.GetComponent(x.entity);
	if (mesh == nullptr)
	{
		// Mesh could be selected indirectly as part of selected object:
		ObjectComponent* object = scene.objects.GetComponent(x.entity);
		if (object != nullptr && object->meshID != INVALID_ENTITY)
		{
			mesh = scene.meshes.GetComponent(object->meshID);
		}
	}
	return mesh;
};

void MeshWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_MESH " Mesh", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(580, 880));

	closeButton.SetTooltip("Delete MeshComponent");
	OnClose([this](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().meshes.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	meshInfoLabel.Create("Mesh Info");
	meshInfoLabel.SetColor(wi::Color::Transparent());
	meshInfoLabel.SetFitTextEnabled(true);
	AddWidget(&meshInfoLabel);

	subsetComboBox.Create("Select subset: ");
	subsetComboBox.SetEnabled(false);
	subsetComboBox.OnSelect([this](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			subset = args.iValue;

			uint32_t main_subset_count = mesh->subsets_per_lod > 0 ? mesh->subsets_per_lod : (uint32_t)mesh->subsets.size();
			if (main_subset_count <= (uint32_t)subset)
			{
				wi::Archive& archive = editor->AdvanceHistory();
				archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
				editor->RecordEntity(archive, entity);

				mesh->CreateSubset();

				editor->RecordEntity(archive, entity);
			}

			SetEntity(entity, subset);
		}
	});
	subsetComboBox.SetTooltip("Select a subset. A subset can also be selected by picking it in the 3D scene.\nLook at the material window when a subset is selected to edit it.\nIf you add a new subset, LODs will be lost and need to be regenerated!");
	AddWidget(&subsetComboBox);

	subsetRemoveButton.Create("X");
	subsetRemoveButton.SetTooltip("Remove currently selected subset. LODs will be lost and need to be regenerated!");
	subsetRemoveButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			int selected = subsetComboBox.GetSelected();

			mesh->DeleteSubset(uint32_t(selected));

			SetEntity(entity, selected - 1);

			editor->RecordEntity(archive, entity);
		}
	});
	AddWidget(&subsetRemoveButton);

	subsetLastButton.Create("Move subset to last index");
	subsetLastButton.SetTooltip("Move subset to the last index to render last. Useful if you want to force transparency render order within subsets.");
	subsetLastButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
			editor->RecordEntity(archive, entity);

			int selected = subsetComboBox.GetSelected();

			uint32_t lod_count = mesh->GetLODCount();
			wi::vector<MeshComponent::MeshSubset> newSubsets;
			newSubsets.reserve(mesh->subsets.size());

			for (uint32_t lod = 0; lod < lod_count; ++lod)
			{
				uint32_t first_subset = 0;
				uint32_t last_subset = 0;
				mesh->GetLODSubsetRange(lod, first_subset, last_subset);
				int s = 0;
				for (uint32_t i = first_subset; i < last_subset; ++i)
				{
					if (s != selected)
					{
						newSubsets.push_back(mesh->subsets[i]);
					}
					s++;
				}
				newSubsets.push_back(mesh->subsets[first_subset + selected]);
			}
			mesh->subsets = newSubsets;
			SetEntity(entity, mesh->subsets_per_lod > 0 ? ((int)mesh->subsets_per_lod - 1) : (int)mesh->subsets.size());

			editor->RecordEntity(archive, entity);
		}
	});
	AddWidget(&subsetLastButton);

	auto forEachSelected = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				MeshComponent* mesh = get_mesh(scene, x);
				if (mesh != nullptr)
					func(mesh, args);
			}
		};
	};

	doubleSidedCheckBox.Create("Double Sided: ");
	doubleSidedCheckBox.SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox.OnClick(forEachSelected([] (auto mesh, auto args) {
		mesh->SetDoubleSided(args.bValue);
	}));
	AddWidget(&doubleSidedCheckBox);

	doubleSidedShadowCheckBox.Create("Double Sided Shadow: ");
	doubleSidedShadowCheckBox.SetTooltip("If enabled, the shadow rendering will be forced to use double sided mode.\nThis can help fix some shadow artifacts without enabling double sided mode for the main rendering of this mesh.");
	doubleSidedShadowCheckBox.OnClick(forEachSelected([] (auto mesh, auto args) {
		mesh->SetDoubleSidedShadow(args.bValue);
	}));
	AddWidget(&doubleSidedShadowCheckBox);

	bvhCheckBox.Create("Enable BVH: ");
	bvhCheckBox.SetTooltip("Whether to generate BVH (Bounding Volume Hierarchy) for the mesh or not.\nBVH will be used to optimize intersections with the mesh at an additional memory cost.\nIt is recommended to use a BVH for high polygon count meshes that will be used for intersections.\nThis CPU BVH does not support skinned or morphed geometry.");
	bvhCheckBox.OnClick(forEachSelected([] (auto mesh, auto args) {
		mesh->SetBVHEnabled(args.bValue);
	}));
	AddWidget(&bvhCheckBox);

	quantizeCheckBox.Create("Quantization Disabled: ");
	quantizeCheckBox.SetTooltip("Disable quantization of vertex positions if you notice inaccuracy errors with UNORM position formats.");
	quantizeCheckBox.OnClick(forEachSelected([] (auto mesh, auto args) {
		mesh->SetQuantizedPositionsDisabled(args.bValue);
		mesh->CreateRenderData();
		if (!mesh->BLASes.empty())
		{
			mesh->CreateRaytracingRenderData();
		}
	}));
	AddWidget(&quantizeCheckBox);

	impostorCreateButton.Create("Create Impostor");
	impostorCreateButton.SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton.OnClick([this](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
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

	impostorDistanceSlider.Create(0, 1000, 100, 10000, "Impostor Dist: ");
	impostorDistanceSlider.SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider.OnSlide([this](wi::gui::EventArgs args) {
		ImpostorComponent* impostor = editor->GetCurrentScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	AddWidget(&impostorDistanceSlider);

	tessellationFactorSlider.Create(0, 100, 0, 10000, "Tess Factor: ");
	tessellationFactorSlider.SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider.OnSlide(forEachSelected([] (auto mesh, auto args) {
		mesh->tessellationFactor = args.fValue;
	}));
	AddWidget(&tessellationFactorSlider);

	instanceSelectButton.Create("Select instances");
	instanceSelectButton.SetTooltip("Select all instances that use this mesh.");
	instanceSelectButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		wi::vector<Entity> sel;
		wi::unordered_set<const ObjectComponent*> visited_objects; // fix double visit (straight mesh + object->mesh)
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			const ObjectComponent& object = scene.objects[i];
			if (object.meshID != this->entity || visited_objects.count(&object) > 0)
				continue;
			visited_objects.insert(&object);
			sel.push_back(scene.objects.GetEntity(i));
		}
		editor->ClearSelected();
		for (auto& x : sel)
		{
			editor->AddSelected(x);
		}
	});
	AddWidget(&instanceSelectButton);

	auto changeSelectedMesh = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			wi::unordered_set<MeshComponent*> visited_meshes; // fix double visit (straight mesh + object->mesh)
			for (auto& x : editor->translator.selected)
			{
				MeshComponent* mesh = get_mesh(scene, x);
				if (mesh == nullptr || visited_meshes.count(mesh) > 0)
					continue;
				func(mesh);
				visited_meshes.insert(mesh);
			};
			SetEntity(entity, subset);
		};
	};

	flipCullingButton.Create("Flip Culling");
	flipCullingButton.SetTooltip("Flip faces to reverse triangle culling order.");
	flipCullingButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->FlipCulling();
	}));
	AddWidget(&flipCullingButton);

	flipNormalsButton.Create("Flip Normals");
	flipNormalsButton.SetTooltip("Flip surface normals.");
	flipNormalsButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->FlipNormals();
	}));
	AddWidget(&flipNormalsButton);

	computeNormalsSmoothButton.Create("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex. This can reduce vertex count, but is slow.");
	computeNormalsSmoothButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_SMOOTH);
	}));
	AddWidget(&computeNormalsSmoothButton);

	computeNormalsHardButton.Create("Compute Normals [HARD]");
	computeNormalsHardButton.SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face. This can increase vertex count.");
	computeNormalsHardButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->ComputeNormals(MeshComponent::COMPUTE_NORMALS_HARD);

	}));
	AddWidget(&computeNormalsHardButton);

	recenterButton.Create("Recenter");
	recenterButton.SetTooltip("Recenter mesh to AABB center.");
	recenterButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->Recenter();
	}));
	AddWidget(&recenterButton);

	recenterToBottomButton.Create("RecenterToBottom");
	recenterToBottomButton.SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton.OnClick(changeSelectedMesh([] (auto mesh) {
		mesh->RecenterToBottom();
	}));
	AddWidget(&recenterToBottomButton);

	mergeButton.Create("Merge Selected");
	mergeButton.SetTooltip("Merges selected objects/meshes into one.\nAll selected object transformations will be applied to meshes and all meshes will be baked into a single mesh.");
	mergeButton.OnClick([=](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		ObjectComponent merged_object;
		MeshComponent merged_mesh;
		bool valid_normals = false;
		bool valid_tangents = false;
		bool valid_uvset_0 = false;
		bool valid_uvset_1 = false;
		bool valid_atlas = false;
		bool valid_boneindices = false;
		bool valid_boneweights = false;
		bool valid_boneindices2 = false;
		bool valid_boneweights2 = false;
		bool valid_colors = false;
		bool valid_windweights = false;
		wi::unordered_set<Entity> entities_to_remove;
		Entity prev_subset_material = INVALID_ENTITY;

		// Search for first object with a mesh from selection, that will be the base:
		Entity baseEntity = INVALID_ENTITY;
		TransformComponent* baseTransform = nullptr;
		ObjectComponent* baseObject = nullptr;
		MeshComponent* baseMesh = nullptr;
		for (auto& picked : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object == nullptr)
				continue;
			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				continue;
			baseEntity = picked.entity;
			baseTransform = scene.transforms.GetComponent(picked.entity);
			baseObject = object;
			baseMesh = mesh;
		}
		if (baseObject == nullptr)
			return;
		merged_object.meshID = baseObject->meshID;

		// Merge all other meshes into the base object:
		for (auto& picked : editor->translator.selected)
		{
			ObjectComponent* object = scene.objects.GetComponent(picked.entity);
			if (object == nullptr)
				continue;
			MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
			if (mesh == nullptr)
				continue;
			merged_object._flags |= object->_flags;
			merged_object.filterMask |= object->filterMask;
			merged_mesh._flags |= mesh->_flags;
			const TransformComponent* transform = scene.transforms.GetComponent(picked.entity);
			XMMATRIX W = XMLoadFloat4x4(&transform->world);
			uint32_t vertexOffset = (uint32_t)merged_mesh.vertex_positions.size();
			uint32_t indexOffset = (uint32_t)merged_mesh.indices.size();
			for (auto& ind : mesh->indices)
			{
				merged_mesh.indices.push_back(vertexOffset + ind);
			}
			uint32_t first_subset = 0;
			uint32_t last_subset = 0;
			mesh->GetLODSubsetRange(0, first_subset, last_subset);
			for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
				if (subset.materialID != prev_subset_material)
				{
					// new subset
					prev_subset_material = subset.materialID;
					merged_mesh.subsets.push_back(subset);
					merged_mesh.subsets.back().indexOffset += indexOffset;
				}
				else
				{
					// append to previous subset
					merged_mesh.subsets.back().indexCount += subset.indexCount;
				}
			}
			for (size_t i = 0; i < mesh->vertex_positions.size(); ++i)
			{
				merged_mesh.vertex_positions.push_back(mesh->vertex_positions[i]);
				XMStoreFloat3(&merged_mesh.vertex_positions.back(), XMVector3Transform(XMLoadFloat3(&merged_mesh.vertex_positions.back()), W));

				if (mesh->vertex_normals.empty())
				{
					merged_mesh.vertex_normals.emplace_back();
				}
				else
				{
					valid_normals = true;
					merged_mesh.vertex_normals.push_back(mesh->vertex_normals[i]);
					XMStoreFloat3(&merged_mesh.vertex_normals.back(), XMVector3TransformNormal(XMLoadFloat3(&merged_mesh.vertex_normals.back()), W));
				}

				if (mesh->vertex_tangents.empty())
				{
					merged_mesh.vertex_tangents.emplace_back();
				}
				else
				{
					valid_tangents = true;
					merged_mesh.vertex_tangents.push_back(mesh->vertex_tangents[i]);
					XMFLOAT3* tan = (XMFLOAT3*)&merged_mesh.vertex_tangents.back();
					XMStoreFloat3(tan, XMVector3TransformNormal(XMLoadFloat3(tan), W));
				}

				if (mesh->vertex_uvset_0.empty())
				{
					merged_mesh.vertex_uvset_0.emplace_back();
				}
				else
				{
					valid_uvset_0 = true;
					merged_mesh.vertex_uvset_0.push_back(mesh->vertex_uvset_0[i]);
				}

				if (mesh->vertex_uvset_1.empty())
				{
					merged_mesh.vertex_uvset_1.emplace_back();
				}
				else
				{
					valid_uvset_1 = true;
					merged_mesh.vertex_uvset_1.push_back(mesh->vertex_uvset_1[i]);
				}

				if (mesh->vertex_atlas.empty())
				{
					merged_mesh.vertex_atlas.emplace_back();
				}
				else
				{
					valid_atlas = true;
					merged_mesh.vertex_atlas.push_back(mesh->vertex_atlas[i]);
				}

				if (mesh->vertex_boneindices.empty())
				{
					merged_mesh.vertex_boneindices.emplace_back();
				}
				else
				{
					valid_boneindices = true;
					merged_mesh.vertex_boneindices.push_back(mesh->vertex_boneindices[i]);
				}

				if (mesh->vertex_boneweights.empty())
				{
					merged_mesh.vertex_boneweights.emplace_back();
				}
				else
				{
					valid_boneweights = true;
					merged_mesh.vertex_boneweights.push_back(mesh->vertex_boneweights[i]);
				}

				if (mesh->vertex_boneindices2.empty())
				{
					merged_mesh.vertex_boneindices2.emplace_back();
				}
				else
				{
					valid_boneindices2 = true;
					merged_mesh.vertex_boneindices2.push_back(mesh->vertex_boneindices2[i]);
				}

				if (mesh->vertex_boneweights2.empty())
				{
					merged_mesh.vertex_boneweights2.emplace_back();
				}
				else
				{
					valid_boneweights2 = true;
					merged_mesh.vertex_boneweights2.push_back(mesh->vertex_boneweights2[i]);
				}

				if (mesh->vertex_colors.empty())
				{
					merged_mesh.vertex_colors.push_back(~0u);
				}
				else
				{
					valid_colors = true;
					merged_mesh.vertex_colors.push_back(mesh->vertex_colors[i]);
				}

				if (mesh->vertex_windweights.empty())
				{
					merged_mesh.vertex_windweights.emplace_back();
				}
				else
				{
					valid_windweights = true;
					merged_mesh.vertex_windweights.push_back(mesh->vertex_windweights[i]);
				}
			}
			if (merged_mesh.armatureID == INVALID_ENTITY)
			{
				merged_mesh.armatureID = mesh->armatureID;
			}

			if (object != baseObject) // don't remove base object
			{
				entities_to_remove.insert(picked.entity);
			}

			if (mesh != baseMesh)
			{
				// Only remove mesh if it is no longer used by any other objects:
				bool mesh_still_used = false;
				for (size_t i = 0; i < scene.objects.GetCount(); ++i)
				{
					if (entities_to_remove.count(scene.objects.GetEntity(i)) == 0 && scene.objects[i].meshID == object->meshID)
					{
						mesh_still_used = true;
						break;
					}
				}
				if (!mesh_still_used)
				{
					entities_to_remove.insert(object->meshID);
				}
			}
		}

		if (!merged_mesh.vertex_positions.empty())
		{
			if (!valid_normals)
				merged_mesh.vertex_normals.clear();
			if (!valid_tangents)
				merged_mesh.vertex_tangents.clear();
			if (!valid_uvset_0)
				merged_mesh.vertex_uvset_0.clear();
			if (!valid_uvset_1)
				merged_mesh.vertex_uvset_1.clear();
			if (!valid_atlas)
				merged_mesh.vertex_atlas.clear();
			if (!valid_boneindices)
				merged_mesh.vertex_boneindices.clear();
			if (!valid_boneweights)
				merged_mesh.vertex_boneweights.clear();
			if (!valid_boneindices2)
				merged_mesh.vertex_boneindices2.clear();
			if (!valid_boneweights2)
				merged_mesh.vertex_boneweights2.clear();
			if (!valid_colors)
				merged_mesh.vertex_colors.clear();
			if (!valid_windweights)
				merged_mesh.vertex_windweights.clear();

			*baseObject = std::move(merged_object);
			*baseMesh = std::move(merged_mesh);
			baseMesh->CreateRenderData();
			scene.Component_Detach(baseEntity);
			if (baseTransform != nullptr)
			{
				baseTransform->ClearTransform();
			}
		}

		for (auto& x : entities_to_remove)
		{
			scene.Entity_Remove(x);
		}

	});
	AddWidget(&mergeButton);

	optimizeButton.Create("Optimize");
	optimizeButton.SetTooltip("Run the meshoptimizer library.");
	optimizeButton.OnClick([=] (auto args) {
		forEachSelected([] (auto mesh, auto args) {
			// https://github.com/zeux/meshoptimizer#vertex-cache-optimization

			size_t index_count = mesh->indices.size();
			size_t vertex_count = mesh->vertex_positions.size();

			wi::vector<uint32_t> indices(index_count);
			meshopt_optimizeVertexCache(indices.data(), mesh->indices.data(), index_count, vertex_count);

			mesh->indices = indices;

			mesh->CreateRenderData();
		})(args);
		SetEntity(entity, subset);
	});
	AddWidget(&optimizeButton);

	exportHeaderButton.Create("Export to C++ header");
	exportHeaderButton.SetTooltip("Export vertex positions and index buffer into a C++ header file.\n - Object transformation (if selected through object picking) and Skinning pose will be applied.\n - Only LOD0 will be exported.\n - The generated vertex positions and indices will be reordered and optimized without considering other vertex attributes.");
	exportHeaderButton.OnClick([this](wi::gui::EventArgs args) {
		wi::helper::FileDialogParams params;
		params.description = ".h (C++ header file)";
		params.extensions.push_back("h");
		params.type = wi::helper::FileDialogParams::TYPE::SAVE;
		wi::helper::FileDialog(params, [&](std::string filename) {
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

				wi::scene::Scene& scene = editor->GetCurrentScene();
				wi::vector<XMFLOAT3> vertices;
				wi::vector<uint32_t> indices;
				uint32_t vertexOffset = 0;

				for (auto& x : editor->translator.selected)
				{
					const ObjectComponent* object = scene.objects.GetComponent(x.entity);
					if (object == nullptr)
						continue;
					const MeshComponent* mesh = scene.meshes.GetComponent(object->meshID);
					if (mesh == nullptr)
						continue;

					size_t object_index = scene.objects.GetIndex(x.entity);
					XMMATRIX M = XMLoadFloat4x4(&scene.matrix_objects[object_index]);

					// Bake transformed and skinned positions:
					const ArmatureComponent* armature = scene.armatures.GetComponent(mesh->armatureID);
					for (size_t i = 0; i < mesh->vertex_positions.size(); ++i)
					{
						XMVECTOR P;
						if (armature == nullptr)
						{
							P = XMLoadFloat3(&mesh->vertex_positions[i]);
						}
						else
						{
							P = wi::scene::SkinVertex(*mesh, *armature, (uint32_t)i);
						}
						P = XMVector3Transform(P, M);

						XMFLOAT3 pos;
						XMStoreFloat3(&pos, P);

						vertices.push_back(pos);
					}

					// Gather all indices for all subsets in LOD0:
					uint32_t first_subset = 0;
					uint32_t last_subset = 0;
					mesh->GetLODSubsetRange(0, first_subset, last_subset);
					for (uint32_t subsetIndex = first_subset; subsetIndex < last_subset; ++subsetIndex)
					{
						const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
						if (subset.indexCount == 0)
							continue;
						for (uint32_t i = 0; i < subset.indexCount; ++i)
						{
							uint32_t index = mesh->indices[subset.indexOffset + i];
							assert(index < mesh->vertex_positions.size());
							index += vertexOffset;
							assert(index < vertices.size());
							indices.push_back(index);
						}
					}

					vertexOffset = (uint32_t)vertices.size();
				}

				// Generate shadow indices for position-only stream:
				wi::vector<uint32_t> shadow_indices(indices.size());
				meshopt_generateShadowIndexBuffer(
					shadow_indices.data(), indices.data(), indices.size(),
					vertices.data(), vertices.size(), sizeof(XMFLOAT3), sizeof(XMFLOAT3)
				);

				// De-duplicate vertices based on shadow index buffer:
				wi::vector<unsigned int> remap(shadow_indices.size());
				const size_t vertex_count = meshopt_generateVertexRemap(
					remap.data(),
					shadow_indices.data(), shadow_indices.size(),
					vertices.data(), vertices.size(), sizeof(XMFLOAT3)
				);
				wi::vector<XMFLOAT3> remapped_vertices(vertex_count);
				wi::vector<uint32_t> remapped_indices(shadow_indices.size());
				meshopt_remapIndexBuffer(remapped_indices.data(), shadow_indices.data(), shadow_indices.size(), remap.data());
				meshopt_remapVertexBuffer(remapped_vertices.data(), vertices.data(), vertices.size() /*initial vertex count, not the one returned from meshopt_generateVertexRemap*/, sizeof(XMFLOAT3), remap.data());

				// Optimizations:
				meshopt_optimizeVertexCache(remapped_indices.data(), remapped_indices.data(), remapped_indices.size(), vertex_count);
				meshopt_optimizeVertexFetch(remapped_vertices.data(), remapped_indices.data(), remapped_indices.size(), remapped_vertices.data(), vertex_count, sizeof(XMFLOAT3));

				// Generate C++ header syntax:
				std::string str;
				str += "static const float3 vertices[" + std::to_string(remapped_vertices.size()) + "] = {\n";
				for (auto& pos : remapped_vertices)
				{
					str += "\tfloat3(" + std::to_string(pos.x) + "f," + std::to_string(pos.y) + "f," + std::to_string(pos.z) + "f),\n";
				}
				str += "};\n";
				str += "static const unsigned int indices[" + std::to_string(remapped_indices.size()) + "] = {\n";
				for (size_t i = 0; i < remapped_indices.size(); i += 3)
				{
					str += "\t" + std::to_string(remapped_indices[i + 0]) + "," + std::to_string(remapped_indices[i + 1]) + "," + std::to_string(remapped_indices[i + 2]) + ",\n";
				}
				str += "};\n";

				// Write to file:
				std::string filename_dest = wi::helper::ForceExtension(filename, "h");
				if (wi::helper::FileWrite(filename_dest, (uint8_t*)str.c_str(), str.length()))
				{
					editor->PostSaveText("Mesh exported to header file: ", filename_dest);
				}
				else
				{
					editor->PostSaveText("Failed to write file: ", filename_dest);
				}
			});
		});
	});
	AddWidget(&exportHeaderButton);


	subsetMaterialComboBox.Create("Material: ");
	subsetMaterialComboBox.SetEnabled(false);
	subsetMaterialComboBox.OnSelect([this](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		MeshComponent* mesh = scene.meshes.GetComponent(entity);
		if (mesh != nullptr && subset >= 0 && subset < mesh->subsets.size())
		{
			if (args.iValue == 0)
			{
				mesh->SetSubsetMaterial(uint32_t(subset), INVALID_ENTITY);
			}
			else
			{
				mesh->SetSubsetMaterial(uint32_t(subset), scene.materials.GetEntity(args.iValue - 1));
			}
		}
	});
	subsetMaterialComboBox.SetTooltip("Set the base material of the selected MeshSubset");
	AddWidget(&subsetMaterialComboBox);


	morphTargetCombo.Create("Morph Target:");
	morphTargetCombo.OnSelect([this](wi::gui::EventArgs args) {
		MeshComponent* mesh = editor->GetCurrentScene().meshes.GetComponent(entity);
		if (mesh != nullptr && args.iValue < (int)mesh->morph_targets.size())
		{
			morphTargetSlider.SetValue(mesh->morph_targets[args.iValue].weight);
		}
	});
	morphTargetCombo.SetTooltip("Choose a morph target to edit weight.");
	AddWidget(&morphTargetCombo);

	morphTargetSlider.Create(0, 1, 0, 100000, "Weight: ");
	morphTargetSlider.SetTooltip("Set the weight for morph target");
	morphTargetSlider.OnSlide(forEachSelected([this] (auto mesh, auto args) {
		if (morphTargetCombo.GetSelected() < (int)mesh->morph_targets.size())
			{
				mesh->morph_targets[morphTargetCombo.GetSelected()].weight = args.fValue;
			}
	}));
	AddWidget(&morphTargetSlider);

	lodgenButton.Create("LOD Gen");
	lodgenButton.SetTooltip("Generate LODs (levels of detail).");
	lodgenButton.OnClick([this, forEachSelected] (auto args) {
		forEachSelected([this] (auto mesh, auto args) {
			if (mesh->subsets_per_lod == 0)
			{
				// if there were no lods before, record the subset count without lods:
				mesh->subsets_per_lod = (uint32_t)mesh->subsets.size();
			}

			// https://github.com/zeux/meshoptimizer/blob/bedaaaf6e710d3b42d49260ca738c15d171b1a8f/demo/main.cpp
			size_t index_count = mesh->indices.size();
			size_t vertex_count = mesh->vertex_positions.size();

			const size_t lod_count = (size_t)lodCountSlider.GetValue();
			struct LOD
			{
				struct Subset
				{
					wi::vector<uint32_t> indices;
				};
				wi::vector<Subset> subsets;
			};
			wi::vector<LOD> lods(lod_count);

			const float target_error = lodErrorSlider.GetValue();

			for (size_t i = 0; i < lod_count; ++i)
			{
				lods[i].subsets.resize(mesh->subsets_per_lod);
				for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					lods[i].subsets[subsetIndex].indices.resize(subset.indexCount);
					for (uint32_t ind = 0; ind < subset.indexCount; ++ind)
					{
						lods[i].subsets[subsetIndex].indices[ind] = mesh->indices[subset.indexOffset + ind];
					}
				}
			}

			for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
			{
				const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];

				float threshold = wi::math::Lerp(0, 0.9f, saturate(lodQualitySlider.GetValue()));
				for (size_t i = 1; i < lod_count; ++i)
				{
					wi::vector<unsigned int>& lod = lods[i].subsets[subsetIndex].indices;

					size_t target_index_count = size_t(mesh->indices.size() * threshold) / 3 * 3;

					// we can simplify all the way from base level or from the last result
					// simplifying from the base level sometimes produces better results, but simplifying from last level is faster
					//const wi::vector<unsigned int>& source = lods[0].subsets[subsetIndex].indices;
					const wi::vector<unsigned int>& source = lods[i - 1].subsets[subsetIndex].indices;

					if (source.size() < target_index_count)
						target_index_count = source.size();

					lod.resize(source.size());
					if (lodSloppyCheckBox.GetCheck())
					{
						lod.resize(meshopt_simplifySloppy(&lod[0], &source[0], source.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), target_index_count, target_error));
					}
					else
					{
						lod.resize(meshopt_simplify(&lod[0], &source[0], source.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), target_index_count, target_error));
					}

					threshold *= threshold;
				}

				// optimize each individual LOD for vertex cache & overdraw
				for (size_t i = 0; i < lod_count; ++i)
				{
					wi::vector<unsigned int>& lod = lods[i].subsets[subsetIndex].indices;

					meshopt_optimizeVertexCache(&lod[0], &lod[0], lod.size(), mesh->vertex_positions.size());
					meshopt_optimizeOverdraw(&lod[0], &lod[0], lod.size(), &mesh->vertex_positions[0].x, mesh->vertex_positions.size(), sizeof(XMFLOAT3), 1.0f);
				}
			}

			mesh->indices.clear();
			wi::vector<MeshComponent::MeshSubset> subsets;
			for (size_t i = 0; i < lod_count; ++i)
			{
				for (uint32_t subsetIndex = 0; subsetIndex < mesh->subsets_per_lod; ++subsetIndex)
				{
					const MeshComponent::MeshSubset& subset = mesh->subsets[subsetIndex];
					subsets.emplace_back();
					subsets.back() = subset;
					subsets.back().indexOffset = (uint32_t)mesh->indices.size();
					subsets.back().indexCount = (uint32_t)lods[i].subsets[subsetIndex].indices.size();
					for (auto& x : lods[i].subsets[subsetIndex].indices)
					{
				 		mesh->indices.push_back(x);
					}
				}
			}
			mesh->subsets = subsets;

			mesh->CreateRenderData();

			if (mesh->IsBVHEnabled())
			{
				mesh->BuildBVH();
			}
		})(args);
		SetEntity(entity, subset);
	});
	AddWidget(&lodgenButton);

	lodCountSlider.Create(2, 10, 6, 8, "LOD Count: ");
	lodCountSlider.SetTooltip("This is how many levels of detail will be created.");
	AddWidget(&lodCountSlider);

	lodQualitySlider.Create(0.1f, 1.0f, 0.5f, 10000, "LOD Quality: ");
	lodQualitySlider.SetTooltip("Lower values will make LODs more agressively simplified.");
	AddWidget(&lodQualitySlider);

	lodErrorSlider.Create(0.01f, 0.1f, 0.03f, 10000, "LOD Error: ");
	lodErrorSlider.SetTooltip("Lower values will make more precise levels of detail.");
	AddWidget(&lodErrorSlider);

	lodSloppyCheckBox.Create("Sloppy LOD: ");
	lodSloppyCheckBox.SetTooltip("Use the sloppy simplification algorithm, which is faster but doesn't preserve shape well.");
	AddWidget(&lodSloppyCheckBox);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY, -1);
}

void MeshWindow::SetEntity(Entity entity, int subset)
{
	subset = std::max(0, subset);

	this->entity = entity;
	this->subset = subset;

	Scene& scene = editor->GetCurrentScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh != nullptr)
	{
		std::string ss;
		const NameComponent* name = scene.names.GetComponent(entity);
		if (name != nullptr)
		{
			ss += "Mesh name: " + name->name + "\n";
		}
		ss += "Vertex count: " + std::to_string(mesh->vertex_positions.size()) + "\n";
		ss += "Index count: " + std::to_string(mesh->indices.size()) + "\n";
		ss += "Index format: " + std::string(wi::graphics::GetIndexBufferFormatString(mesh->GetIndexFormat())) + "\n";
		ss += "Position format: " + std::string(wi::graphics::GetFormatString(mesh->position_format)) + "\n";
		ss += "Subset count: " + std::to_string(mesh->subsets.size()) + " (" + std::to_string(mesh->GetLODCount()) + " LODs)\n";
		if (!mesh->morph_targets.empty())
		{
			ss += "Morph target count: " + std::to_string(mesh->morph_targets.size()) + "\n";
		}
		if (!mesh->cluster_ranges.empty())
		{
			ss += "Cluster count: " + std::to_string(mesh->GetClusterCount()) + "\n";
		}
		ss += "CPU memory: " + wi::helper::GetMemorySizeText(mesh->GetMemoryUsageCPU()) + "\n";
		if (mesh->bvh.IsValid())
		{
			ss += "\tCPU BVH size: " + wi::helper::GetMemorySizeText(mesh->GetMemoryUsageBVH()) + "\n";
		}
		ss += "GPU memory: " + wi::helper::GetMemorySizeText(mesh->GetMemoryUsageGPU()) + "\n";
		if (!mesh->BLASes.empty())
		{
			size_t size = 0;
			for (auto& x : mesh->BLASes)
			{
				size += x.size;
			}
			ss += "\tBLAS size: " + wi::helper::GetMemorySizeText(size) + "\n";
		}
		ss += "\nVertex buffers:\n";
		if (!mesh->vertex_positions.empty()) ss += "\tposition;\n";
		if (!mesh->vertex_normals.empty()) ss += "\tnormal;\n";
		if (!mesh->vertex_windweights.empty()) ss += "\twind;\n";
		if (!mesh->vertex_uvset_0.empty()) ss += "\tuvset0;\n";
		if (!mesh->vertex_uvset_1.empty()) ss += "\tuvset1;\n";
		if (!mesh->vertex_atlas.empty()) ss += "\tatlas;\n";
		if (!mesh->vertex_colors.empty()) ss += "\tcolor;\n";
		if (!mesh->vertex_boneindices.empty()) ss += "\tbone 4 influence;\n";
		if (!mesh->vertex_boneindices2.empty()) ss += "\tbone 8 influence;\n";
		if (!mesh->vertex_tangents.empty()) ss += "\ttangent;\n";
		if (!mesh->vertex_windweights.empty()) ss += "\twind weights;\n";
		if (mesh->so_pos.IsValid()) ss += "\tstreamout_position;\n";
		if (mesh->so_nor.IsValid()) ss += "\tstreamout_normals;\n";
		if (mesh->so_tan.IsValid()) ss += "\tstreamout_tangents;\n";
		if (mesh->so_pre.IsValid()) ss += "\tprevious_position;\n";

		ss += "\nSuballocation offset: ";
		if (mesh->generalBufferOffsetAllocation.IsValid())
		{
			ss += wi::helper::GetMemorySizeText(mesh->generalBufferOffsetAllocation.byte_offset);
		}
		else
		{
			ss += "suballocation is not used for this mesh";
		}

		meshInfoLabel.SetText(ss);

		subsetComboBox.ClearItems();
		uint32_t main_subset_count = mesh->subsets_per_lod > 0 ? mesh->subsets_per_lod : (uint32_t)mesh->subsets.size();
		for (uint32_t i = 0; i < main_subset_count; ++i)
		{
			subsetComboBox.AddItem(std::to_string(i));
		}
		subsetComboBox.AddItem("[Create New] " + std::to_string(main_subset_count));

		subset = std::max(0, std::min(subset, (int)main_subset_count - 1));
		subsetComboBox.SetSelectedWithoutCallback(subset);

		if (!editor->translator.selected.empty())
		{
			editor->translator.selected.back().subsetIndex = subset;
		}

		subsetMaterialComboBox.ClearItems();
		subsetMaterialComboBox.AddItem("NO MATERIAL");
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			if (scene.materials[i].IsInternal())
			{
				continue;
			}

			Entity entity = scene.materials.GetEntity(i);

			if (scene.names.Contains(entity))
			{
				const NameComponent& name = *scene.names.GetComponent(entity);
				subsetMaterialComboBox.AddItem(name.name);
			}
			else
			{
				subsetMaterialComboBox.AddItem(std::to_string(entity));
			}

			if (subset >= 0 && subset < mesh->subsets.size() && mesh->subsets[subset].materialID == entity)
			{
				subsetMaterialComboBox.SetSelected((int)i + 1);
			}
		}

		doubleSidedCheckBox.SetCheck(mesh->IsDoubleSided());
		doubleSidedShadowCheckBox.SetCheck(mesh->IsDoubleSidedShadow());
		bvhCheckBox.SetCheck(mesh->bvh.IsValid());
		quantizeCheckBox.SetCheck(mesh->IsQuantizedPositionsDisabled());

		const ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostorCreateButton.SetText("Delete Impostor");
			impostorDistanceSlider.SetValue(impostor->swapInDistance);
		}
		else
		{
			impostorCreateButton.SetText("Create Impostor");
		}
		tessellationFactorSlider.SetValue(mesh->GetTessellationFactor());

		uint8_t selected = morphTargetCombo.GetSelected();
		morphTargetCombo.ClearItems();
		for (size_t i = 0; i < mesh->morph_targets.size(); i++)
		{
			morphTargetCombo.AddItem(std::to_string(i).c_str());
		}
		if (selected < mesh->morph_targets.size())
		{
			morphTargetCombo.SetSelected(selected);
		}
		SetEnabled(true);

		if (mesh->morph_targets.empty())
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

	mergeButton.SetEnabled(true);
}

void MeshWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 100;

	layout.add_fullwidth(meshInfoLabel);
	layout.add(subsetComboBox);
	subsetRemoveButton.SetPos(XMFLOAT2(subsetComboBox.GetPos().x + subsetComboBox.GetSize().x + 1 + subsetComboBox.GetSize().y, subsetComboBox.GetPos().y));
	subsetRemoveButton.SetSize(XMFLOAT2(subsetComboBox.GetSize().y, subsetComboBox.GetSize().y));
	layout.add(subsetMaterialComboBox);
	layout.add(subsetLastButton);
	layout.add_right(doubleSidedCheckBox);
	layout.add_right(doubleSidedShadowCheckBox);
	layout.add_right(bvhCheckBox);
	layout.add_right(quantizeCheckBox);
	layout.add_fullwidth(impostorCreateButton);
	layout.add(impostorDistanceSlider);
	layout.add(tessellationFactorSlider);
	layout.add_fullwidth(instanceSelectButton);
	layout.add_fullwidth(flipCullingButton);
	layout.add_fullwidth(flipNormalsButton);
	layout.add_fullwidth(computeNormalsSmoothButton);
	layout.add_fullwidth(computeNormalsHardButton);
	layout.add_fullwidth(recenterButton);
	layout.add_fullwidth(recenterToBottomButton);
	layout.add_fullwidth(mergeButton);
	layout.add_fullwidth(optimizeButton);
	layout.add_fullwidth(exportHeaderButton);

	layout.add(morphTargetCombo);
	layout.add(morphTargetSlider);

	layout.add_fullwidth(lodgenButton);
	layout.add(lodCountSlider);
	layout.add(lodQualitySlider);
	layout.add(lodErrorSlider);
	layout.add_right(lodSloppyCheckBox);
}
