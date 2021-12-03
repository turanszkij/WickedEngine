#include "stdafx.h"
#include "DecalWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void DecalWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Decal Window");
	SetSize(XMFLOAT2(400, 200));

	float x = 200;
	float y = 5;
	float hei = 18;
	float step = hei + 2;

	placementCheckBox.Create("Decal Placement Enabled: ");
	placementCheckBox.SetPos(XMFLOAT2(x, y += step));
	placementCheckBox.SetSize(XMFLOAT2(hei, hei));
	placementCheckBox.SetCheck(false);
	placementCheckBox.SetTooltip("Enable decal placement. Use the left mouse button to place decals to the scene.");
	AddWidget(&placementCheckBox);

	y += step;

	infoLabel.Create("");
	infoLabel.SetText("Selecting decals will select the according material. Set decal properties (texture, color, etc.) in the Material window.");
	infoLabel.SetSize(XMFLOAT2(400 - 20, 100));
	infoLabel.SetPos(XMFLOAT2(10, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);
	y += infoLabel.GetScale().y - step + 5;

	decalNameField.Create("Decal Name");
	decalNameField.SetPos(XMFLOAT2(10, y+=step));
	decalNameField.SetSize(XMFLOAT2(300, hei));
	decalNameField.OnInputAccepted([=](wi::gui::EventArgs args) {
		NameComponent* name = wi::scene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;

			editor->RefreshSceneGraphView();
		}
	});
	AddWidget(&decalNameField);

	Translate(XMFLOAT3(30, 30, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void DecalWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wi::scene::GetScene();
	const DecalComponent* decal = scene.decals.GetComponent(entity);

	if (decal != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		decalNameField.SetValue(name.name);
		decalNameField.SetEnabled(true);
	}
	else
	{
		decalNameField.SetValue("No decal selected");
		decalNameField.SetEnabled(false);
	}
}
