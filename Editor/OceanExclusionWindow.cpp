#include "stdafx.h"
#include "OceanExclusionWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

// Keep multi-selection edits in a single undo transaction.
template<typename NeedsEdit, typename Edit>
static void EditOceanExclusions(EditorComponent& editor, NeedsEdit needs_edit, Edit edit)
{
	Scene& scene = editor.GetCurrentScene();
	wi::vector<Entity> entities;
	for (const auto& selected : editor.translator.selected)
	{
		const OceanExclusionComponent* component = scene.ocean_exclusions.GetComponent(selected.entity);
		if (component != nullptr && needs_edit(*component) &&
			std::find(entities.begin(), entities.end(), selected.entity) == entities.end())
		{
			entities.push_back(selected.entity);
		}
	}
	if (entities.empty())
	{
		return;
	}
	wi::Archive& archive = editor.AdvanceHistory();
	archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
	editor.RecordEntity(archive, entities);
	for (Entity entity : entities)
	{
		edit(*scene.ocean_exclusions.GetComponent(entity));
	}
	editor.RecordEntity(archive, entities);
}

void OceanExclusionWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_OCEANEXCLUSION " Ocean Exclusion", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(420, 260));

	closeButton.SetTooltip("Delete OceanExclusionComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().ocean_exclusions.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 60;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 200;

	infoLabel.Create("");
	infoLabel.SetText("Removes the global ocean inside this volume. Use the transform gizmo to move, rotate and resize it. To open a hole in the surface, extend the volume through the waterline and down to the area you want dry.");
	infoLabel.SetFitTextEnabled(true);
	AddWidget(&infoLabel);

	shapeComboBox.Create("Shape: ");
	shapeComboBox.SetPos(XMFLOAT2(x, y));
	shapeComboBox.SetSize(XMFLOAT2(wid, hei));
	shapeComboBox.OnSelect([this](wi::gui::EventArgs args) {
		const auto shape = (OceanExclusionComponent::Shape)args.userdata;
		EditOceanExclusions(*editor,
			[shape](const OceanExclusionComponent& component) { return component.shape != shape; },
			[shape](OceanExclusionComponent& component) {
				if (shape == OceanExclusionComponent::Shape::Polygon && component.polygon.empty())
				{
					component.SetPolygon({ XMFLOAT2(-1, -1), XMFLOAT2(1, -1), XMFLOAT2(1, 1), XMFLOAT2(-1, 1) });
				}
				else
				{
					component.shape = shape;
					component.UpdateTransform(component.world);
				}
			});
		RefreshPolygonLabel();
	});
	shapeComboBox.AddItem("Box", (uint64_t)OceanExclusionComponent::Shape::Box);
	shapeComboBox.AddItem("Sphere", (uint64_t)OceanExclusionComponent::Shape::Sphere);
	shapeComboBox.AddItem("Cylinder", (uint64_t)OceanExclusionComponent::Shape::Cylinder);
	shapeComboBox.AddItem("Polygon", (uint64_t)OceanExclusionComponent::Shape::Polygon);
	shapeComboBox.SetEnabled(false);
	shapeComboBox.SetTooltip("Choose the shape of the volume, in its local space:\n- Box: from -1 to 1 on all axes\n- Sphere: radius of 1\n- Cylinder: along the Y axis, radius of 1, from -1 to 1 on Y\n- Polygon: an outline on the XZ plane, from -1 to 1 on Y");
	AddWidget(&shapeComboBox);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick([this](wi::gui::EventArgs args) {
		EditOceanExclusions(*editor,
			[&](const OceanExclusionComponent& component) { return component.IsDisabled() != args.bValue; },
			[&](OceanExclusionComponent& component) { component.SetDisabled(args.bValue); });
	});
	disabledCheckBox.SetEnabled(false);
	disabledCheckBox.SetTooltip("A disabled volume doesn't keep the ocean out.");
	AddWidget(&disabledCheckBox);

	polygonLabel.Create("");
	polygonLabel.SetFitTextEnabled(true);
	AddWidget(&polygonLabel);

	subdivisionSlider.Create(1, 32, 8, 31, "Spline subdivision: ");
	subdivisionSlider.SetPos(XMFLOAT2(x, y += step));
	subdivisionSlider.SetSize(XMFLOAT2(wid, hei));
	subdivisionSlider.SetTooltip("The number of outline points that are made for every node of the spline.");
	AddWidget(&subdivisionSlider);

	polygonFromSplineButton.Create("Polygon from spline");
	polygonFromSplineButton.SetPos(XMFLOAT2(x, y += step));
	polygonFromSplineButton.SetSize(XMFLOAT2(wid, hei));
	polygonFromSplineButton.SetTooltip("Sets the Polygon shape with an outline that follows a spline through its nodes, as a closed loop.\nThe spline is taken from this entity if it has a spline component, otherwise from an other selected entity:\nselect a spline together with this ocean exclusion, then press this button.\nThe outline is made once, it doesn't follow later changes of the spline.");
	polygonFromSplineButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		OceanExclusionComponent* exclusion = scene.ocean_exclusions.GetComponent(entity);
		if (exclusion == nullptr)
		{
			return;
		}

		// The spline of this entity, or of an other selected entity:
		Entity spline_entity = INVALID_ENTITY;
		if (scene.splines.Contains(entity))
		{
			spline_entity = entity;
		}
		else
		{
			for (auto& x : editor->translator.selected)
			{
				if (scene.splines.Contains(x.entity))
				{
					spline_entity = x.entity;
					break;
				}
			}
		}
		if (spline_entity == INVALID_ENTITY)
		{
			editor->PostSaveText("No spline:", "select a spline together with the ocean exclusion");
			return;
		}

		OceanExclusionComponent before = *exclusion;
		if (!scene.SetOceanExclusionPolygonFromSpline(entity, spline_entity, (int)subdivisionSlider.GetValue()))
		{
			editor->PostSaveText("Cannot create ocean exclusion outline", "Use at least 3 valid spline nodes, a nonzero volume scale and no more than 4096 sampled points.");
			return;
		}

		// Only record an undo operation if the spline was successfully baked.
		std::swap(before, *exclusion);
		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);
		std::swap(before, *exclusion);
		editor->RecordEntity(archive, entity);
		SetEntity(entity);
	});
	AddWidget(&polygonFromSplineButton);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void OceanExclusionWindow::RefreshPolygonLabel()
{
	const OceanExclusionComponent* exclusion = editor->GetCurrentScene().ocean_exclusions.GetComponent(entity);
	if (exclusion == nullptr)
	{
		polygonLabel.SetText("");
		return;
	}
	std::string text = "Polygon outline: " + std::to_string(exclusion->polygon.size()) + " points";
	if (exclusion->shape != OceanExclusionComponent::Shape::Polygon)
	{
		text += " (not used by this shape)";
	}
	else if (exclusion->polygon.size() < 3 || exclusion->polygon.size() > OceanExclusionComponent::max_polygon_points)
	{
		text += "\nUse 3–4096 points. Make a new outline from a spline, or from a script with SetPolygon().";
	}
	polygonLabel.SetText(text);
}

void OceanExclusionWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const OceanExclusionComponent* exclusion = editor->GetCurrentScene().ocean_exclusions.GetComponent(entity);

	if (exclusion != nullptr)
	{
		shapeComboBox.SetSelectedByUserdataWithoutCallback((uint64_t)exclusion->shape);
		disabledCheckBox.SetCheck(exclusion->IsDisabled());
		shapeComboBox.SetEnabled(true);
		disabledCheckBox.SetEnabled(true);
		polygonFromSplineButton.SetEnabled(true);
	}
	else
	{
		shapeComboBox.SetEnabled(false);
		disabledCheckBox.SetEnabled(false);
		polygonFromSplineButton.SetEnabled(false);
	}
	RefreshPolygonLabel();
}

void OceanExclusionWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 110;

	layout.add_fullwidth(infoLabel);
	layout.add(shapeComboBox);
	layout.add_right(disabledCheckBox);
	layout.add_fullwidth(polygonLabel);
	layout.add(subdivisionSlider);
	layout.add_fullwidth(polygonFromSplineButton);
}
