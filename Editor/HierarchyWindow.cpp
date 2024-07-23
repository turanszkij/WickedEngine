#include "stdafx.h"
#include "HierarchyWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void HierarchyWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_HIERARCHY " Hierarchy", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
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
	parentCombo.OnSelect([&](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		Scene& scene = editor->GetCurrentScene();
		if (args.iValue == 0)
		{
			scene.Component_Detach(entity);
		}
		else
		{
			scene.Component_Attach(entity, (Entity)args.userdata);
		}

		editor->RecordEntity(archive, entity);

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

	Scene& scene = editor->GetCurrentScene();

	entities.clear();
	scene.FindAllEntities(entities);

	parentCombo.ClearItems();
	parentCombo.AddItem("NO PARENT " ICON_DISABLED);
	HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
	for (auto candidate_parent_entity : entities)
	{
		if (candidate_parent_entity == entity)
		{
			continue; // Don't list selected (don't allow attach to self)
		}

		// Don't allow creating a loop:
		bool loop = false;
		const HierarchyComponent* candidate_hier = scene.hierarchy.GetComponent(candidate_parent_entity);
		while (candidate_hier != nullptr && loop == false)
		{
			if (candidate_hier->parentID == entity)
			{
				loop = true;
				break;
			}
			candidate_hier = scene.hierarchy.GetComponent(candidate_hier->parentID);
		}
		if (loop)
		{
			continue;
		}

		const NameComponent* name = scene.names.GetComponent(candidate_parent_entity);
		parentCombo.AddItem(name == nullptr ? std::to_string(candidate_parent_entity) : name->name, candidate_parent_entity);

		if (hier != nullptr && hier->parentID == candidate_parent_entity)
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
