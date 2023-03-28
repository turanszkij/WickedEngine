#include "stdafx.h"
#include "NameWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void NameWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_NAME " Name", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(360, 60));

	closeButton.SetTooltip("Delete NameComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().names.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 0;
	float step = 25;
	float siz = 250;
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

		editor->optionsWnd.RefreshEntityTree();
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

void NameWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	nameInput.SetPos(XMFLOAT2(60, 4));
	nameInput.SetSize(XMFLOAT2(GetSize().x - 65, nameInput.GetSize().y));
}
