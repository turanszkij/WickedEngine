#include "stdafx.h"
#include "DecalWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void DecalWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Decal", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(300, 150));

	closeButton.SetTooltip("Delete DecalComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().decals.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->RefreshEntityTree();
		});

	float x = 200;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	placementCheckBox.Create("Decal Placement Enabled: ");
	placementCheckBox.SetPos(XMFLOAT2(x, y));
	placementCheckBox.SetSize(XMFLOAT2(hei, hei));
	placementCheckBox.SetCheck(false);
	placementCheckBox.SetTooltip("Enable decal placement. Use the left mouse button to place decals to the scene.");
	AddWidget(&placementCheckBox);

	y += step;

	infoLabel.Create("");
	infoLabel.SetText("Selecting decals will select the according material. Set decal properties (texture, color, etc.) in the Material window.");
	infoLabel.SetSize(XMFLOAT2(300, 100));
	infoLabel.SetPos(XMFLOAT2(10, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);
	y += infoLabel.GetScale().y - step + 5;

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void DecalWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const DecalComponent* decal = scene.decals.GetComponent(entity);

	if (decal != nullptr)
	{
		SetEnabled(true);
	}
	else
	{
		SetEnabled(false);
	}
}
