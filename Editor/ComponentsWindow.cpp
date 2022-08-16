#include "stdafx.h"
#include "ComponentsWindow.h"
#include "Editor.h"

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


	newComponentCombo.Create("Add: ");
	newComponentCombo.selected_font.anim.typewriter.looped = true;
	newComponentCombo.selected_font.anim.typewriter.time = 2;
	newComponentCombo.selected_font.anim.typewriter.character_start = 1;
	newComponentCombo.SetTooltip("Add a component to the first selected entity.");
	newComponentCombo.AddItem("...", ~0ull);
	newComponentCombo.AddItem("Name", 0);
	newComponentCombo.AddItem("Layer " ICON_LAYER, 1);
	newComponentCombo.AddItem("Transform " ICON_TRANSFORM, 2);
	newComponentCombo.AddItem("Light " ICON_POINTLIGHT, 3);
	newComponentCombo.AddItem("Matetial " ICON_MATERIAL, 4);
	newComponentCombo.AddItem("Spring", 5);
	newComponentCombo.AddItem("Inverse Kinematics", 6);
	newComponentCombo.AddItem("Sound " ICON_SOUND, 7);
	newComponentCombo.AddItem("Environment Probe " ICON_ENVIRONMENTPROBE, 8);
	newComponentCombo.AddItem("Emitted Particle System " ICON_EMITTER, 9);
	newComponentCombo.AddItem("Hair Particle System " ICON_HAIR, 10);
	newComponentCombo.AddItem("Decal " ICON_DECAL, 11);
	newComponentCombo.AddItem("Weather " ICON_WEATHER, 12);
	newComponentCombo.AddItem("Force Field " ICON_FORCE, 13);
	newComponentCombo.AddItem("Animation " ICON_ANIMATION, 14);
	newComponentCombo.OnSelect([=](wi::gui::EventArgs args) {
		newComponentCombo.SetSelectedWithoutCallback(0);
		if (editor->translator.selected.empty())
			return;
		Scene& scene = editor->GetCurrentScene();
		Entity entity = editor->translator.selected.front().entity;
		if (entity == INVALID_ENTITY)
		{
			assert(0);
			return;
		}

		// Can early exit before creating history entry!
		switch (args.userdata)
		{
		case 0:
			if (scene.names.Contains(entity))
				return;
			break;
		case 1:
			if (scene.layers.Contains(entity))
				return;
			break;
		case 2:
			if (scene.transforms.Contains(entity))
				return;
			break;
		case 3:
			if (scene.lights.Contains(entity))
				return;
			break;
		case 4:
			if (scene.materials.Contains(entity))
				return;
			break;
		case 5:
			if (scene.springs.Contains(entity))
				return;
			break;
		case 6:
			if (scene.inverse_kinematics.Contains(entity))
				return;
			break;
		case 7:
			if (scene.inverse_kinematics.Contains(entity))
				return;
			break;
		case 8:
			if (scene.probes.Contains(entity))
				return;
			break;
		case 9:
			if (scene.emitters.Contains(entity))
				return;
			break;
		case 10:
			if (scene.hairs.Contains(entity))
				return;
			break;
		case 11:
			if (scene.decals.Contains(entity))
				return;
			break;
		case 12:
			if (scene.weathers.Contains(entity))
				return;
			break;
		case 13:
			if (scene.forces.Contains(entity))
				return;
			break;
		case 14:
			if (scene.animations.Contains(entity))
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
		case 0:
			scene.names.Create(entity);
			break;
		case 1:
			scene.layers.Create(entity);
			break;
		case 2:
			scene.transforms.Create(entity);
			break;
		case 3:
			scene.lights.Create(entity);
			scene.aabb_lights.Create(entity);
			break;
		case 4:
			scene.materials.Create(entity);
			break;
		case 5:
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
		case 6:
			scene.inverse_kinematics.Create(entity);
			break;
		case 7:
			scene.sounds.Create(entity);
			break;
		case 8:
			scene.probes.Create(entity);
			scene.aabb_probes.Create(entity);
			break;
		case 9:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.emitters.Create(entity);
			break;
		case 10:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.hairs.Create(entity);
			break;
		case 11:
			if (!scene.materials.Contains(entity))
				scene.materials.Create(entity);
			scene.decals.Create(entity);
			scene.aabb_decals.Create(entity);
			break;
		case 12:
			scene.weathers.Create(entity);
			break;
		case 13:
			scene.forces.Create(entity);
			break;
		case 14:
			scene.animations.Create(entity);
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

	materialWnd.SetVisible(false);
	weatherWnd.SetVisible(false);
	objectWnd.SetVisible(false);
	meshWnd.SetVisible(false);
	envProbeWnd.SetVisible(false);
	soundWnd.SetVisible(false);
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


	SetSize(editor->optionsWnd.GetSize());
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
		nameWnd.Update();
		pos.y += nameWnd.GetSize().y;
		pos.y += padding;
	}
	else
	{
		nameWnd.SetVisible(false);
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
}
