#include "stdafx.h"
#include "EnvProbeWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;

void EnvProbeWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Environment Probe Window");
	SetSize(XMFLOAT2(300, 200));

	float x = 100, y = 5, step = 35;

	realTimeCheckBox.Create("RealTime: ");
	realTimeCheckBox.SetPos(XMFLOAT2(x, y += step));
	realTimeCheckBox.SetEnabled(false);
	realTimeCheckBox.OnClick([&](wiEventArgs args) {
		EnvironmentProbeComponent* probe = wiScene::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetRealTime(args.bValue);
			probe->SetDirty();
		}
	});
	AddWidget(&realTimeCheckBox);

	generateButton.Create("Put");
	generateButton.SetPos(XMFLOAT2(x, y += step));
	generateButton.OnClick([=](wiEventArgs args) {
		XMFLOAT3 pos;
		XMStoreFloat3(&pos, XMVectorAdd(wiRenderer::GetCamera().GetEye(), wiRenderer::GetCamera().GetAt() * 4));
		Entity entity = wiScene::GetScene().Entity_CreateEnvironmentProbe("editorProbe", pos);
		editor->ClearSelected();
		editor->AddSelected(entity);
		editor->RefreshSceneGraphView();
		SetEntity(entity);
	});
	AddWidget(&generateButton);

	refreshButton.Create("Refresh");
	refreshButton.SetPos(XMFLOAT2(x, y += step));
	refreshButton.SetEnabled(false);
	refreshButton.OnClick([&](wiEventArgs args) {
		EnvironmentProbeComponent* probe = wiScene::GetScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetDirty();
		}
	});
	AddWidget(&refreshButton);

	refreshAllButton.Create("Refresh All");
	refreshAllButton.SetPos(XMFLOAT2(x, y += step));
	refreshAllButton.SetEnabled(true);
	refreshAllButton.OnClick([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
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

	const EnvironmentProbeComponent* probe = wiScene::GetScene().probes.GetComponent(entity);

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

