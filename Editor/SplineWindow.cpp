#include "stdafx.h"
#include "SplineWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void SplineWindow::Create(EditorComponent* _editor)
{
	editor = _editor;

	wi::gui::Window::Create(ICON_SPLINE " Spline", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(670, 440));

	closeButton.SetTooltip("Delete SplineComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().splines.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	infoLabel.Create("SplineInfo");
	infoLabel.SetSize(XMFLOAT2(100, 120));
	infoLabel.SetText("The spline is a curve that goes through every specified entity that has a TransformComponent smoothly. A mesh can be generated from it automatically by increasing the subdivisions. It can also modify terrain when the terrain modification slider is used. The terrain texture can also be modified if the spline has a material.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	auto forEachSelectedAndIndirect = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				SplineComponent* spline = scene.splines.GetComponent(x.entity);
				if (spline != nullptr)
				{
					func(spline, args);
				}
			}

			// indirect set:
			SplineComponent* spline = scene.splines.GetComponent(entity);
			if (spline != nullptr)
			{
				func(spline, args);
			}

			editor->componentsWnd.RefreshEntityTree();
		};
	};

	loopedCheck.Create("Looped: ");
	loopedCheck.OnClick([this, forEachSelectedAndIndirect](wi::gui::EventArgs args) {
		forEachSelectedAndIndirect([](auto spline, auto args) {
			spline->SetLooped(args.bValue);
		})(args);

		filledCheck.SetEnabled(args.bValue);
		if (!args.bValue)
		{
			filledCheck.SetCheck(false);
			forEachSelectedAndIndirect([](auto spline, auto args) {
				spline->SetFilled(false);
			})(args);
		}
	});
	AddWidget(&loopedCheck);

	filledCheck.Create("Filled: ");
	filledCheck.SetTooltip("Fill the interior of the looped spline.");
	filledCheck.OnClick(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->SetFilled(args.bValue);
	}));
	AddWidget(&filledCheck);

	alignedCheck.Create("Draw Aligned: ");
	alignedCheck.OnClick(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->SetDrawAligned(args.bValue);
	}));
	AddWidget(&alignedCheck);

	widthSlider.Create(0.001f, 4, 0, 1000, "Width: ");
	widthSlider.SetTooltip("Set overall width multiplier for all nodes, used in mesh generation.");
	widthSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->width = args.fValue;
	}));
	AddWidget(&widthSlider);

	rotSlider.Create(0, 360, 0, 360, "Rotation: ");
	rotSlider.SetTooltip("Set overall rotation for all nodes, used in mesh generation.");
	rotSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		float rad = wi::math::DegreesToRadians(args.fValue);
		spline->rotation = rad;
	}));
	AddWidget(&rotSlider);

	subdivSlider.Create(0, 100, 0, 100, "Mesh subdivision: ");
	subdivSlider.SetTooltip("Set subdivision count for mesh generation. \nIncreasing this above 0 will enable mesh generation and higher values mean higher quality.");
	subdivSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->mesh_generation_subdivision = args.iValue;
	}));
	AddWidget(&subdivSlider);

	subdivVerticalSlider.Create(0, 36, 0, 36, "Vertical subdivision: ");
	subdivVerticalSlider.SetTooltip("Set subdivision count for mesh generation's vertical axis to create a corridoor or a tunnel.");
	subdivVerticalSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->mesh_generation_vertical_subdivision = args.iValue;
	}));
	AddWidget(&subdivVerticalSlider);

	terrainSlider.Create(0, 1, 0, 100, "Terrain modifier: ");
	terrainSlider.SetTooltip("Set terrain modification strength (0 to turn it off, higher values mean more sharpness).");
	terrainSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->terrain_modifier_amount = args.fValue;
	}));
	AddWidget(&terrainSlider);

	terrainTexSlider.Create(0, 1, 0, 100, "Terrain texture falloff: ");
	terrainTexSlider.SetTooltip("Affects the terrain texturing falloff when spline is terrain modifying.");
	terrainTexSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->terrain_texture_falloff = args.fValue;
	}));
	AddWidget(&terrainTexSlider);

	terrainPushdownSlider.Create(0, 10, 0, 100, "Terrain push down: ");
	terrainPushdownSlider.SetTooltip("Push down the terrain genetry from the spline plane by an amount.");
	terrainPushdownSlider.OnSlide(forEachSelectedAndIndirect([] (auto spline, auto args) {
		spline->terrain_pushdown = args.fValue;
	}));
	AddWidget(&terrainPushdownSlider);

	recenterButton.Create("Recenter");
	recenterButton.SetTooltip("Recompute the parent transform position to the center of spline nodes.");
	recenterButton.OnClick([this](wi::gui::EventArgs args) {
		auto recenterSpline = [](wi::scene::Scene& scene, Entity splineEntity) {
			SplineComponent* spline = scene.splines.GetComponent(splineEntity);
			if (spline == nullptr || spline->spline_node_entities.empty())
				return;

			TransformComponent* parent_transform = scene.transforms.GetComponent(splineEntity);
			if (parent_transform == nullptr)
				return;

			// Compute centroid of all spline nodes in local space
			XMVECTOR localCentroid = XMVectorZero();
			size_t validCount = 0;
			for (size_t i = 0; i < spline->spline_node_entities.size(); ++i)
			{
				const Entity node_entity = spline->spline_node_entities[i];
				const TransformComponent* node_transform = scene.transforms.GetComponent(node_entity);
				if (node_transform != nullptr)
				{
					localCentroid = XMVectorAdd(localCentroid, XMLoadFloat3(&node_transform->translation_local));
					validCount++;
				}
			}

			if (validCount == 0)
				return;

			localCentroid = XMVectorScale(localCentroid, 1.0f / float(validCount));

			// Offset each child node's local position by negative centroid, so they stay in place after parent moves
			for (size_t i = 0; i < spline->spline_node_entities.size(); ++i)
			{
				const Entity node_entity = spline->spline_node_entities[i];
				TransformComponent* node_transform = scene.transforms.GetComponent(node_entity);
				if (node_transform != nullptr)
				{
					const XMVECTOR newLocalPos = XMVectorSubtract(XMLoadFloat3(&node_transform->translation_local), localCentroid);
					XMStoreFloat3(&node_transform->translation_local, newLocalPos);
					node_transform->SetDirty();
				}
			}

			// Move the parent transform by the local centroid, transformed to world space
			const XMVECTOR worldOffset = XMVector3TransformNormal(localCentroid, XMLoadFloat4x4(&parent_transform->world));
			const XMVECTOR newParentPos = XMVectorAdd(XMLoadFloat3(&parent_transform->translation_local), worldOffset);
			XMStoreFloat3(&parent_transform->translation_local, newParentPos);
			parent_transform->SetDirty();

			spline->SetDirty();
		};

		wi::scene::Scene& scene = editor->GetCurrentScene();
		// Apply to all selected splines
		for (auto& x : editor->translator.selected)
		{
			if (scene.splines.Contains(x.entity))
			{
				recenterSpline(scene, x.entity);
			}
		}
		// Also apply to indirect entity
		if (scene.splines.Contains(entity))
		{
			recenterSpline(scene, entity);
		}

		editor->componentsWnd.RefreshEntityTree();
	});
	AddWidget(&recenterButton);

	addButton.Create("AddNode");
	addButton.SetText("+");
	addButton.SetTooltip("Add an entity as a node to the spline (it must have TransformComponent). Hotkey: Ctrl + E");
	addButton.OnClick([this](wi::gui::EventArgs args) {
		NewNode();
	});
	AddWidget(&addButton);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void SplineWindow::SetEntity(Entity entity)
{
	const bool changed = this->entity != entity;
	Scene& scene = editor->GetCurrentScene();

	const SplineComponent* spline = scene.splines.GetComponent(entity);

	if (spline != nullptr)
	{
		this->entity = entity;

		alignedCheck.SetCheck(spline->IsDrawAligned());
		loopedCheck.SetCheck(spline->IsLooped());
		filledCheck.SetCheck(spline->IsFilled());
		filledCheck.SetEnabled(spline->IsLooped());
		widthSlider.SetValue(spline->width);
		rotSlider.SetValue(wi::math::RadiansToDegrees(spline->rotation));
		subdivSlider.SetValue(spline->mesh_generation_subdivision);
		subdivVerticalSlider.SetValue(spline->mesh_generation_vertical_subdivision);
		terrainSlider.SetValue(spline->terrain_modifier_amount);
		terrainPushdownSlider.SetValue(spline->terrain_pushdown);
		terrainTexSlider.SetValue(spline->terrain_texture_falloff);

		if (changed)
		{
			RefreshEntries();
			SetEnabled(true);
		}
	}
	else
	{
		this->entity = INVALID_ENTITY;
	}

}

void SplineWindow::RefreshEntries()
{
	for (auto& entry : entries)
	{
		RemoveWidget(&entry.removeButton);
		RemoveWidget(&entry.entityButton);
	}
	entries.clear();

	const Scene& scene = editor->GetCurrentScene();
	SplineComponent* spline = scene.splines.GetComponent(entity);
	if (spline == nullptr)
		return;

	for (size_t i = 0; i < spline->spline_node_entities.size(); ++i)
	{
		Entity entity = spline->spline_node_entities[i];
		const NameComponent* name = scene.names.GetComponent(entity);

		Entry& entry = entries.emplace_back();
		entry.entityButton.Create("");
		if (name != nullptr)
		{
			entry.entityButton.SetText(name->name);
		}
		else
		{
			entry.entityButton.SetText("[unnamed] " + std::to_string(entity));
		}
		entry.entityButton.OnClick([this, entity](wi::gui::EventArgs args) {
			editor->ClearSelected();
			editor->AddSelected(entity);
			});
		AddWidget(&entry.entityButton);

		entry.removeButton.Create("");
		entry.removeButton.SetText("X");
		entry.removeButton.SetSize(XMFLOAT2(entry.removeButton.GetSize().y, entry.removeButton.GetSize().y));
		entry.removeButton.OnClick([i, spline, this](wi::gui::EventArgs args) {
			Scene& scene = editor->GetCurrentScene();
			scene.Entity_Remove(spline->spline_node_entities[i]);
			spline->spline_node_entities.erase(spline->spline_node_entities.begin() + i);
			spline->spline_node_transforms.erase(spline->spline_node_transforms.begin() + i);
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t userdata) {
				RefreshEntries();
				editor->componentsWnd.RefreshEntityTree();
				});
			});
		AddWidget(&entry.removeButton);
	}

	editor->generalWnd.RefreshTheme();
}

void SplineWindow::NewNode()
{
	wi::scene::Scene& scene = editor->GetCurrentScene();
	SplineComponent* spline = scene.splines.GetComponent(entity);
	if (spline == nullptr)
		return;
	Entity node_entity = CreateEntity();
	scene.names.Create(node_entity) = "spline_node_" + std::to_string(spline->spline_node_entities.size());
	TransformComponent& transform = scene.transforms.Create(node_entity);
	if (!spline->spline_node_transforms.empty())
	{
		XMVECTOR D = XMVectorSet(1, 0, 0, 0);
		if (spline->spline_node_transforms.size() > 1)
		{
			D = XMVector3Normalize(spline->spline_node_transforms[spline->spline_node_transforms.size() - 1].GetPositionV() - spline->spline_node_transforms[spline->spline_node_transforms.size() - 2].GetPositionV());
		}
		transform = spline->spline_node_transforms.back();
		transform.Translate(D);
		transform.UpdateTransform();
	}
	spline->spline_node_entities.push_back(node_entity);
	spline->spline_node_transforms.push_back(transform);
	scene.Component_Attach(node_entity, entity);
	RefreshEntries();

	wi::Archive& archive = editor->AdvanceHistory();
	archive << EditorComponent::HISTORYOP_ADD_TO_SPLINE;
	editor->RecordSelection(archive);

	editor->ClearSelected();
	editor->AddSelected(node_entity);

	editor->RecordSelection(archive);

	editor->RecordEntity(archive, entity);
	editor->RecordEntity(archive, node_entity);

	editor->componentsWnd.RefreshEntityTree();
}

void SplineWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 145;

	layout.add_fullwidth(infoLabel);
	layout.add(subdivSlider);
	layout.add(subdivVerticalSlider);
	layout.add(terrainSlider);
	layout.add(terrainPushdownSlider);
	layout.add(terrainTexSlider);
	layout.add(widthSlider);
	layout.add(rotSlider);
	layout.add_right(loopedCheck);
	layout.add_right(filledCheck);
	layout.add_right(alignedCheck);

	layout.y += layout.padding * 2;

	layout.add_fullwidth(recenterButton);
	layout.add_fullwidth(addButton);

	for (auto& entry : entries)
	{
		entry.removeButton.SetPos(XMFLOAT2(layout.padding, layout.y));

		entry.entityButton.SetSize(XMFLOAT2(layout.width - layout.padding * 3 - entry.removeButton.GetSize().x, entry.entityButton.GetSize().y));
		entry.entityButton.SetPos(XMFLOAT2(entry.removeButton.GetPos().x + entry.removeButton.GetSize().x + layout.padding, layout.y));

		layout.y += entry.entityButton.GetSize().y;
		layout.y += layout.padding;
	}
}
