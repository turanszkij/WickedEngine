#include "stdafx.h"
#include "ComponentsWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void ComponentsWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create("Components ", wi::gui::Window::WindowControls::RESIZE_TOPLEFT);
	font.params.h_align = wi::font::WIFALIGN_RIGHT;
	SetShadowRadius(2);


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
	};

	newComponentCombo.Create("Add: ");
	newComponentCombo.selected_font.anim.typewriter.looped = true;
	newComponentCombo.selected_font.anim.typewriter.time = 2;
	newComponentCombo.selected_font.anim.typewriter.character_start = 1;
	newComponentCombo.SetTooltip("Add a component to the last selected entity.");
	newComponentCombo.SetInvalidSelectionText("...");
	newComponentCombo.AddItem("Name " ICON_NAME, ADD_NAME);
	newComponentCombo.AddItem("Layer " ICON_LAYER, ADD_LAYER);
	newComponentCombo.AddItem("Hierarchy " ICON_HIERARCHY, ADD_HIERARCHY);
	newComponentCombo.AddItem("Transform " ICON_TRANSFORM, ADD_TRANSFORM);
	newComponentCombo.AddItem("Light " ICON_POINTLIGHT, ADD_LIGHT);
	newComponentCombo.AddItem("Material " ICON_MATERIAL, ADD_MATERIAL);
	newComponentCombo.AddItem("Spring", ADD_SPRING);
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
	newComponentCombo.OnSelect([=](wi::gui::EventArgs args) {
		newComponentCombo.SetSelectedWithoutCallback(-1);
		if (editor->translator.selected.empty())
			return;
		Scene& scene = editor->GetCurrentScene();
		Entity entity = editor->translator.selected.back().entity;
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
		{
			assert(0);
			return;
		}

		// Can early exit before creating history entry!
		switch (args.userdata)
		{
		case ADD_NAME:
			if (scene.names.Contains(entity))
				return;
			break;
		case ADD_LAYER:
			if (scene.layers.Contains(entity))
				return;
			break;
		case ADD_TRANSFORM:
			if (scene.transforms.Contains(entity))
				return;
			break;
		case ADD_LIGHT:
			if (scene.lights.Contains(entity))
				return;
			break;
		case ADD_MATERIAL:
			if (scene.materials.Contains(entity))
				return;
			break;
		case ADD_SPRING:
			if (scene.springs.Contains(entity))
				return;
			break;
		case ADD_IK:
			if (scene.inverse_kinematics.Contains(entity))
				return;
			break;
		case ADD_SOUND:
			if (scene.sounds.Contains(entity))
				return;
			break;
		case ADD_ENVPROBE:
			if (scene.probes.Contains(entity))
				return;
			break;
		case ADD_EMITTER:
			if (scene.emitters.Contains(entity))
				return;
			break;
		case ADD_HAIR:
			if (scene.hairs.Contains(entity))
				return;
			break;
		case ADD_DECAL:
			if (scene.decals.Contains(entity))
				return;
			break;
		case ADD_WEATHER:
			if (scene.weathers.Contains(entity))
				return;
			break;
		case ADD_FORCE:
			if (scene.forces.Contains(entity))
				return;
			break;
		case ADD_ANIMATION:
			if (scene.animations.Contains(entity))
				return;
			break;
		case ADD_SCRIPT:
			if (scene.scripts.Contains(entity))
				return;
			break;
		case ADD_RIGIDBODY:
			if (scene.rigidbodies.Contains(entity))
				return;
			break;
		case ADD_SOFTBODY:
			if (scene.softbodies.Contains(entity))
				return;
			break;
		case ADD_COLLIDER:
			if (scene.colliders.Contains(entity))
				return;
			break;
		case ADD_HIERARCHY:
			if (scene.hierarchy.Contains(entity))
				return;
			break;
		case ADD_CAMERA:
			if (scene.cameras.Contains(entity))
				return;
			break;
		case ADD_OBJECT:
			if (scene.objects.Contains(entity))
				return;
			break;
		case ADD_VIDEO:
			if (scene.videos.Contains(entity))
				return;
			break;
		case ADD_SPRITE:
			if (scene.sprites.Contains(entity))
				return;
			break;
		case ADD_FONT:
			if (scene.fonts.Contains(entity))
				return;
			break;
		default:
			return;
		}

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

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

			// Springs are special because they are computed in ordered fashion
			//	So if we add a new spring that was parent of an other one, we move it in memory before the child
			for (size_t i = 0; i < scene.springs.GetCount(); ++i)
			{
				Entity other = scene.springs.GetEntity(i);
				const HierarchyComponent* hier = scene.hierarchy.GetComponent(other);
				if (hier != nullptr && hier->parentID == entity)
				{
					size_t entity_index = scene.springs.GetCount() - 1; // last added entity (the parent)
					scene.springs.MoveItem(entity_index, i); // will be moved before
					break;
				}
			}
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
				rigidbody.SetKinematic(true); // Set it to kinematic so that it doesn't immediately fall
				rigidbody.SetDisableDeactivation(true);
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
		default:
			break;
		}

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();

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

	XMFLOAT2 size = XMFLOAT2(338, 500);
	if (editor->main->config.GetSection("layout").Has("components.width"))
	{
		size.x = editor->main->config.GetSection("layout").GetFloat("components.width");
	}
	if (editor->main->config.GetSection("layout").Has("components.height"))
	{
		size.y = editor->main->config.GetSection("layout").GetFloat("components.height");
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
	const float padding = 4;
	XMFLOAT2 pos = XMFLOAT2(padding, padding);
	const float width = GetWidgetAreaSize().x - padding * 2;
	editor->main->config.GetSection("layout").Set("components.width", GetSize().x);
	editor->main->config.GetSection("layout").Set("components.height", GetSize().y);

	if (!editor->translator.selected.empty())
	{
		newComponentCombo.SetVisible(true);
		newComponentCombo.SetPos(XMFLOAT2(pos.x + 35, pos.y));
		newComponentCombo.SetSize(XMFLOAT2(width - 35 - 21, 20));
		pos.y += newComponentCombo.GetSize().y;
		pos.y += padding;
	}
	else
	{
		newComponentCombo.SetVisible(false);
	}

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
}
