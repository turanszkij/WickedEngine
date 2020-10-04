#include "stdafx.h"
#include "DecalWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


DecalWindow::DecalWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	decalWindow = new wiWindow(GUI, "Decal Window");
	decalWindow->SetSize(XMFLOAT2(400, 200));
	GUI->AddWidget(decalWindow);

	float x = 200;
	float y = 5;
	float hei = 18;
	float step = hei + 2;

	placementCheckBox = new wiCheckBox("Decal Placement Enabled: ");
	placementCheckBox->SetPos(XMFLOAT2(x, y += step));
	placementCheckBox->SetSize(XMFLOAT2(hei, hei));
	placementCheckBox->SetCheck(false);
	placementCheckBox->SetTooltip("Enable decal placement. Use the left mouse button to place decals to the scene.");
	decalWindow->AddWidget(placementCheckBox);

	y += step;

	infoLabel = new wiLabel("");
	infoLabel->SetText("Selecting decals will select the according material. Set decal properties (texture, color, etc.) in the Material window.");
	infoLabel->SetSize(XMFLOAT2(400 - 20, 100));
	infoLabel->SetPos(XMFLOAT2(10, y));
	infoLabel->SetColor(wiColor::Transparent());
	decalWindow->AddWidget(infoLabel);
	y += infoLabel->GetScale().y - step + 5;

	decalNameField = new wiTextInputField("Decal Name");
	decalNameField->SetPos(XMFLOAT2(10, y+=step));
	decalNameField->SetSize(XMFLOAT2(300, hei));
	decalNameField->OnInputAccepted([&](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name != nullptr)
		{
			*name = args.sValue;
		}
	});
	decalWindow->AddWidget(decalNameField);

	decalWindow->Translate(XMFLOAT3(30, 30, 0));
	decalWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


DecalWindow::~DecalWindow()
{
	decalWindow->RemoveWidgets(true);
	GUI->RemoveWidget(decalWindow);
	delete decalWindow;
}

void DecalWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
	const DecalComponent* decal = scene.decals.GetComponent(entity);

	if (decal != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		decalNameField->SetValue(name.name);
		decalNameField->SetEnabled(true);
	}
	else
	{
		decalNameField->SetValue("No decal selected");
		decalNameField->SetEnabled(false);
	}
}
