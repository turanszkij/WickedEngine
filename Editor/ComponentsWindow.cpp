#include "stdafx.h"
#include "ComponentsWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void ComponentsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Components ", wi::gui::Window::WindowControls::RESIZE_LEFT);
	SetText("Entity - Component System");
	font.params.h_align = wi::font::WIFALIGN_RIGHT;
	SetShadowRadius(2);

	filterCombo.Create("");
	filterCombo.SetShadowRadius(0);
	filterCombo.SetMaxVisibleItemCount(16);
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
	filterCombo.AddItem(ICON_VOXELGRID, (uint64_t)Filter::VoxelGrid);
	filterCombo.AddItem(ICON_RIGIDBODY, (uint64_t)Filter::RigidBody);
	filterCombo.AddItem(ICON_SOFTBODY, (uint64_t)Filter::SoftBody);
	filterCombo.AddItem(ICON_METADATA, (uint64_t)Filter::Metadata);
	filterCombo.SetTooltip("Apply filtering to the Entities by components");
	filterCombo.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	filterCombo.OnSelect([&](wi::gui::EventArgs args) {
		filter = (Filter)args.userdata;
		RefreshEntityTree();
		});
	AddWidget(&filterCombo);


	filterInput.Create("");
	filterInput.SetShadowRadius(0);
	filterInput.SetTooltip("Search entities by name");
	filterInput.SetDescription(ICON_SEARCH "  ");
	filterInput.SetCancelInputEnabled(false);
	filterInput.OnInput([=](wi::gui::EventArgs args) {
		RefreshEntityTree();
		});
	filterInput.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
	AddWidget(&filterInput);

	filterCaseCheckBox.Create("");
	filterCaseCheckBox.SetShadowRadius(0);
	filterCaseCheckBox.SetCheckText("Aa");
	filterCaseCheckBox.SetUnCheckText("a");
	filterCaseCheckBox.SetTooltip("Toggle case-sensitive name filtering");
	filterCaseCheckBox.SetLocalizationEnabled(wi::gui::LocalizationEnabled::Tooltip);
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
				editor->AddSelected(pick, false);
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
	entityTree.OnDoubleClick([this](wi::gui::EventArgs args) {
		editor->FocusCameraOnSelected();
		});
	AddWidget(&entityTree);

	if (editor->main->config.GetSection("layout").Has("entities.height"))
	{
		float height = editor->main->config.GetSection("layout").GetFloat("entities.height");
		entityTree.SetSize(XMFLOAT2(entityTree.GetSize().x, height));
	}

	materialWnd.Create(editor);
	weatherWnd.Create(editor);
	objectWnd.Create(editor);
	meshWnd.Create(editor);
	envProbeWnd.Create(editor);
	soundWnd.Create(editor);
	videoWnd.Create(editor);
	decalWnd.Create(editor);
	lightWnd.Create(editor);
	animWnd.Create(editor);
	emitterWnd.Create(editor);
	hairWnd.Create(editor);
	forceFieldWnd.Create(editor);
	springWnd.Create(editor);
	ikWnd.Create(editor);
	transformWnd.Create(editor);
	layerWnd.Create(editor);
	nameWnd.Create(editor);
	scriptWnd.Create(editor);
	rigidWnd.Create(editor);
	softWnd.Create(editor);
	colliderWnd.Create(editor);
	hierarchyWnd.Create(editor);
	cameraComponentWnd.Create(editor);
	expressionWnd.Create(editor);
	armatureWnd.Create(editor);
	humanoidWnd.Create(editor);
	terrainWnd.Create(editor);
	spriteWnd.Create(editor);
	fontWnd.Create(editor);
	voxelGridWnd.Create(editor);
	metadataWnd.Create(editor);

	enum ADD_THING
	{
		ADD_NAME,
		ADD_LAYER,
		ADD_HIERARCHY,
		ADD_TRANSFORM,
		ADD_LIGHT,
		ADD_MATERIAL,
		ADD_SPRING,
		ADD_IK,
		ADD_SOUND,
		ADD_ENVPROBE,
		ADD_EMITTER,
		ADD_HAIR,
		ADD_DECAL,
		ADD_WEATHER,
		ADD_FORCE,
		ADD_ANIMATION,
		ADD_SCRIPT,
		ADD_RIGIDBODY,
		ADD_SOFTBODY,
		ADD_COLLIDER,
		ADD_CAMERA,
		ADD_OBJECT,
		ADD_VIDEO,
		ADD_SPRITE,
		ADD_FONT,
		ADD_VOXELGRID,
		ADD_METADATA,
	};

	newComponentCombo.Create("Add component  ");
	newComponentCombo.SetDropArrowEnabled(false);
	newComponentCombo.SetAngularHighlightWidth(3);
	newComponentCombo.SetShadowRadius(0);
	newComponentCombo.SetFixedDropWidth(250);
	newComponentCombo.SetTooltip("Add a component to the selected entity.");
	newComponentCombo.SetInvalidSelectionText("+");
	newComponentCombo.AddItem("Name " ICON_NAME, ADD_NAME);
	newComponentCombo.AddItem("Layer " ICON_LAYER, ADD_LAYER);
	newComponentCombo.AddItem("Hierarchy " ICON_HIERARCHY, ADD_HIERARCHY);
	newComponentCombo.AddItem("Transform " ICON_TRANSFORM, ADD_TRANSFORM);
	newComponentCombo.AddItem("Light " ICON_POINTLIGHT, ADD_LIGHT);
	newComponentCombo.AddItem("Material " ICON_MATERIAL, ADD_MATERIAL);
	newComponentCombo.AddItem("Spring " ICON_SPRING, ADD_SPRING);
	newComponentCombo.AddItem("Inverse Kinematics " ICON_IK, ADD_IK);
	newComponentCombo.AddItem("Sound " ICON_SOUND, ADD_SOUND);
	newComponentCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, ADD_ENVPROBE);
	newComponentCombo.AddItem("Emitted Particle System " ICON_EMITTER, ADD_EMITTER);
	newComponentCombo.AddItem("Hair Particle System " ICON_HAIR, ADD_HAIR);
	newComponentCombo.AddItem("Decal " ICON_DECAL, ADD_DECAL);
	newComponentCombo.AddItem("Weather " ICON_WEATHER, ADD_WEATHER);
	newComponentCombo.AddItem("Force Field " ICON_FORCE, ADD_FORCE);
	newComponentCombo.AddItem("Animation " ICON_ANIMATION, ADD_ANIMATION);
	newComponentCombo.AddItem("Script " ICON_SCRIPT, ADD_SCRIPT);
	newComponentCombo.AddItem("Rigid Body Physics " ICON_RIGIDBODY, ADD_RIGIDBODY);
	newComponentCombo.AddItem("Soft Body Physics " ICON_SOFTBODY, ADD_SOFTBODY);
	newComponentCombo.AddItem("Collider " ICON_COLLIDER, ADD_COLLIDER);
	newComponentCombo.AddItem("Camera " ICON_CAMERA, ADD_CAMERA);
	newComponentCombo.AddItem("Object " ICON_OBJECT, ADD_OBJECT);
	newComponentCombo.AddItem("Video " ICON_VIDEO, ADD_VIDEO);
	newComponentCombo.AddItem("Sprite " ICON_SPRITE, ADD_SPRITE);
	newComponentCombo.AddItem("Font " ICON_FONT, ADD_FONT);
	newComponentCombo.AddItem("Voxel Grid " ICON_VOXELGRID, ADD_VOXELGRID);
	newComponentCombo.AddItem("Metadata " ICON_METADATA, ADD_METADATA);
	newComponentCombo.OnSelect([=](wi::gui::EventArgs args) {
		newComponentCombo.SetSelectedWithoutCallback(-1);
		wi::scene::Scene& scene = editor->GetCurrentScene();
		wi::vector<Entity> entities;
		for (auto& x : editor->translator.selected)
		{
			Entity entity = x.entity;
			if (args.userdata == ADD_SOFTBODY)
			{
				// explanation: for softbody, we want to create it for the MeshComponent, if it's also selected together with the object:
				ObjectComponent* object = scene.objects.GetComponent(entity);
				if (object != nullptr)
				{
					entity = object->meshID;
				}
			}
			if (entity == INVALID_ENTITY)
				continue;

			// Can early exit before creating history entry!
			bool valid = true;
			switch (args.userdata)
			{
			case ADD_NAME:
				if (scene.names.Contains(entity))
					valid = false;
				break;
			case ADD_LAYER:
				if (scene.layers.Contains(entity))
					valid = false;
				break;
			case ADD_TRANSFORM:
				if (scene.transforms.Contains(entity))
					valid = false;
				break;
			case ADD_LIGHT:
				if (scene.lights.Contains(entity))
					valid = false;
				break;
			case ADD_MATERIAL:
				if (scene.materials.Contains(entity))
					valid = false;
				break;
			case ADD_SPRING:
				if (scene.springs.Contains(entity))
					valid = false;
				break;
			case ADD_IK:
				if (scene.inverse_kinematics.Contains(entity))
					valid = false;
				break;
			case ADD_SOUND:
				if (scene.sounds.Contains(entity))
					valid = false;
				break;
			case ADD_ENVPROBE:
				if (scene.probes.Contains(entity))
					valid = false;
				break;
			case ADD_EMITTER:
				if (scene.emitters.Contains(entity))
					valid = false;
				break;
			case ADD_HAIR:
				if (scene.hairs.Contains(entity))
					valid = false;
				break;
			case ADD_DECAL:
				if (scene.decals.Contains(entity))
					valid = false;
				break;
			case ADD_WEATHER:
				if (scene.weathers.Contains(entity))
					valid = false;
				break;
			case ADD_FORCE:
				if (scene.forces.Contains(entity))
					valid = false;
				break;
			case ADD_ANIMATION:
				if (scene.animations.Contains(entity))
					valid = false;
				break;
			case ADD_SCRIPT:
				if (scene.scripts.Contains(entity))
					valid = false;
				break;
			case ADD_RIGIDBODY:
				if (scene.rigidbodies.Contains(entity))
					valid = false;
				break;
			case ADD_SOFTBODY:
				if (scene.softbodies.Contains(entity))
					valid = false;
				break;
			case ADD_COLLIDER:
				if (scene.colliders.Contains(entity))
					valid = false;
				break;
			case ADD_HIERARCHY:
				if (scene.hierarchy.Contains(entity))
					valid = false;
				break;
			case ADD_CAMERA:
				if (scene.cameras.Contains(entity))
					valid = false;
				break;
			case ADD_OBJECT:
				if (scene.objects.Contains(entity))
					valid = false;
				break;
			case ADD_VIDEO:
				if (scene.videos.Contains(entity))
					valid = false;
				break;
			case ADD_SPRITE:
				if (scene.sprites.Contains(entity))
					valid = false;
				break;
			case ADD_FONT:
				if (scene.fonts.Contains(entity))
					valid = false;
				break;
			case ADD_VOXELGRID:
				if (scene.voxel_grids.Contains(entity))
					valid = false;
				break;
			case ADD_METADATA:
				if (scene.metadatas.Contains(entity))
					valid = false;
				break;
			default:
				valid = false;
				break;
			}

			if (valid)
			{
				entities.push_back(entity);
			}
		}

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entities);

		for (Entity entity : entities)
		{
			switch (args.userdata)
			{
			case ADD_NAME:
				scene.names.Create(entity);
				break;
			case ADD_LAYER:
				scene.layers.Create(entity);
				break;
			case ADD_TRANSFORM:
				scene.transforms.Create(entity);
				break;
			case ADD_LIGHT:
				scene.lights.Create(entity);
				break;
			case ADD_MATERIAL:
				scene.materials.Create(entity);
				break;
			case ADD_SPRING:
				scene.springs.Create(entity);
				break;
			case ADD_IK:
				scene.inverse_kinematics.Create(entity);
				break;
			case ADD_SOUND:
				scene.sounds.Create(entity);
				break;
			case ADD_ENVPROBE:
				scene.probes.Create(entity);
				break;
			case ADD_EMITTER:
				if (!scene.materials.Contains(entity))
					scene.materials.Create(entity);
				scene.emitters.Create(entity);
				break;
			case ADD_HAIR:
				if (!scene.materials.Contains(entity))
					scene.materials.Create(entity);
				scene.hairs.Create(entity);
				break;
			case ADD_DECAL:
				if (!scene.materials.Contains(entity))
					scene.materials.Create(entity);
				scene.decals.Create(entity);
				break;
			case ADD_WEATHER:
				scene.weathers.Create(entity);
				break;
			case ADD_FORCE:
				scene.forces.Create(entity);
				break;
			case ADD_ANIMATION:
				scene.animations.Create(entity);
				break;
			case ADD_SCRIPT:
				scene.scripts.Create(entity);
				break;
			case ADD_RIGIDBODY:
			{
				RigidBodyPhysicsComponent& rigidbody = scene.rigidbodies.Create(entity);
				rigidbody.SetStartDeactivated(true);
			}
			break;
			case ADD_SOFTBODY:
				scene.softbodies.Create(entity);
				break;
			case ADD_COLLIDER:
				scene.colliders.Create(entity);
				break;
			case ADD_HIERARCHY:
				scene.hierarchy.Create(entity);
				break;
			case ADD_CAMERA:
				scene.cameras.Create(entity);
				break;
			case ADD_OBJECT:
				scene.objects.Create(entity);
				break;
			case ADD_VIDEO:
				scene.videos.Create(entity);
				break;
			case ADD_SPRITE:
				scene.sprites.Create(entity);
				break;
			case ADD_FONT:
				scene.fonts.Create(entity);
				break;
			case ADD_VOXELGRID:
				scene.voxel_grids.Create(entity);
				break;
			case ADD_METADATA:
				scene.metadatas.Create(entity);
				break;
			default:
				break;
			}
		}

		editor->RecordEntity(archive, entities);

		RefreshEntityTree();

	});
	AddWidget(&newComponentCombo);


	AddWidget(&materialWnd);
	AddWidget(&weatherWnd);
	AddWidget(&objectWnd);
	AddWidget(&meshWnd);
	AddWidget(&envProbeWnd);
	AddWidget(&soundWnd);
	AddWidget(&videoWnd);
	AddWidget(&decalWnd);
	AddWidget(&lightWnd);
	AddWidget(&animWnd);
	AddWidget(&emitterWnd);
	AddWidget(&hairWnd);
	AddWidget(&forceFieldWnd);
	AddWidget(&springWnd);
	AddWidget(&ikWnd);
	AddWidget(&transformWnd);
	AddWidget(&layerWnd);
	AddWidget(&nameWnd);
	AddWidget(&scriptWnd);
	AddWidget(&rigidWnd);
	AddWidget(&softWnd);
	AddWidget(&colliderWnd);
	AddWidget(&hierarchyWnd);
	AddWidget(&cameraComponentWnd);
	AddWidget(&expressionWnd);
	AddWidget(&armatureWnd);
	AddWidget(&humanoidWnd);
	AddWidget(&terrainWnd);
	AddWidget(&spriteWnd);
	AddWidget(&fontWnd);
	AddWidget(&voxelGridWnd);
	AddWidget(&metadataWnd);

	materialWnd.SetVisible(false);
	weatherWnd.SetVisible(false);
	objectWnd.SetVisible(false);
	meshWnd.SetVisible(false);
	envProbeWnd.SetVisible(false);
	soundWnd.SetVisible(false);
	videoWnd.SetVisible(false);
	decalWnd.SetVisible(false);
	lightWnd.SetVisible(false);
	animWnd.SetVisible(false);
	emitterWnd.SetVisible(false);
	hairWnd.SetVisible(false);
	forceFieldWnd.SetVisible(false);
	springWnd.SetVisible(false);
	ikWnd.SetVisible(false);
	transformWnd.SetVisible(false);
	layerWnd.SetVisible(false);
	nameWnd.SetVisible(false);
	scriptWnd.SetVisible(false);
	rigidWnd.SetVisible(false);
	softWnd.SetVisible(false);
	colliderWnd.SetVisible(false);
	hierarchyWnd.SetVisible(false);
	cameraComponentWnd.SetVisible(false);
	expressionWnd.SetVisible(false);
	armatureWnd.SetVisible(false);
	humanoidWnd.SetVisible(false);
	terrainWnd.SetVisible(false);
	spriteWnd.SetVisible(false);
	fontWnd.SetVisible(false);
	voxelGridWnd.SetVisible(false);
	metadataWnd.SetVisible(false);

	XMFLOAT2 size = XMFLOAT2(338, 500);
	if (editor->main->config.GetSection("layout").Has("components.width"))
	{
		size.x = editor->main->config.GetSection("layout").GetFloat("components.width");
	}
	SetSize(size);
}
void ComponentsWindow::Update(float dt)
{
	animWnd.Update();
	weatherWnd.Update();
}

void ComponentsWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const wi::scene::Scene& scene = editor->GetCurrentScene();
	float padding = 2;
	XMFLOAT2 pos = XMFLOAT2(padding, 0);
	const float width = GetWidgetAreaSize().x - padding;
	const float height = GetWidgetAreaSize().y - padding * 2;
	editor->main->config.GetSection("layout").Set("components.width", GetSize().x);
	editor->main->config.GetSection("layout").Set("entities.height", entityTree.GetSize().y);

	// Entities:
	{
		float x_off = 25;
		float filterHeight = filterCombo.GetSize().y;
		float filterComboWidth = 30;

		filterInput.SetPos(XMFLOAT2(pos.x + x_off, pos.y));
		filterInput.SetSize(XMFLOAT2(width - x_off - filterHeight - 5 - filterComboWidth - filterHeight, filterCombo.GetScale().y));

		filterCaseCheckBox.SetPos(XMFLOAT2(filterInput.GetPos().x + filterInput.GetSize().x + 1, pos.y));
		filterCaseCheckBox.SetSize(XMFLOAT2(filterHeight, filterHeight));

		filterCombo.SetPos(XMFLOAT2(filterCaseCheckBox.GetPos().x + filterCaseCheckBox.GetSize().x + 1, pos.y));
		filterCombo.SetSize(XMFLOAT2(filterComboWidth, filterHeight));
		pos.y += filterCombo.GetSize().y;
		pos.y += padding;

		pos.x = 0;
		entityTree.SetPos(pos);
		entityTree.SetSize(XMFLOAT2(width, wi::math::Clamp(entityTree.GetSize().y, 0, height - pos.y - 50)));
		pos.y += entityTree.GetSize().y;
		pos.y += padding * 4;
	}

	if (!editor->translator.selected.empty())
	{
		newComponentCombo.SetVisible(true);
		newComponentCombo.SetSize(XMFLOAT2(20, 20));
		newComponentCombo.SetPos(XMFLOAT2(pos.x + width - 30, pos.y));
		pos.y += newComponentCombo.GetSize().y;
		pos.y += padding * 4;
	}
	else
	{
		newComponentCombo.SetVisible(false);
	}

	padding = 1;

	if (scene.names.Contains(nameWnd.entity))
	{
		nameWnd.SetVisible(true);
		nameWnd.SetPos(pos);
		nameWnd.SetSize(XMFLOAT2(width, nameWnd.GetScale().y));
		pos.y += nameWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		nameWnd.SetVisible(false);
	}

	if (scene.hierarchy.Contains(hierarchyWnd.entity))
	{
		hierarchyWnd.SetVisible(true);
		hierarchyWnd.SetPos(pos);
		hierarchyWnd.SetSize(XMFLOAT2(width, hierarchyWnd.GetScale().y));
		pos.y += hierarchyWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		hierarchyWnd.SetVisible(false);
	}

	if (scene.layers.Contains(layerWnd.entity))
	{
		layerWnd.SetVisible(true);
		layerWnd.SetPos(pos);
		layerWnd.SetSize(XMFLOAT2(width, layerWnd.GetScale().y));
		pos.y += layerWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		layerWnd.SetVisible(false);
	}

	if (scene.transforms.Contains(transformWnd.entity))
	{
		transformWnd.SetVisible(true);
		transformWnd.SetPos(pos);
		transformWnd.SetSize(XMFLOAT2(width, transformWnd.GetScale().y));
		pos.y += transformWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		transformWnd.SetVisible(false);
	}

	if (scene.inverse_kinematics.Contains(ikWnd.entity))
	{
		ikWnd.SetVisible(true);
		ikWnd.SetPos(pos);
		ikWnd.SetSize(XMFLOAT2(width, ikWnd.GetScale().y));
		pos.y += ikWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		ikWnd.SetVisible(false);
	}

	if (scene.springs.Contains(springWnd.entity))
	{
		springWnd.SetVisible(true);
		springWnd.SetPos(pos);
		springWnd.SetSize(XMFLOAT2(width, springWnd.GetScale().y));
		pos.y += springWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		springWnd.SetVisible(false);
	}

	if (scene.forces.Contains(forceFieldWnd.entity))
	{
		forceFieldWnd.SetVisible(true);
		forceFieldWnd.SetPos(pos);
		forceFieldWnd.SetSize(XMFLOAT2(width, forceFieldWnd.GetScale().y));
		pos.y += forceFieldWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		forceFieldWnd.SetVisible(false);
	}

	if (scene.hairs.Contains(hairWnd.entity))
	{
		hairWnd.SetVisible(true);
		hairWnd.SetPos(pos);
		hairWnd.SetSize(XMFLOAT2(width, hairWnd.GetScale().y));
		pos.y += hairWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		hairWnd.SetVisible(false);
	}

	if (scene.emitters.Contains(emitterWnd.entity))
	{
		emitterWnd.SetVisible(true);
		emitterWnd.SetPos(pos);
		emitterWnd.SetSize(XMFLOAT2(width, emitterWnd.GetScale().y));
		pos.y += emitterWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		emitterWnd.SetVisible(false);
	}

	if (scene.animations.Contains(animWnd.entity))
	{
		animWnd.SetVisible(true);
		animWnd.SetPos(pos);
		animWnd.SetSize(XMFLOAT2(width, animWnd.GetScale().y));
		pos.y += animWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		animWnd.SetVisible(false);
	}

	if (scene.lights.Contains(lightWnd.entity))
	{
		lightWnd.SetVisible(true);
		lightWnd.SetPos(pos);
		lightWnd.SetSize(XMFLOAT2(width, lightWnd.GetScale().y));
		pos.y += lightWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		lightWnd.SetVisible(false);
	}

	if (scene.sounds.Contains(soundWnd.entity))
	{
		soundWnd.SetVisible(true);
		soundWnd.SetPos(pos);
		soundWnd.SetSize(XMFLOAT2(width, soundWnd.GetScale().y));
		pos.y += soundWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		soundWnd.SetVisible(false);
	}

	if (scene.videos.Contains(videoWnd.entity))
	{
		videoWnd.SetVisible(true);
		videoWnd.SetPos(pos);
		videoWnd.SetSize(XMFLOAT2(width, videoWnd.GetScale().y));
		pos.y += videoWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		videoWnd.SetVisible(false);
	}

	if (scene.decals.Contains(decalWnd.entity))
	{
		decalWnd.SetVisible(true);
		decalWnd.SetPos(pos);
		decalWnd.SetSize(XMFLOAT2(width, decalWnd.GetScale().y));
		pos.y += decalWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		decalWnd.SetVisible(false);
	}

	if (scene.probes.Contains(envProbeWnd.entity))
	{
		envProbeWnd.SetVisible(true);
		envProbeWnd.SetPos(pos);
		envProbeWnd.SetSize(XMFLOAT2(width, envProbeWnd.GetScale().y));
		pos.y += envProbeWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		envProbeWnd.SetVisible(false);
	}

	//if (scene.cameras.Contains(cameraWnd.entity))
	//{
	//	cameraWnd.SetVisible(true);
	//	cameraWnd.SetPos(pos);
	//	cameraWnd.SetSize(XMFLOAT2(width, cameraWnd.GetScale().y));
	//	pos.y += cameraWnd.GetSize().y;
	//	pos.y += padding;
	//}
	//else
	//{
	//	cameraWnd.SetVisible(false);
	//}

	if (scene.materials.Contains(materialWnd.entity))
	{
		materialWnd.SetVisible(true);
		materialWnd.SetPos(pos);
		materialWnd.SetSize(XMFLOAT2(width, materialWnd.GetScale().y));
		pos.y += materialWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		materialWnd.SetVisible(false);
	}

	if (scene.meshes.Contains(meshWnd.entity))
	{
		meshWnd.SetVisible(true);
		meshWnd.SetPos(pos);
		meshWnd.SetSize(XMFLOAT2(width, meshWnd.GetScale().y));
		pos.y += meshWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		meshWnd.SetVisible(false);
	}

	if (scene.softbodies.Contains(softWnd.entity))
	{
		softWnd.SetVisible(true);
		softWnd.SetPos(pos);
		softWnd.SetSize(XMFLOAT2(width, softWnd.GetScale().y));
		pos.y += softWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		softWnd.SetVisible(false);
	}

	if (scene.objects.Contains(objectWnd.entity))
	{
		objectWnd.SetVisible(true);
		objectWnd.SetPos(pos);
		objectWnd.SetSize(XMFLOAT2(width, objectWnd.GetScale().y));
		pos.y += objectWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		objectWnd.SetVisible(false);
	}

	if (scene.rigidbodies.Contains(rigidWnd.entity))
	{
		rigidWnd.SetVisible(true);
		rigidWnd.SetPos(pos);
		rigidWnd.SetSize(XMFLOAT2(width, rigidWnd.GetScale().y));
		pos.y += rigidWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		rigidWnd.SetVisible(false);
	}

	if (scene.weathers.Contains(weatherWnd.entity))
	{
		weatherWnd.SetVisible(true);
		weatherWnd.SetPos(pos);
		weatherWnd.SetSize(XMFLOAT2(width, weatherWnd.GetScale().y));
		pos.y += weatherWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		weatherWnd.SetVisible(false);
	}

	if (scene.colliders.Contains(colliderWnd.entity))
	{
		colliderWnd.SetVisible(true);
		colliderWnd.SetPos(pos);
		colliderWnd.SetSize(XMFLOAT2(width, colliderWnd.GetScale().y));
		pos.y += colliderWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		colliderWnd.SetVisible(false);
	}

	if (scene.cameras.Contains(cameraComponentWnd.entity))
	{
		cameraComponentWnd.SetVisible(true);
		cameraComponentWnd.SetPos(pos);
		cameraComponentWnd.SetSize(XMFLOAT2(width, cameraComponentWnd.GetScale().y));
		pos.y += cameraComponentWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		cameraComponentWnd.SetVisible(false);
	}

	if (scene.scripts.Contains(scriptWnd.entity))
	{
		scriptWnd.SetVisible(true);
		scriptWnd.SetPos(pos);
		scriptWnd.SetSize(XMFLOAT2(width, scriptWnd.GetScale().y));
		pos.y += scriptWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		scriptWnd.SetVisible(false);
	}

	if (scene.armatures.Contains(armatureWnd.entity))
	{
		armatureWnd.SetVisible(true);
		armatureWnd.SetPos(pos);
		armatureWnd.SetSize(XMFLOAT2(width, armatureWnd.GetScale().y));
		pos.y += armatureWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		armatureWnd.SetVisible(false);
	}

	if (scene.humanoids.Contains(humanoidWnd.entity))
	{
		humanoidWnd.SetVisible(true);
		humanoidWnd.SetPos(pos);
		humanoidWnd.SetSize(XMFLOAT2(width, humanoidWnd.GetScale().y));
		pos.y += humanoidWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		humanoidWnd.SetVisible(false);
	}

	if (scene.expressions.Contains(expressionWnd.entity))
	{
		expressionWnd.SetVisible(true);
		expressionWnd.SetPos(pos);
		expressionWnd.SetSize(XMFLOAT2(width, expressionWnd.GetScale().y));
		pos.y += expressionWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		expressionWnd.SetVisible(false);
	}

	if (scene.terrains.Contains(terrainWnd.entity))
	{
		terrainWnd.SetVisible(true);
		terrainWnd.SetPos(pos);
		terrainWnd.SetSize(XMFLOAT2(width, terrainWnd.GetScale().y));
		pos.y += terrainWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		terrainWnd.SetVisible(false);
	}

	if (scene.sprites.Contains(spriteWnd.entity))
	{
		spriteWnd.SetVisible(true);
		spriteWnd.SetPos(pos);
		spriteWnd.SetSize(XMFLOAT2(width, spriteWnd.GetScale().y));
		pos.y += spriteWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		spriteWnd.SetVisible(false);
	}

	if (scene.fonts.Contains(fontWnd.entity))
	{
		fontWnd.SetVisible(true);
		fontWnd.SetPos(pos);
		fontWnd.SetSize(XMFLOAT2(width, fontWnd.GetScale().y));
		pos.y += fontWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		fontWnd.SetVisible(false);
	}

	if (scene.voxel_grids.Contains(voxelGridWnd.entity))
	{
		voxelGridWnd.SetVisible(true);
		voxelGridWnd.SetPos(pos);
		voxelGridWnd.SetSize(XMFLOAT2(width, voxelGridWnd.GetScale().y));
		pos.y += voxelGridWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		voxelGridWnd.SetVisible(false);
	}

	if (scene.metadatas.Contains(metadataWnd.entity))
	{
		metadataWnd.SetVisible(true);
		metadataWnd.SetPos(pos);
		metadataWnd.SetSize(XMFLOAT2(width, metadataWnd.GetScale().y));
		pos.y += metadataWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		metadataWnd.SetVisible(false);
	}
}


void ComponentsWindow::PushToEntityTree(wi::ecs::Entity entity, int level)
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

		bool filter_valid = true;
		std::string name_filter = filterInput.GetCurrentInputValue();
		if (!name_filter.empty())
		{
			if (filterCaseCheckBox.GetCheck() && name_string.find(name_filter) == std::string::npos)
			{
				filter_valid = false;
			}
			else if (wi::helper::toUpper(name_string).find(wi::helper::toUpper(name_filter)) == std::string::npos)
			{
				filter_valid = false;
			}
			item.level = 0;
		}

		if (filter_valid)
		{
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
			if (scene.voxel_grids.Contains(entity))
			{
				item.name += ICON_VOXELGRID " ";
			}
			if (scene.metadatas.Contains(entity))
			{
				item.name += ICON_METADATA " ";
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
	}

	for (size_t i = 0; i < scene.hierarchy.GetCount(); ++i)
	{
		if (scene.hierarchy[i].parentID == entity)
		{
			PushToEntityTree(scene.hierarchy.GetEntity(i), level + 1);
		}
	}
}
void ComponentsWindow::RefreshEntityTree()
{
	if (editor == nullptr)
		return;
	const Scene& scene = editor->GetCurrentScene();
	editor->materialPickerWnd.RecreateButtons();

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
		const HierarchyComponent* hier = scene.hierarchy.GetComponent(x);
		if (hier == nullptr || hier->parentID == INVALID_ENTITY)
		{
			PushToEntityTree(x, 0);
		}
	}

	entitytree_added_items.clear();
	entitytree_opened_items.clear();
}
bool ComponentsWindow::CheckEntityFilter(wi::ecs::Entity entity)
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
		has_flag(filter, Filter::Font) && scene.fonts.Contains(entity) ||
		has_flag(filter, Filter::VoxelGrid) && scene.voxel_grids.Contains(entity) ||
		has_flag(filter, Filter::RigidBody) && scene.rigidbodies.Contains(entity) ||
		has_flag(filter, Filter::SoftBody) && scene.softbodies.Contains(entity) ||
		has_flag(filter, Filter::Metadata) && scene.metadatas.Contains(entity)
		)
	{
		valid = true;
	}

	return valid;
}
