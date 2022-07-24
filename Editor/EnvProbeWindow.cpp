#include "stdafx.h"
#include "EnvProbeWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void EnvProbeWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Environment Probe", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(420, 220));

	float x = 5, y = 0, step = 35;

	infoLabel.Create("");
	infoLabel.SetText("Environment probes can be used to capture the scene from a specific location in a 360 degrees panorama. The probes will be used for reflections fallback, where a better reflection type is not available. The probes can affect the ambient colors slightly.\nTip: You can scale, rotate and move the probes to set up parallax correct rendering to affect a specific area only. The parallax correction will take effect inside the probe's bounds (indicated with a cyan colored box).");
	infoLabel.SetSize(XMFLOAT2(300, 100));
	infoLabel.SetPos(XMFLOAT2(x, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);
	y += infoLabel.GetScale().y + 5;

	realTimeCheckBox.Create("RealTime: ");
	realTimeCheckBox.SetTooltip("Enable continuous rendering of the probe in every frame.");
	realTimeCheckBox.SetPos(XMFLOAT2(x + 100, y));
	realTimeCheckBox.SetEnabled(false);
	realTimeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetRealTime(args.bValue);
			probe->SetDirty();
		}
	});
	AddWidget(&realTimeCheckBox);

	msaaCheckBox.Create("MSAA: ");
	msaaCheckBox.SetTooltip("Enable Multi Sampling Anti Aliasing for the probe, this will improve its quality.");
	msaaCheckBox.SetPos(XMFLOAT2(x + 200, y));
	msaaCheckBox.SetEnabled(false);
	msaaCheckBox.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetMSAA(args.bValue);
			probe->SetDirty();
		}
		});
	AddWidget(&msaaCheckBox);

	refreshButton.Create("Refresh");
	refreshButton.SetTooltip("Re-renders the selected probe.");
	refreshButton.SetPos(XMFLOAT2(x, y+= step));
	refreshButton.SetEnabled(false);
	refreshButton.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetDirty();
		}
	});
	AddWidget(&refreshButton);

	refreshAllButton.Create("Refresh All");
	refreshAllButton.SetTooltip("Re-renders all probes in the scene.");
	refreshAllButton.SetPos(XMFLOAT2(x + 120, y));
	refreshAllButton.SetEnabled(true);
	refreshAllButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = scene.probes[i];
			probe.SetDirty();
		}
	});
	AddWidget(&refreshAllButton);




	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void EnvProbeWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);

	if (probe == nullptr)
	{
		realTimeCheckBox.SetEnabled(false);
		msaaCheckBox.SetEnabled(false);
		refreshButton.SetEnabled(false);
	}
	else
	{
		realTimeCheckBox.SetCheck(probe->IsRealTime());
		realTimeCheckBox.SetEnabled(true);
		msaaCheckBox.SetCheck(probe->IsMSAA());
		msaaCheckBox.SetEnabled(true);
		refreshButton.SetEnabled(true);
	}
}

