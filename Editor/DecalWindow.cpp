#include "stdafx.h"
#include "DecalWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


DecalWindow::DecalWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	decalWindow = new wiWindow(GUI, "Decal Window");
	decalWindow->SetSize(XMFLOAT2(400, 300));
	GUI->AddWidget(decalWindow);

	float x = 200;
	float y = 0;

	decalNameField = new wiTextInputField("MaterialName");
	decalNameField->SetPos(XMFLOAT2(10, 30));
	decalNameField->SetSize(XMFLOAT2(300, 20));
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
