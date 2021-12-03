#include "stdafx.h"
#include "EnvProbeWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void EnvProbeWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Environment Probe Window");
	SetSize(XMFLOAT2(300, 200));

	float x = 100, y = 5, step = 35;

	realTimeCheckBox.Create("RealTime: ");
	realTimeCheckBox.SetPos(XMFLOAT2(x, y += step));
	realTimeCheckBox.SetEnabled(false);
	realTimeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = wi::scene::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetRealTime(args.bValue);
			probe->SetDirty();
		}
	});
	AddWidget(&realTimeCheckBox);

	generateButton.Create("Put");
	generateButton.SetPos(XMFLOAT2(x, y += step));
	generateButton.OnClick([=](wi::gui::EventArgs args) {
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, XMVectorAdd(wi::scene::GetCamera().GetEye(), wi::scene::GetCamera().GetAt() * 4));
		Entity entity = wi::scene::GetScene().Entity_CreateEnvironmentProbe("editorProbe", pos);
		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
	});
	AddWidget(&generateButton);

	refreshButton.Create("Refresh");
	refreshButton.SetPos(XMFLOAT2(x, y += step));
	refreshButton.SetEnabled(false);
	refreshButton.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = wi::scene::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetDirty();
		}
	});
	AddWidget(&refreshButton);

	refreshAllButton.Create("Refresh All");
	refreshAllButton.SetPos(XMFLOAT2(x, y += step));
	refreshAllButton.SetEnabled(true);
	refreshAllButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = wi::scene::GetScene();
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = scene.probes[i];
			probe.SetDirty();
		}
	});
	AddWidget(&refreshAllButton);




	Translate(XMFLOAT3(100, 100, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void EnvProbeWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const EnvironmentProbeComponent* probe = wi::scene::GetScene().probes.GetComponent(entity);

	if (probe == nullptr)
	{
		realTimeCheckBox.SetEnabled(false);
		refreshButton.SetEnabled(false);
	}
	else
	{
		realTimeCheckBox.SetCheck(probe->IsRealTime());
		realTimeCheckBox.SetEnabled(true);
		refreshButton.SetEnabled(true);
	}
}

