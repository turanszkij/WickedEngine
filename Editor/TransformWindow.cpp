#include "stdafx.h"
#include "TransformWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


void TransformWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Transform Window");
	SetSize(XMFLOAT2(460, 180));

	float x = 100;
	float y = 0;
	float step = 25;
	float siz = 50;
	float hei = 20;

	createButton.Create("Create New Transform");
	createButton.SetTooltip("Create a new entity with only a trasform component");
	createButton.SetPos(XMFLOAT2(x, y += step));
	createButton.SetSize(XMFLOAT2(350, hei));
	createButton.OnClick([=](wiEventArgs args) {
		Entity entity = CreateEntity();
		wiScene::GetScene().transforms.Create(entity);
		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
		});
	AddWidget(&createButton);

	parentCombo.Create("Parent: ");
	parentCombo.SetSize(XMFLOAT2(330, hei));
	parentCombo.SetPos(XMFLOAT2(x, y += step));
	parentCombo.SetEnabled(false);
	parentCombo.OnSelect([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
		HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);

		if (args.iValue == 0 && hier != nullptr)
		{
			scene.hierarchy.Remove_KeepSorted(entity);
		}
		else if(args.iValue != 0)
		{
			if (hier == nullptr)
			{
				hier = &scene.hierarchy.Create(entity);
			}
			hier->parentID = scene.transforms.GetEntity(args.iValue - 1);
			if (hier->parentID == entity)
			{
				scene.hierarchy.Remove_KeepSorted(entity);
			}
		}

		});
	parentCombo.SetTooltip("Choose a parent entity for the transform");
	AddWidget(&parentCombo);

	txInput.Create("");
	txInput.SetValue(0);
	txInput.SetDescription("Translation X: ");
	txInput.SetPos(XMFLOAT2(x, y += step));
	txInput.SetSize(XMFLOAT2(siz, hei));
	txInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	tyInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	tzInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	rxInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	ryInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	rzInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	rwInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	sxInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	syInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
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
	szInput.OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	AddWidget(&szInput);

	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 750, 100, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void TransformWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
	const TransformComponent* transform = scene.transforms.GetComponent(entity);

	if (transform != nullptr)
	{
		parentCombo.ClearItems();
		parentCombo.AddItem("NO PARENT");

		HierarchyComponent* hier = scene.hierarchy.GetComponent(entity);
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			Entity entity = scene.transforms.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(entity);
			parentCombo.AddItem(name == nullptr ? std::to_string(entity) : name->name);

			if (hier != nullptr && hier->parentID == entity)
			{
				parentCombo.SetSelected((int)i + 1);
			}
		}

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
}
