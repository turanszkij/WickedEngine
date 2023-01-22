#include "stdafx.h"
#include "OptionsWindow.h"
#include "Editor.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void OptionsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Options", wi::gui::Window::WindowControls::RESIZE_TOPRIGHT);
	SetShadowRadius(2);

	newCombo.Create("New: ");
	newCombo.selected_font.anim.typewriter.looped = true;
	newCombo.selected_font.anim.typewriter.time = 2;
	newCombo.selected_font.anim.typewriter.character_start = 1;
	newCombo.SetInvalidSelectionText("...");
	newCombo.AddItem("Transform " ICON_TRANSFORM, 0);
	newCombo.AddItem("Material " ICON_MATERIAL, 1);
	newCombo.AddItem("Point Light " ICON_POINTLIGHT, 2);
	newCombo.AddItem("Spot Light " ICON_SPOTLIGHT, 3);
	newCombo.AddItem("Directional Light " ICON_DIRECTIONALLIGHT, 4);
	newCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, 5);
	newCombo.AddItem("Force " ICON_FORCE, 6);
	newCombo.AddItem("Decal " ICON_DECAL, 7);
	newCombo.AddItem("Sound " ICON_SOUND, 8);
	newCombo.AddItem("Weather " ICON_WEATHER, 9);
	newCombo.AddItem("Emitter " ICON_EMITTER, 10);
	newCombo.AddItem("HairParticle " ICON_HAIR, 11);
	newCombo.AddItem("Camera " ICON_CAMERA, 12);
	newCombo.AddItem("Cube " ICON_CUBE, 13);
	newCombo.AddItem("Plane " ICON_SQUARE, 14);
	newCombo.AddItem("Animation " ICON_ANIMATION, 15);
	newCombo.AddItem("Script " ICON_SCRIPT, 16);
	newCombo.AddItem("Collider " ICON_COLLIDER, 17);
	newCombo.AddItem("Terrain " ICON_TERRAIN, 18);
	newCombo.OnSelect([&](wi::gui::EventArgs args) {
		newCombo.SetSelectedWithoutCallback(-1);
		const EditorComponent::EditorScene& editorscene = editor->GetCurrentEditorScene();
		const CameraComponent& camera = editorscene.camera;
		Scene& scene = editor->GetCurrentScene();
		PickResult pick;

		XMFLOAT3 in_front_of_camera;
		XMStoreFloat3(&in_front_of_camera, XMVectorAdd(camera.GetEye(), camera.GetAt() * 4));

		switch (args.userdata)
		{
		case 0:
			pick.entity = scene.Entity_CreateTransform("transform");
			break;
		case 1:
			pick.entity = scene.Entity_CreateMaterial("material");
			break;
		case 2:
			pick.entity = scene.Entity_CreateLight("pointlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::POINT;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			break;
		case 3:
			pick.entity = scene.Entity_CreateLight("spotlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::SPOT;
			scene.lights.GetComponent(pick.entity)->intensity = 100;
			break;
		case 4:
			pick.entity = scene.Entity_CreateLight("dirlight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::DIRECTIONAL;
			scene.lights.GetComponent(pick.entity)->intensity = 10;
			break;
		case 5:
			pick.entity = scene.Entity_CreateEnvironmentProbe("envprobe", in_front_of_camera);
			break;
		case 6:
			pick.entity = scene.Entity_CreateForce("force");
			break;
		case 7:
			pick.entity = scene.Entity_CreateDecal("decal", "images/logo_small.png");
			scene.transforms.GetComponent(pick.entity)->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
			break;
		case 8:
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Sound";
			params.extensions = wi::resourcemanager::GetSupportedSoundExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Entity entity = editor->GetCurrentScene().Entity_CreateSound(wi::helper::GetFileNameFromPath(fileName), fileName);

					wi::Archive& archive = editor->AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					editor->RecordSelection(archive);

					editor->ClearSelected();
					editor->AddSelected(entity);

					editor->RecordSelection(archive);
					editor->RecordEntity(archive, entity);

					RefreshEntityTree();
					editor->componentsWnd.soundWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case 9:
			pick.entity = CreateEntity();
			scene.weathers.Create(pick.entity);
			scene.names.Create(pick.entity) = "weather";
			break;
		case 10:
			pick.entity = scene.Entity_CreateEmitter("emitter");
			break;
		case 11:
			pick.entity = scene.Entity_CreateHair("hair");
			break;
		case 12:
			pick.entity = scene.Entity_CreateCamera("camera", camera.width, camera.height);
			*scene.cameras.GetComponent(pick.entity) = camera;
			*scene.transforms.GetComponent(pick.entity) = editorscene.camera_transform;
			break;
		case 13:
			pick.entity = scene.Entity_CreateCube("cube");
			pick.subsetIndex = 0;
			break;
		case 14:
			pick.entity = scene.Entity_CreatePlane("plane");
			pick.subsetIndex = 0;
			break;
		case 15:
			pick.entity = CreateEntity();
			scene.animations.Create(pick.entity);
			scene.names.Create(pick.entity) = "animation";
			break;
		case 16:
			pick.entity = CreateEntity();
			scene.scripts.Create(pick.entity);
			scene.names.Create(pick.entity) = "script";
			break;
		case 17:
			pick.entity = CreateEntity();
			scene.colliders.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "collider";
			break;
		case 18:
			editor->componentsWnd.terrainWnd.SetupAssets();
			pick.entity = CreateEntity();
			scene.terrains.Create(pick.entity) = editor->componentsWnd.terrainWnd.terrain_preset;
			scene.names.Create(pick.entity) = "terrain";
			break;
		default:
			break;
		}
		if (pick.entity != INVALID_ENTITY)
		{
			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_ADD;
			editor->RecordSelection(archive);

			editor->ClearSelected();
			editor->AddSelected(pick);

			editor->RecordSelection(archive);
			editor->RecordEntity(archive, pick.entity);
		}
		RefreshEntityTree();
		});
	newCombo.SetEnabled(true);
	newCombo.SetTooltip("Create new entity");
	AddWidget(&newCombo);





	filterCombo.Create("");
	filterCombo.AddItem("*", (uint64_t)Filter::All);
	filterCombo.AddItem(ICON_TRANSFORM, (uint64_t)Filter::Transform);
	filterCombo.AddItem(ICON_MATERIAL, (uint64_t)Filter::Material);
	filterCombo.AddItem(ICON_MESH, (uint64_t)Filter::Mesh);
	filterCombo.AddItem(ICON_OBJECT, (uint64_t)Filter::Object);
	filterCombo.AddItem(ICON_ENVIRONMENTPROBE, (uint64_t)Filter::EnvironmentProbe);
	filterCombo.AddItem(ICON_DECAL, (uint64_t)Filter::Decal);
	filterCombo.AddItem(ICON_SOUND, (uint64_t)Filter::Sound);
	filterCombo.AddItem(ICON_WEATHER, (uint64_t)Filter::Weather);
	filterCombo.AddItem(ICON_POINTLIGHT, (uint64_t)Filter::Light);
	filterCombo.AddItem(ICON_ANIMATION, (uint64_t)Filter::Animation);
	filterCombo.AddItem(ICON_FORCE, (uint64_t)Filter::Force);
	filterCombo.AddItem(ICON_EMITTER, (uint64_t)Filter::Emitter);
	filterCombo.AddItem(ICON_HAIR, (uint64_t)Filter::Hairparticle);
	filterCombo.AddItem(ICON_IK, (uint64_t)Filter::IK);
	filterCombo.AddItem(ICON_CAMERA, (uint64_t)Filter::Camera);
	filterCombo.AddItem(ICON_ARMATURE, (uint64_t)Filter::Armature);
	filterCombo.AddItem(ICON_SPRING, (uint64_t)Filter::Spring);
	filterCombo.AddItem(ICON_COLLIDER, (uint64_t)Filter::Collider);
	filterCombo.AddItem(ICON_SCRIPT, (uint64_t)Filter::Script);
	filterCombo.AddItem(ICON_EXPRESSION, (uint64_t)Filter::Expression);
	filterCombo.AddItem(ICON_HUMANOID, (uint64_t)Filter::Humanoid);
	filterCombo.AddItem(ICON_TERRAIN, (uint64_t)Filter::Terrain);
	filterCombo.SetTooltip("Apply filtering to the Entities by components");
	filterCombo.OnSelect([&](wi::gui::EventArgs args) {
		filter = (Filter)args.userdata;
		RefreshEntityTree();
		});
	AddWidget(&filterCombo);


	filterInput.Create("");
	filterInput.SetTooltip("Apply filtering to the Entities by name");
	filterInput.SetDescription(ICON_FILTER ": ");
	filterInput.SetCancelInputEnabled(false);
	filterInput.OnInput([=](wi::gui::EventArgs args) {
		RefreshEntityTree();
	});
	AddWidget(&filterInput);

	filterCaseCheckBox.Create("");
	filterCaseCheckBox.SetCheckText("Aa");
	filterCaseCheckBox.SetUnCheckText("a");
	filterCaseCheckBox.SetTooltip("Toggle case-sensitive name filtering");
	filterCaseCheckBox.OnClick([=](wi::gui::EventArgs args) {
		RefreshEntityTree();
	});
	AddWidget(&filterCaseCheckBox);


	entityTree.Create("Entities");
	entityTree.SetSize(XMFLOAT2(300, 300));
	entityTree.OnSelect([this](wi::gui::EventArgs args) {

		if (args.iValue < 0)
			return;

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_SELECTION;
		// record PREVIOUS selection state...
		editor->RecordSelection(archive);

		editor->translator.selected.clear();

		for (int i = 0; i < entityTree.GetItemCount(); ++i)
		{
			const wi::gui::TreeList::Item& item = entityTree.GetItem(i);
			if (item.selected)
			{
				wi::scene::PickResult pick;
				pick.entity = (Entity)item.userdata;
				editor->AddSelected(pick);
			}
		}

		// record NEW selection state...
		editor->RecordSelection(archive);

		});
	entityTree.OnDelete([=](wi::gui::EventArgs args) {
		// Deletions will be performed in a batch next frame:
		//	We don't delete here, because this callback will execute once for each item
		editor->deleting = true;
	});
	AddWidget(&entityTree);

	graphicsWnd.Create(editor);
	graphicsWnd.SetCollapsed(true);
	AddWidget(&graphicsWnd);

	cameraWnd.Create(editor);
	cameraWnd.SetCollapsed(true);
	AddWidget(&cameraWnd);

	paintToolWnd.Create(editor);
	paintToolWnd.SetCollapsed(true);
	AddWidget(&paintToolWnd);

	materialPickerWnd.Create(editor);
	AddWidget(&materialPickerWnd);


	generalWnd.Create(editor);
	generalWnd.SetCollapsed(true);
	AddWidget(&generalWnd);


	XMFLOAT2 size = XMFLOAT2(338, 500);
	if (editor->main->config.GetSection("layout").Has("options.width"))
	{
		size.x = editor->main->config.GetSection("layout").GetFloat("options.width");
	}
	if (editor->main->config.GetSection("layout").Has("options.height"))
	{
		size.y = editor->main->config.GetSection("layout").GetFloat("options.height");
	}
	SetSize(size);
}
void OptionsWindow::Update(float dt)
{
	cameraWnd.Update();
	paintToolWnd.Update(dt);
	graphicsWnd.Update();
}

void OptionsWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	XMFLOAT2 pos = XMFLOAT2(padding, padding);
	const float width = GetWidgetAreaSize().x - padding * 2;
	float x_off = 100;
	editor->main->config.GetSection("layout").Set("options.width", GetSize().x);
	editor->main->config.GetSection("layout").Set("options.height", GetSize().y);

	generalWnd.SetPos(pos);
	generalWnd.SetSize(XMFLOAT2(width, generalWnd.GetScale().y));
	pos.y += generalWnd.GetSize().y;
	pos.y += padding;

	graphicsWnd.SetPos(pos);
	graphicsWnd.SetSize(XMFLOAT2(width, graphicsWnd.GetScale().y));
	pos.y += graphicsWnd.GetSize().y;
	pos.y += padding;

	cameraWnd.SetPos(pos);
	cameraWnd.SetSize(XMFLOAT2(width, cameraWnd.GetScale().y));
	pos.y += cameraWnd.GetSize().y;
	pos.y += padding;

	materialPickerWnd.SetPos(pos);
	materialPickerWnd.SetSize(XMFLOAT2(width, materialPickerWnd.GetScale().y));
	pos.y += materialPickerWnd.GetSize().y;
	pos.y += padding;

	paintToolWnd.SetPos(pos);
	paintToolWnd.SetSize(XMFLOAT2(width, paintToolWnd.GetScale().y));
	pos.y += paintToolWnd.GetSize().y;
	pos.y += padding;

	x_off = 45;

	newCombo.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	newCombo.SetSize(XMFLOAT2(width - x_off - newCombo.GetScale().y - 1, newCombo.GetScale().y));
	pos.y += newCombo.GetSize().y;
	pos.y += padding;



	float filterHeight = filterCombo.GetSize().y;
	float filterComboWidth = 30;

	filterInput.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
	filterInput.SetSize(XMFLOAT2(width - x_off - filterHeight - 5 - filterComboWidth - filterHeight, filterCombo.GetScale().y));

	filterCaseCheckBox.SetPos(XMFLOAT2(filterInput.GetPos().x + filterInput.GetSize().x + 2, pos.y));
	filterCaseCheckBox.SetSize(XMFLOAT2(filterHeight, filterHeight));

	filterCombo.SetPos(XMFLOAT2(filterCaseCheckBox.GetPos().x + filterCaseCheckBox.GetSize().x + 2, pos.y));
	filterCombo.SetSize(XMFLOAT2(filterComboWidth, filterHeight));
	pos.y += filterCombo.GetSize().y;
	pos.y += padding;



	entityTree.SetPos(pos);
	entityTree.SetSize(XMFLOAT2(width, std::max(editor->GetLogicalHeight() * 0.75f, editor->GetLogicalHeight() - pos.y)));
	pos.y += entityTree.GetSize().y;
	pos.y += padding;
}


void OptionsWindow::PushToEntityTree(wi::ecs::Entity entity, int level)
{
	if (entitytree_added_items.count(entity) != 0)
	{
		return;
	}
	const Scene& scene = editor->GetCurrentScene();

	wi::gui::TreeList::Item item;
	item.level = level;
	item.userdata = entity;
	item.selected = editor->IsSelected(entity);
	item.open = entitytree_opened_items.count(entity) != 0;

	const NameComponent* name = scene.names.GetComponent(entity);

	std::string name_string;
	if (name == nullptr)
	{
		name_string = "[no_name] " + std::to_string(entity);
	}
	else if (name->name.empty())
	{
		name_string = "[name_empty] " + std::to_string(entity);
	}
	else
	{
		name_string = name->name;
	}

	std::string name_filter = filterInput.GetCurrentInputValue();
	if (!name_filter.empty())
	{
		if (filterCaseCheckBox.GetCheck() && name_string.find(name_filter) == std::string::npos)
		{
			return;
		}
		else if (wi::helper::toUpper(name_string).find(wi::helper::toUpper(name_filter)) == std::string::npos)
		{
			return;
		}
	}

	// Icons:
	if (scene.layers.Contains(entity))
	{
		item.name += ICON_LAYER " ";
	}
	if (scene.transforms.Contains(entity))
	{
		item.name += ICON_TRANSFORM " ";
	}
	if (scene.terrains.Contains(entity))
	{
		item.name += ICON_TERRAIN " ";
	}
	if (scene.meshes.Contains(entity))
	{
		item.name += ICON_MESH " ";
	}
	if (scene.objects.Contains(entity))
	{
		item.name += ICON_OBJECT " ";
	}
	if (scene.rigidbodies.Contains(entity))
	{
		item.name += ICON_RIGIDBODY " ";
	}
	if (scene.softbodies.Contains(entity))
	{
		item.name += ICON_SOFTBODY " ";
	}
	if (scene.emitters.Contains(entity))
	{
		item.name += ICON_EMITTER " ";
	}
	if (scene.hairs.Contains(entity))
	{
		item.name += ICON_HAIR " ";
	}
	if (scene.forces.Contains(entity))
	{
		item.name += ICON_FORCE " ";
	}
	if (scene.sounds.Contains(entity))
	{
		item.name += ICON_SOUND " ";
	}
	if (scene.decals.Contains(entity))
	{
		item.name += ICON_DECAL " ";
	}
	if (scene.cameras.Contains(entity))
	{
		item.name += ICON_CAMERA " ";
	}
	if (scene.probes.Contains(entity))
	{
		item.name += ICON_ENVIRONMENTPROBE " ";
	}
	if (scene.animations.Contains(entity))
	{
		item.name += ICON_ANIMATION " ";
	}
	if (scene.animation_datas.Contains(entity))
	{
		item.name += "[animation_data] ";
	}
	if (scene.armatures.Contains(entity))
	{
		item.name += ICON_ARMATURE " ";
	}
	if (scene.humanoids.Contains(entity))
	{
		item.name += ICON_HUMANOID " ";
	}
	if (scene.lights.Contains(entity))
	{
		const LightComponent* light = scene.lights.GetComponent(entity);
		switch (light->type)
		{
		default:
		case LightComponent::POINT:
			item.name += ICON_POINTLIGHT " ";
			break;
		case LightComponent::SPOT:
			item.name += ICON_SPOTLIGHT " ";
			break;
		case LightComponent::DIRECTIONAL:
			item.name += ICON_DIRECTIONALLIGHT " ";
			break;
		}
	}
	if (scene.materials.Contains(entity))
	{
		item.name += ICON_MATERIAL " ";
	}
	if (scene.weathers.Contains(entity))
	{
		item.name += ICON_WEATHER " ";
	}
	if (scene.inverse_kinematics.Contains(entity))
	{
		item.name += ICON_IK " ";
	}
	if (scene.springs.Contains(entity))
	{
		item.name += ICON_SPRING " ";
	}
	if (scene.colliders.Contains(entity))
	{
		item.name += ICON_COLLIDER " ";
	}
	if (scene.scripts.Contains(entity))
	{
		item.name += ICON_SCRIPT " ";
	}
	if (scene.expressions.Contains(entity))
	{
		item.name += ICON_EXPRESSION " ";
	}
	bool bone_found = false;
	for (size_t i = 0; i < scene.armatures.GetCount() && !bone_found; ++i)
	{
		const ArmatureComponent& armature = scene.armatures[i];
		for (Entity bone : armature.boneCollection)
		{
			if (entity == bone)
			{
				item.name += ICON_BONE " ";
				bone_found = true;
				break;
			}
		}
	}

	item.name += name_string;
	entityTree.AddItem(item);

	entitytree_added_items.insert(entity);

	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		if (scene.hierarchy[i].parentID == entity)
		{
			PushToEntityTree(scene.hierarchy.GetEntity(i), level + 1);
		}
	}
}
void OptionsWindow::RefreshEntityTree()
{
	const Scene& scene = editor->GetCurrentScene();
	materialPickerWnd.RecreateButtons();

	for (int i = 0; i < entityTree.GetItemCount(); ++i)
	{
		const wi::gui::TreeList::Item& item = entityTree.GetItem(i);
		if (item.open)
		{
			entitytree_opened_items.insert((Entity)item.userdata);
		}
	}

	entityTree.ClearItems();

	if (has_flag(filter, Filter::All))
	{
		// Add hierarchy:
		for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
		{
			if (scene.hierarchy[i].parentID == INVALID_ENTITY)
				continue;
			PushToEntityTree(scene.hierarchy[i].parentID, 0);
		}
	}

	// Add any left over entities that might not have had a hierarchy:

	if (has_flag(filter, Filter::Terrain))
	{
		// Any transform left that is not part of a hierarchy:
		for (size_t i = 0; i < scene.terrains.GetCount(); ++i)
		{
			PushToEntityTree(scene.terrains.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Transform))
	{
		// Any transform left that is not part of a hierarchy:
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			PushToEntityTree(scene.transforms.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Light))
	{
		for (size_t i = 0; i < scene.lights.GetCount(); ++i)
		{
			PushToEntityTree(scene.lights.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Decal))
	{
		for (size_t i = 0; i < scene.decals.GetCount(); ++i)
		{
			PushToEntityTree(scene.decals.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Camera))
	{
		for (size_t i = 0; i < scene.cameras.GetCount(); ++i)
		{
			PushToEntityTree(scene.cameras.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Material))
	{
		for (size_t i = 0; i < scene.materials.GetCount(); ++i)
		{
			PushToEntityTree(scene.materials.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Mesh))
	{
		for (size_t i = 0; i < scene.meshes.GetCount(); ++i)
		{
			PushToEntityTree(scene.meshes.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Armature))
	{
		for (size_t i = 0; i < scene.armatures.GetCount(); ++i)
		{
			PushToEntityTree(scene.armatures.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Object))
	{
		for (size_t i = 0; i < scene.objects.GetCount(); ++i)
		{
			PushToEntityTree(scene.objects.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Weather))
	{
		for (size_t i = 0; i < scene.weathers.GetCount(); ++i)
		{
			PushToEntityTree(scene.weathers.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Sound))
	{
		for (size_t i = 0; i < scene.sounds.GetCount(); ++i)
		{
			PushToEntityTree(scene.sounds.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Hairparticle))
	{
		for (size_t i = 0; i < scene.hairs.GetCount(); ++i)
		{
			PushToEntityTree(scene.hairs.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Emitter))
	{
		for (size_t i = 0; i < scene.emitters.GetCount(); ++i)
		{
			PushToEntityTree(scene.emitters.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Animation))
	{
		for (size_t i = 0; i < scene.animations.GetCount(); ++i)
		{
			PushToEntityTree(scene.animations.GetEntity(i), 0);
		}
		for (size_t i = 0; i < scene.animation_datas.GetCount(); ++i)
		{
			PushToEntityTree(scene.animation_datas.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::EnvironmentProbe))
	{
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			PushToEntityTree(scene.probes.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Force))
	{
		for (size_t i = 0; i < scene.forces.GetCount(); ++i)
		{
			PushToEntityTree(scene.forces.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.rigidbodies.GetCount(); ++i)
		{
			PushToEntityTree(scene.rigidbodies.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.softbodies.GetCount(); ++i)
		{
			PushToEntityTree(scene.softbodies.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Spring))
	{
		for (size_t i = 0; i < scene.springs.GetCount(); ++i)
		{
			PushToEntityTree(scene.springs.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Collider))
	{
		for (size_t i = 0; i < scene.colliders.GetCount(); ++i)
		{
			PushToEntityTree(scene.colliders.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::IK))
	{
		for (size_t i = 0; i < scene.inverse_kinematics.GetCount(); ++i)
		{
			PushToEntityTree(scene.inverse_kinematics.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::All))
	{
		for (size_t i = 0; i < scene.names.GetCount(); ++i)
		{
			PushToEntityTree(scene.names.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Script))
	{
		for (size_t i = 0; i < scene.scripts.GetCount(); ++i)
		{
			PushToEntityTree(scene.scripts.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Humanoid))
	{
		for (size_t i = 0; i < scene.humanoids.GetCount(); ++i)
		{
			PushToEntityTree(scene.humanoids.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Expression))
	{
		for (size_t i = 0; i < scene.expressions.GetCount(); ++i)
		{
			PushToEntityTree(scene.expressions.GetEntity(i), 0);
		}
	}

	if (has_flag(filter, Filter::Terrain))
	{
		for (size_t i = 0; i < scene.terrains.GetCount(); ++i)
		{
			PushToEntityTree(scene.terrains.GetEntity(i), 0);
		}
	}

	entitytree_added_items.clear();
	entitytree_opened_items.clear();
}
