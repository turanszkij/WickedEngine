#include "stdafx.h"
#include "NameWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


NameWindow::NameWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Name Window");
	window->SetSize(XMFLOAT2(360, 80));
	GUI->AddWidget(window);

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 280;
	float hei = 20;

	nameInput = new wiTextInputField("");
	nameInput->SetDescription("Name: ");
	nameInput->SetPos(XMFLOAT2(x, y += step));
	nameInput->SetSize(XMFLOAT2(siz, hei));
	nameInput->OnInputAccepted([&](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name == nullptr)
		{
			name = &wiScene::GetScene().names.Create(entity);
		}
		name->name = args.sValue;
	});
	window->AddWidget(nameInput);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 450, 200, 0));
	window->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


NameWindow::~NameWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void NameWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		window->SetEnabled(true);

		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			nameInput->SetValue(name->name);
		}
	}
	else
	{
		window->SetEnabled(false);
		nameInput->SetValue("Select entity to modify name...");
	}
}
