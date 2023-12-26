#include "stdafx.h"
#include "OptionsWindow.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

void OptionsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Options", wi::gui::Window::WindowControls::RESIZE_TOPRIGHT);
	SetShadowRadius(2);

	enum NEW_THING
	{
		NEW_TRANSFORM,
		NEW_MATERIAL,
		NEW_POINTLIGHT,
		NEW_SPOTLIGHT,
		NEW_DIRECTIONALLIGHT,
		NEW_ENVIRONMENTPROBE,
		NEW_FORCE,
		NEW_DECAL,
		NEW_SOUND,
		NEW_VIDEO,
		NEW_WEATHER,
		NEW_EMITTER,
		NEW_HAIR,
		NEW_CAMERA,
		NEW_CUBE,
		NEW_PLANE,
		NEW_SPHERE,
		NEW_ANIMATION,
		NEW_SCRIPT,
		NEW_COLLIDER,
		NEW_TERRAIN,
		NEW_SPRITE,
		NEW_FONT,
	};

	newCombo.Create("New: ");
	newCombo.selected_font.anim.typewriter.looped = true;
	newCombo.selected_font.anim.typewriter.time = 2;
	newCombo.selected_font.anim.typewriter.character_start = 1;
	newCombo.SetInvalidSelectionText("...");
	newCombo.AddItem("Transform " ICON_TRANSFORM, NEW_TRANSFORM);
	newCombo.AddItem("Material " ICON_MATERIAL, NEW_MATERIAL);
	newCombo.AddItem("Point Light " ICON_POINTLIGHT, NEW_POINTLIGHT);
	newCombo.AddItem("Spot Light " ICON_SPOTLIGHT, NEW_SPOTLIGHT);
	newCombo.AddItem("Directional Light " ICON_DIRECTIONALLIGHT, NEW_DIRECTIONALLIGHT);
	newCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, NEW_ENVIRONMENTPROBE);
	newCombo.AddItem("Force " ICON_FORCE, NEW_FORCE);
	newCombo.AddItem("Decal " ICON_DECAL, NEW_DECAL);
	newCombo.AddItem("Sound " ICON_SOUND, NEW_SOUND);
	newCombo.AddItem("Video " ICON_VIDEO, NEW_VIDEO);
	newCombo.AddItem("Weather " ICON_WEATHER, NEW_WEATHER);
	newCombo.AddItem("Emitter " ICON_EMITTER, NEW_EMITTER);
	newCombo.AddItem("HairParticle " ICON_HAIR, NEW_HAIR);
	newCombo.AddItem("Camera " ICON_CAMERA, NEW_CAMERA);
	newCombo.AddItem("Cube " ICON_CUBE, NEW_CUBE);
	newCombo.AddItem("Plane " ICON_SQUARE, NEW_PLANE);
	newCombo.AddItem("Sphere " ICON_CIRCLE, NEW_SPHERE);
	newCombo.AddItem("Animation " ICON_ANIMATION, NEW_ANIMATION);
	newCombo.AddItem("Script " ICON_SCRIPT, NEW_SCRIPT);
	newCombo.AddItem("Collider " ICON_COLLIDER, NEW_COLLIDER);
	newCombo.AddItem("Terrain " ICON_TERRAIN, NEW_TERRAIN);
	newCombo.AddItem("Sprite " ICON_SPRITE, NEW_SPRITE);
	newCombo.AddItem("Font " ICON_FONT, NEW_FONT);
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
		case NEW_TRANSFORM:
			pick.entity = scene.Entity_CreateTransform("transform");
			break;
		case NEW_MATERIAL:
			pick.entity = scene.Entity_CreateMaterial("material");
			break;
		case NEW_POINTLIGHT:
			pick.entity = scene.Entity_CreateLight("pointlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::POINT;
			scene.lights.GetComponent(pick.entity)->intensity = 20;
			break;
		case NEW_SPOTLIGHT:
			pick.entity = scene.Entity_CreateLight("spotlight", in_front_of_camera, XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::SPOT;
			scene.lights.GetComponent(pick.entity)->intensity = 100;
			break;
		case NEW_DIRECTIONALLIGHT:
			pick.entity = scene.Entity_CreateLight("dirlight", XMFLOAT3(0, 3, 0), XMFLOAT3(1, 1, 1), 2, 60);
			scene.lights.GetComponent(pick.entity)->type = LightComponent::DIRECTIONAL;
			scene.lights.GetComponent(pick.entity)->intensity = 10;
			break;
		case NEW_ENVIRONMENTPROBE:
			pick.entity = scene.Entity_CreateEnvironmentProbe("envprobe", in_front_of_camera);
			break;
		case NEW_FORCE:
			pick.entity = scene.Entity_CreateForce("force");
			break;
		case NEW_DECAL:
			pick.entity = scene.Entity_CreateDecal("decal", "");
			if (scene.materials.Contains(pick.entity))
			{
				MaterialComponent* decal_material = scene.materials.GetComponent(pick.entity);
				decal_material->textures[MaterialComponent::BASECOLORMAP].resource.SetTexture(*wi::texturehelper::getLogo());
			}
			scene.transforms.GetComponent(pick.entity)->RotateRollPitchYaw(XMFLOAT3(XM_PIDIV2, 0, 0));
			break;
		case NEW_SOUND:
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
		case NEW_VIDEO:
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Video";
			params.extensions = wi::resourcemanager::GetSupportedVideoExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					Entity entity = editor->GetCurrentScene().Entity_CreateVideo(wi::helper::GetFileNameFromPath(fileName), fileName);

					wi::Archive& archive = editor->AdvanceHistory();
					archive << EditorComponent::HISTORYOP_ADD;
					editor->RecordSelection(archive);

					editor->ClearSelected();
					editor->AddSelected(entity);

					editor->RecordSelection(archive);
					editor->RecordEntity(archive, entity);

					RefreshEntityTree();
					editor->componentsWnd.videoWnd.SetEntity(entity);
					});
				});
			return;
		}
		break;
		case NEW_WEATHER:
			pick.entity = CreateEntity();
			scene.weathers.Create(pick.entity);
			scene.names.Create(pick.entity) = "weather";
			break;
		case NEW_EMITTER:
			pick.entity = scene.Entity_CreateEmitter("emitter");
			break;
		case NEW_HAIR:
			pick.entity = scene.Entity_CreateHair("hair");
			break;
		case NEW_CAMERA:
			pick.entity = scene.Entity_CreateCamera("camera", camera.width, camera.height);
			*scene.cameras.GetComponent(pick.entity) = camera;
			*scene.transforms.GetComponent(pick.entity) = editorscene.camera_transform;
			break;
		case NEW_CUBE:
			pick.entity = scene.Entity_CreateCube("cube");
			pick.subsetIndex = 0;
			break;
		case NEW_PLANE:
			pick.entity = scene.Entity_CreatePlane("plane");
			pick.subsetIndex = 0;
			break;
		case NEW_SPHERE:
			pick.entity = scene.Entity_CreateSphere("sphere");
			pick.subsetIndex = 0;
			break;
		case NEW_ANIMATION:
			pick.entity = CreateEntity();
			scene.animations.Create(pick.entity);
			scene.names.Create(pick.entity) = "animation";
			break;
		case NEW_SCRIPT:
			pick.entity = CreateEntity();
			scene.scripts.Create(pick.entity);
			scene.names.Create(pick.entity) = "script";
			break;
		case NEW_COLLIDER:
			pick.entity = CreateEntity();
			scene.colliders.Create(pick.entity);
			scene.transforms.Create(pick.entity);
			scene.names.Create(pick.entity) = "collider";
			break;
		case NEW_TERRAIN:
			editor->componentsWnd.terrainWnd.SetupAssets();
			pick.entity = CreateEntity();
			scene.terrains.Create(pick.entity) = editor->componentsWnd.terrainWnd.terrain_preset;
			scene.names.Create(pick.entity) = "terrain";
			break;
		case NEW_SPRITE:
		{
			pick.entity = CreateEntity();
			wi::Sprite& sprite = scene.sprites.Create(pick.entity);
			sprite.params.pivot = XMFLOAT2(0.5f, 0.5f);
			sprite.anim.repeatable = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "sprite";
		}
		break;
		case NEW_FONT:
		{
			pick.entity = CreateEntity();
			wi::SpriteFont& font = scene.fonts.Create(pick.entity);
			font.SetText("Text");
			font.params.h_align = wi::font::Alignment::WIFALIGN_CENTER;
			font.params.v_align = wi::font::Alignment::WIFALIGN_CENTER;
			font.params.scaling = 0.1f;
			font.params.size = 26;
			font.anim.typewriter.looped = true;
			scene.transforms.Create(pick.entity).Translate(XMFLOAT3(0, 2, 0));
			scene.names.Create(pick.entity) = "font";
		}
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
	filterCombo.AddItem(ICON_VIDEO, (uint64_t)Filter::Video);
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
	filterCombo.AddItem(ICON_SPRITE, (uint64_t)Filter::Sprite);
	filterCombo.AddItem(ICON_FONT, (uint64_t)Filter::Font);
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
	entityTree.SetSize(XMFLOAT2(width, editor->GetLogicalHeight() - pos.y - this->translation_local.y - this->control_size - padding));
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

	if (CheckEntityFilter(entity))
	{
		wi::gui::TreeList::Item item;
		if (filter == Filter::All)
		{
			item.level = level;
		}
		else
		{
			item.level = 0;
		}
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
		if (scene.videos.Contains(entity))
		{
			item.name += ICON_VIDEO " ";
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
		if (scene.sprites.Contains(entity))
		{
			item.name += ICON_SPRITE " ";
		}
		if (scene.fonts.Contains(entity))
		{
			item.name += ICON_FONT " ";
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
	}

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

	entitytree_temp_items.clear();
	scene.FindAllEntities(entitytree_temp_items);

	// Add items to level 0 that are not in hierarchy (not in hierarchy can also mean top level parent):
	//	Note that PushToEntityTree will add children recursively, so this is all we need
	for (auto& x : entitytree_temp_items)
	{
		if (!scene.hierarchy.Contains(x))
		{
			PushToEntityTree(x, 0);
		}
	}

	entitytree_added_items.clear();
	entitytree_opened_items.clear();
}

bool OptionsWindow::CheckEntityFilter(wi::ecs::Entity entity)
{
	if (filter == Filter::All)
		return true;

	const Scene& scene = editor->GetCurrentScene();
	bool valid = false;

	if (
		has_flag(filter, Filter::Transform) && scene.transforms.Contains(entity) ||
		has_flag(filter, Filter::Material) && scene.materials.Contains(entity) ||
		has_flag(filter, Filter::Mesh) && scene.meshes.Contains(entity) ||
		has_flag(filter, Filter::Object) && scene.objects.Contains(entity) ||
		has_flag(filter, Filter::EnvironmentProbe) && scene.probes.Contains(entity) ||
		has_flag(filter, Filter::Decal) && scene.decals.Contains(entity) ||
		has_flag(filter, Filter::Sound) && scene.sounds.Contains(entity) ||
		has_flag(filter, Filter::Weather) && scene.weathers.Contains(entity) ||
		has_flag(filter, Filter::Light) && scene.lights.Contains(entity) ||
		has_flag(filter, Filter::Animation) && scene.animations.Contains(entity) ||
		has_flag(filter, Filter::Force) && scene.forces.Contains(entity) ||
		has_flag(filter, Filter::Emitter) && scene.emitters.Contains(entity) ||
		has_flag(filter, Filter::IK) && scene.inverse_kinematics.Contains(entity) ||
		has_flag(filter, Filter::Camera) && scene.cameras.Contains(entity) ||
		has_flag(filter, Filter::Armature) && scene.armatures.Contains(entity) ||
		has_flag(filter, Filter::Collider) && scene.colliders.Contains(entity) ||
		has_flag(filter, Filter::Script) && scene.scripts.Contains(entity) ||
		has_flag(filter, Filter::Expression) && scene.expressions.Contains(entity) ||
		has_flag(filter, Filter::Terrain) && scene.terrains.Contains(entity) ||
		has_flag(filter, Filter::Spring) && scene.springs.Contains(entity) ||
		has_flag(filter, Filter::Humanoid) && scene.humanoids.Contains(entity) ||
		has_flag(filter, Filter::Video) && scene.videos.Contains(entity) ||
		has_flag(filter, Filter::Sprite) && scene.sprites.Contains(entity) ||
		has_flag(filter, Filter::Font) && scene.fonts.Contains(entity)
		)
	{
		valid = true;
	}

	return valid;
}
