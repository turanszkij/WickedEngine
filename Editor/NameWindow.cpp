#include "stdafx.h"
#include "NameWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


void NameWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Name Window");
	SetSize(XMFLOAT2(360, 80));

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 280;
	float hei = 20;

	nameInput.Create("");
	nameInput.SetDescription("Name: ");
	nameInput.SetPos(XMFLOAT2(x, y += step));
	nameInput.SetSize(XMFLOAT2(siz, hei));
	nameInput.OnInputAccepted([=](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name == nullptr)
		{
			name = &wiScene::GetScene().names.Create(entity);
		}
		name->name = args.sValue;

		editor->RefreshSceneGraphView();
	});
	AddWidget(&nameInput);

	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 450, 200, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void NameWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		SetEnabled(true);

		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			nameInput.SetValue(name->name);
		}
	}
	else
	{
		SetEnabled(false);
		nameInput.SetValue("Select entity to modify name...");
	}
}
