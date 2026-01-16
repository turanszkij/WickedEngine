#include "stdafx.h"
#include "HierarchyWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void HierarchyWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_HIERARCHY " Hierarchy", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(480, 60));

	closeButton.SetTooltip("Delete HierarchyComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().Component_Detach(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 80;
	float xx = x;
	float y = 4;
	float step = 25;
	float siz = 50;
	float hei = 20;
	float wid = 200;

	parentCombo.Create("Parent: ");
	parentCombo.SetSize(XMFLOAT2(wid, hei));
	parentCombo.SetPos(XMFLOAT2(x, y));
	parentCombo.OnSelect([this](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			if (args.iValue == 0)
			{
				scene.Component_Detach(x.entity);
			}
			else
			{
				scene.Component_Attach(x.entity, (Entity)args.userdata);
			}
		}
		editor->componentsWnd.RefreshEntityTree();

	});
	parentCombo.SetTooltip("Choose a parent entity (also works if selected entity has no transform)");
	AddWidget(&parentCombo);



	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void HierarchyWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;
	this->entity = entity;

	const Scene& scene = editor->GetCurrentScene();

	entities.clear();
	scene.FindAllEntities(entities);

	parentCombo.ClearItems();
	parentCombo.AddItem("NO PARENT " ICON_DISABLED);

	const HierarchyComponent* hierarchy_component = scene.hierarchy.GetComponent(entity);
	for (const auto candidate_parent_entity : entities)
	{
		if (candidate_parent_entity == entity)
		{
			continue; // Skip this candidate (can't be its own parent)
		}

		// Skip candidates that are descendants of the selected entity to prevent circular hierarchy
		bool loop = false;
		const HierarchyComponent* candidate_hierarchy_component = scene.hierarchy.GetComponent(candidate_parent_entity);
		while (candidate_hierarchy_component != nullptr && loop == false)
		{
			if (candidate_hierarchy_component->parentID == entity)
			{
				loop = true;
				break;
			}
			candidate_hierarchy_component = scene.hierarchy.GetComponent(candidate_hierarchy_component->parentID);
		}
		if (loop)
		{
			continue; // Skip this candidate to avoid circular reference
		}

		const NameComponent* name = scene.names.GetComponent(candidate_parent_entity);
		std::string display_name = name == nullptr ? std::to_string(candidate_parent_entity) : name->name;

		// Add parent information with depth indicator
		const HierarchyComponent* candidate_hierarchy = scene.hierarchy.GetComponent(candidate_parent_entity);
		if (candidate_hierarchy != nullptr && candidate_hierarchy->parentID != INVALID_ENTITY)
		{
			// Calculate depth by traversing up the hierarchy
			int depth = 0;
			const HierarchyComponent* current_hierarchy = candidate_hierarchy;
			while (current_hierarchy != nullptr && current_hierarchy->parentID != INVALID_ENTITY)
			{
				depth++;
				current_hierarchy = scene.hierarchy.GetComponent(current_hierarchy->parentID);
			}

			std::string depth_indicator;
			if (depth > 3)
			{
				depth_indicator = ">" + std::to_string(depth);
			}
			else
			{
				for (int i = 0; i < depth; ++i)
				{
					depth_indicator += ">";
				}
			}

			const NameComponent* parent_name = scene.names.GetComponent(candidate_hierarchy->parentID);
			std::string parent_display = parent_name == nullptr ? std::to_string(candidate_hierarchy->parentID) : parent_name->name;
			display_name += " [Child of " + depth_indicator + " " + parent_display + "]";
		}

		parentCombo.AddItem(display_name, candidate_parent_entity);

		if (hierarchy_component != nullptr && hierarchy_component->parentID == candidate_parent_entity)
		{
			parentCombo.SetSelectedWithoutCallback((int)parentCombo.GetItemCount() - 1);
		}
	}
}

void HierarchyWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	
	parentCombo.SetPos(XMFLOAT2(60, 4));
	parentCombo.SetSize(XMFLOAT2(GetSize().x - 86, parentCombo.GetSize().y));
}
