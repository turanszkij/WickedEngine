#include "stdafx.h"
#include "DecalWindow.h"

using namespace wiECS;
using namespace wiSceneSystem;


DecalWindow::DecalWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	decalWindow = new wiWindow(GUI, "Decal Window");
	decalWindow->SetSize(XMFLOAT2(400, 300));
	decalWindow->SetEnabled(false);
	GUI->AddWidget(decalWindow);

	float x = 200;
	float y = 0;

	decalNameField = new wiTextInputField("MaterialName");
	decalNameField->SetPos(XMFLOAT2(10, 30));
	decalNameField->SetSize(XMFLOAT2(300, 20));
	decalNameField->OnInputAccepted([&](wiEventArgs args) {
		NameComponent* name = wiSceneSystem::GetScene().names.GetComponent(entity);
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
	SAFE_DELETE(decalWindow);
}

void DecalWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wiSceneSystem::GetScene();
	const DecalComponent* decal = scene.decals.GetComponent(entity);

	if (decal != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		decalNameField->SetValue(name.name);
		decalWindow->SetEnabled(true);
	}
	else
	{
		decalNameField->SetValue("No decal selected");
		decalWindow->SetEnabled(false);
	}
}
