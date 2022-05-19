#include "stdafx.h"
#include "TransformWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void TransformWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Transform Window");
	SetSize(XMFLOAT2(480, 200));

	float x = 100;
	float y = 0;
	float step = 25;
	float siz = 50;
	float hei = 20;

	createButton.Create("Create New Transform");
	createButton.SetTooltip("Create a new entity with only a trasform component");
	createButton.SetPos(XMFLOAT2(x, y));
	createButton.SetSize(XMFLOAT2(350, hei));
	createButton.OnClick([=](wi::gui::EventArgs args) {
		Entity entity = CreateEntity();
		wi::scene::GetScene().transforms.Create(entity);

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_ADD;
		editor->RecordSelection(archive);

		editor->ClearSelected();
		editor->AddSelected(entity);

		editor->RecordSelection(archive);
		editor->RecordAddedEntity(archive, entity);

		editor->RefreshSceneGraphView();
		SetEntity(entity);
		});
	AddWidget(&createButton);

	parentCombo.Create("Parent: ");
	parentCombo.SetSize(XMFLOAT2(330, hei));
	parentCombo.SetPos(XMFLOAT2(x, y += step));
	parentCombo.SetEnabled(false);
	parentCombo.OnSelect([&](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();

		if (args.iValue == 0)
		{
			scene.Component_Detach(entity);
		}
		else
		{
		    scene.Component_Attach(entity, (Entity)args.userdata);
		}

	});
	parentCombo.SetTooltip("Choose a parent entity (also works if selected entity has no transform)");
	AddWidget(&parentCombo);

	txInput.Create("");
	txInput.SetValue(0);
	txInput.SetDescription("Translation X: ");
	txInput.SetPos(XMFLOAT2(x, y += step));
	txInput.SetSize(XMFLOAT2(siz, hei));
	txInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.x = args.fValue;
			transform->SetDirty();
		}
	});
	AddWidget(&txInput);

	tyInput.Create("");
	tyInput.SetValue(0);
	tyInput.SetDescription("Translation Y: ");
	tyInput.SetPos(XMFLOAT2(x, y += step));
	tyInput.SetSize(XMFLOAT2(siz, hei));
	tyInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&tyInput);

	tzInput.Create("");
	tzInput.SetValue(0);
	tzInput.SetDescription("Translation Z: ");
	tzInput.SetPos(XMFLOAT2(x, y += step));
	tzInput.SetSize(XMFLOAT2(siz, hei));
	tzInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&tzInput);



	x = 250;
	y = step * 2;


	rxInput.Create("");
	rxInput.SetValue(0);
	rxInput.SetDescription("Rotation X: ");
	rxInput.SetPos(XMFLOAT2(x, y += step));
	rxInput.SetSize(XMFLOAT2(siz, hei));
	rxInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.x = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&rxInput);

	ryInput.Create("");
	ryInput.SetValue(0);
	ryInput.SetDescription("Rotation Y: ");
	ryInput.SetPos(XMFLOAT2(x, y += step));
	ryInput.SetSize(XMFLOAT2(siz, hei));
	ryInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&ryInput);

	rzInput.Create("");
	rzInput.SetValue(0);
	rzInput.SetDescription("Rotation Z: ");
	rzInput.SetPos(XMFLOAT2(x, y += step));
	rzInput.SetSize(XMFLOAT2(siz, hei));
	rzInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&rzInput);

	rwInput.Create("");
	rwInput.SetValue(1);
	rwInput.SetDescription("Rotation W: ");
	rwInput.SetPos(XMFLOAT2(x, y += step));
	rwInput.SetSize(XMFLOAT2(siz, hei));
	rwInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.w = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&rwInput);




	x = 400;
	y = step * 2;


	sxInput.Create("");
	sxInput.SetValue(1);
	sxInput.SetDescription("Scale X: ");
	sxInput.SetPos(XMFLOAT2(x, y += step));
	sxInput.SetSize(XMFLOAT2(siz, hei));
	sxInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.x = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&sxInput);

	syInput.Create("");
	syInput.SetValue(1);
	syInput.SetDescription("Scale Y: ");
	syInput.SetPos(XMFLOAT2(x, y += step));
	syInput.SetSize(XMFLOAT2(siz, hei));
	syInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&syInput);

	szInput.Create("");
	szInput.SetValue(1);
	szInput.SetDescription("Scale Z: ");
	szInput.SetPos(XMFLOAT2(x, y += step));
	szInput.SetSize(XMFLOAT2(siz, hei));
	szInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		TransformComponent* transform = wi::scene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&szInput);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 750, 100, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void TransformWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wi::scene::GetScene();
	const TransformComponent* transform = scene.transforms.GetComponent(entity);

	if (transform != nullptr)
	{

		txInput.SetValue(transform->translation_local.x);
		tyInput.SetValue(transform->translation_local.y);
		tzInput.SetValue(transform->translation_local.z);

		rxInput.SetValue(transform->rotation_local.x);
		ryInput.SetValue(transform->rotation_local.y);
		rzInput.SetValue(transform->rotation_local.z);
		rwInput.SetValue(transform->rotation_local.w);

		sxInput.SetValue(transform->scale_local.x);
		syInput.SetValue(transform->scale_local.y);
		szInput.SetValue(transform->scale_local.z);

		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}

	createButton.SetEnabled(true);

	parentCombo.SetEnabled(true);
	parentCombo.ClearItems();
	parentCombo.AddItem("NO PARENT");
	HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
	for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
	{
		Entity candidate_parent_entity = scene.transforms.GetEntity(i);
		if (candidate_parent_entity == entity)
		{
			continue; // Don't list selected (don't allow attach to self)
		}

		bool loop = false;
		for (size_t j = 0; j < scene.hierarchy.GetCount(); ++j)
		{
			if (scene.hierarchy[j].parentID == entity)
			{
				loop = true;
				break;
			}
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
