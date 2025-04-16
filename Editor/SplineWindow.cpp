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
	infoLabel.SetText("The spline is a curve that goes through every specified entity that has a TransformComponent smoothly. A mesh can be generated from it automatically by increasing the subdivisions. It can also modify terrain when the terrain modification slider is used.");
	AddWidget(&infoLabel);

	loopedCheck.Create("Looped: ");
	loopedCheck.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->SetLooped(args.bValue);
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->SetLooped(args.bValue);
		}
	});
	AddWidget(&loopedCheck);

	alignedCheck.Create("Draw Aligned: ");
	alignedCheck.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->SetDrawAligned(args.bValue);
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->SetDrawAligned(args.bValue);
		}
	});
	AddWidget(&alignedCheck);

	widthSlider.Create(0.001f, 4, 0, 1000, "Width: ");
	widthSlider.SetTooltip("Set overall width multiplier for all nodes, used in mesh generation.");
	widthSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->width = args.fValue;
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->width = args.fValue;
		}
		editor->componentsWnd.RefreshEntityTree();
		});
	AddWidget(&widthSlider);

	rotSlider.Create(0, 360, 0, 360, "Rotation: ");
	rotSlider.SetTooltip("Set overall rotation for all nodes, used in mesh generation.");
	rotSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		float rad = wi::math::DegreesToRadians(args.fValue);
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->rotation = rad;
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->rotation = rad;
		}
		editor->componentsWnd.RefreshEntityTree();
		});
	AddWidget(&rotSlider);

	subdivSlider.Create(0, 100, 0, 100, "Mesh subdivision: ");
	subdivSlider.SetTooltip("Set subdivision count for mesh generation. \nIncreasing this above 0 will enable mesh generation and higher values mean higher quality.");
	subdivSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->mesh_generation_subdivision = args.iValue;
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->mesh_generation_subdivision = args.iValue;
		}
		editor->componentsWnd.RefreshEntityTree();
	});
	AddWidget(&subdivSlider);

	subdivVerticalSlider.Create(0, 36, 0, 36, "Vertical subdivision: ");
	subdivVerticalSlider.SetTooltip("Set subdivision count for mesh generation's vertical axis to create a corridoor or a tunnel.");
	subdivVerticalSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->mesh_generation_vertical_subdivision = args.iValue;
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->mesh_generation_vertical_subdivision = args.iValue;
		}
		editor->componentsWnd.RefreshEntityTree();
		});
	AddWidget(&subdivVerticalSlider);

	terrainSlider.Create(0, 1, 0, 100, "Terrain modifier: ");
	terrainSlider.SetTooltip("Set terrain modification strength (0 to turn it off, higher values mean more sharpness).");
	terrainSlider.OnSlide([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SplineComponent* spline = scene.splines.GetComponent(x.entity);
			if (spline == nullptr)
				continue;
			spline->terrain_modifier_amount = args.fValue;
		}

		// indirect set:
		SplineComponent* spline = scene.splines.GetComponent(entity);
		if (spline != nullptr)
		{
			spline->terrain_modifier_amount = args.fValue;
		}
		editor->componentsWnd.RefreshEntityTree();
	});
	AddWidget(&terrainSlider);

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
		widthSlider.SetValue(spline->width);
		rotSlider.SetValue(wi::math::RadiansToDegrees(spline->rotation));
		subdivSlider.SetValue(spline->mesh_generation_subdivision);
		subdivVerticalSlider.SetValue(spline->mesh_generation_vertical_subdivision);
		terrainSlider.SetValue(spline->terrain_modifier_amount);

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

	Scene& scene = editor->GetCurrentScene();
	SplineComponent* spline = scene.splines.GetComponent(entity);
	if (spline == nullptr)
		return;

	entries.reserve(spline->spline_node_entities.size());

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

	editor->generalWnd.themeCombo.SetSelected(editor->generalWnd.themeCombo.GetSelected());
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
	editor->ClearSelected();
	editor->AddSelected(node_entity);
	editor->componentsWnd.RefreshEntityTree();
}

void SplineWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 145;
	float margin_right = 40;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
		};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
		};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
		};

	add_fullwidth(infoLabel);
	add(subdivSlider);
	add(subdivVerticalSlider);
	add(terrainSlider);
	add(widthSlider);
	add(rotSlider);
	add_right(loopedCheck);
	add_right(alignedCheck);

	y += padding * 2;

	add_fullwidth(addButton);

	for (auto& entry : entries)
	{
		entry.removeButton.SetPos(XMFLOAT2(padding, y));

		entry.entityButton.SetSize(XMFLOAT2(width - padding * 3 - entry.removeButton.GetSize().x, entry.entityButton.GetSize().y));
		entry.entityButton.SetPos(XMFLOAT2(entry.removeButton.GetPos().x + entry.removeButton.GetSize().x + padding, y));

		y += entry.entityButton.GetSize().y;
		y += padding;
	}
}
