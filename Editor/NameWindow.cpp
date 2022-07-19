#include "stdafx.h"
#include "NameWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void NameWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Name", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(360, 80));

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 280;
	float hei = 20;

	nameInput.Create("");
	nameInput.SetDescription("Name: ");
	nameInput.SetPos(XMFLOAT2(x, y));
	nameInput.SetSize(XMFLOAT2(siz, hei));
	nameInput.OnInputAccepted([=](wi::gui::EventArgs args) {
		NameComponent* name = editor->GetCurrentScene().names.GetComponent(entity);
		if (name == nullptr)
		{
			name = &editor->GetCurrentScene().names.Create(entity);
		}
		name->name = args.sValue;

		editor->RefreshEntityTree();
	});
	AddWidget(&nameInput);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void NameWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		SetEnabled(true);

		NameComponent* name = editor->GetCurrentScene().names.GetComponent(entity);
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
