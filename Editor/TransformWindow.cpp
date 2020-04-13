#include "stdafx.h"
#include "TransformWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


TransformWindow::TransformWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Transform Window");
	window->SetSize(XMFLOAT2(460, 150));
	GUI->AddWidget(window);

	float x = 100;
	float y = 0;
	float step = 25;
	float siz = 50;
	float hei = 20;

	createButton = new wiButton("Create New Transform");
	createButton->SetTooltip("Create a new entity with only a trasform component");
	createButton->SetPos(XMFLOAT2(x, y += step));
	createButton->SetSize(XMFLOAT2(350, hei));
	createButton->OnClick([=](wiEventArgs args) {
		Entity entity = CreateEntity();
		wiScene::GetScene().transforms.Create(entity);
		editor->ClearSelected();
		editor->AddSelected(entity);
		SetEntity(entity);
		});
	window->AddWidget(createButton);

	txInput = new wiTextInputField("");
	txInput->SetValue(0);
	txInput->SetDescription("Translation X: ");
	txInput->SetPos(XMFLOAT2(x, y += step));
	txInput->SetSize(XMFLOAT2(siz, hei));
	txInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.x = args.fValue;
			transform->SetDirty();
		}
	});
	window->AddWidget(txInput);

	tyInput = new wiTextInputField("");
	tyInput->SetValue(0);
	tyInput->SetDescription("Translation Y: ");
	tyInput->SetPos(XMFLOAT2(x, y += step));
	tyInput->SetSize(XMFLOAT2(siz, hei));
	tyInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(tyInput);

	tzInput = new wiTextInputField("");
	tzInput->SetValue(0);
	tzInput->SetDescription("Translation Z: ");
	tzInput->SetPos(XMFLOAT2(x, y += step));
	tzInput->SetSize(XMFLOAT2(siz, hei));
	tzInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->translation_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(tzInput);



	x = 250;
	y = step;


	rxInput = new wiTextInputField("");
	rxInput->SetValue(0);
	rxInput->SetDescription("Rotation X: ");
	rxInput->SetPos(XMFLOAT2(x, y += step));
	rxInput->SetSize(XMFLOAT2(siz, hei));
	rxInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.x = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(rxInput);

	ryInput = new wiTextInputField("");
	ryInput->SetValue(0);
	ryInput->SetDescription("Rotation Y: ");
	ryInput->SetPos(XMFLOAT2(x, y += step));
	ryInput->SetSize(XMFLOAT2(siz, hei));
	ryInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(ryInput);

	rzInput = new wiTextInputField("");
	rzInput->SetValue(0);
	rzInput->SetDescription("Rotation Z: ");
	rzInput->SetPos(XMFLOAT2(x, y += step));
	rzInput->SetSize(XMFLOAT2(siz, hei));
	rzInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(rzInput);

	rwInput = new wiTextInputField("");
	rwInput->SetValue(1);
	rwInput->SetDescription("Rotation W: ");
	rwInput->SetPos(XMFLOAT2(x, y += step));
	rwInput->SetSize(XMFLOAT2(siz, hei));
	rwInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->rotation_local.w = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(rwInput);




	x = 400;
	y = step;


	sxInput = new wiTextInputField("");
	sxInput->SetValue(1);
	sxInput->SetDescription("Scale X: ");
	sxInput->SetPos(XMFLOAT2(x, y += step));
	sxInput->SetSize(XMFLOAT2(siz, hei));
	sxInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.x = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(sxInput);

	syInput = new wiTextInputField("");
	syInput->SetValue(1);
	syInput->SetDescription("Scale Y: ");
	syInput->SetPos(XMFLOAT2(x, y += step));
	syInput->SetSize(XMFLOAT2(siz, hei));
	syInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.y = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(syInput);

	szInput = new wiTextInputField("");
	szInput->SetValue(1);
	szInput->SetDescription("Scale Z: ");
	szInput->SetPos(XMFLOAT2(x, y += step));
	szInput->SetSize(XMFLOAT2(siz, hei));
	szInput->OnInputAccepted([&](wiEventArgs args) {
		TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
		if (transform != nullptr)
		{
			transform->scale_local.z = args.fValue;
			transform->SetDirty();
		}
		});
	window->AddWidget(szInput);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 750, 100, 0));
	window->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


TransformWindow::~TransformWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void TransformWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);

	if (transform != nullptr)
	{
		txInput->SetValue(transform->translation_local.x);
		tyInput->SetValue(transform->translation_local.y);
		tzInput->SetValue(transform->translation_local.z);

		rxInput->SetValue(transform->rotation_local.x);
		ryInput->SetValue(transform->rotation_local.y);
		rzInput->SetValue(transform->rotation_local.z);
		rwInput->SetValue(transform->rotation_local.w);

		sxInput->SetValue(transform->scale_local.x);
		syInput->SetValue(transform->scale_local.y);
		szInput->SetValue(transform->scale_local.z);

		window->SetEnabled(true);
	}
	else
	{
		window->SetEnabled(false);
	}

	createButton->SetEnabled(true);
}
