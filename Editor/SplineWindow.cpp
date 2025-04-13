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
	infoLabel.SetSize(XMFLOAT2(100, 90));
	infoLabel.SetText("The spline is a curve that goes through every specified entity that has a TransformComponent smoothly.");
	AddWidget(&infoLabel);

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

	subdivSlider.Create(0, 100, 0, 100, "Mesh subdivision: ");
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

	addButton.Create("AddNode");
	addButton.SetText("+");
	addButton.SetTooltip("Add an entity as a node to the spline (it must have TransformComponent)");
	addButton.OnClick([this](wi::gui::EventArgs args) {
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
		subdivSlider.SetValue(spline->mesh_generation_subdivision);

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
