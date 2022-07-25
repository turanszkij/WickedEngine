#include "stdafx.h"
#include "IKWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void IKWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Inverse Kinematics", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(400, 150));

	float x = 120;
	float y = 0;
	float siz = 140;
	float hei = 18;
	float step = hei + 2;

	targetCombo.Create("Target: ");
	targetCombo.SetSize(XMFLOAT2(siz, hei));
	targetCombo.SetPos(XMFLOAT2(x, y));
	targetCombo.SetEnabled(false);
	targetCombo.OnSelect([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		InverseKinematicsComponent* ik = scene.inverse_kinematics.GetComponent(entity);
		if (ik != nullptr)
		{
			if (args.iValue == 0)
			{
				ik->target = INVALID_ENTITY;
			}
			else
			{
				ik->target = scene.transforms.GetEntity(args.iValue - 1);
			}
		}
		});
	targetCombo.SetTooltip("Choose a target entity (with transform) that the IK will follow");
	AddWidget(&targetCombo);

	disabledCheckBox.Create("Disabled: ");
	disabledCheckBox.SetTooltip("Disable simulation.");
	disabledCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox.OnClick([=](wi::gui::EventArgs args) {
		editor->GetCurrentScene().inverse_kinematics.GetComponent(entity)->SetDisabled(args.bValue);
		});
	AddWidget(&disabledCheckBox);

	chainLengthSlider.Create(0, 10, 0, 10, "Chain Length: ");
	chainLengthSlider.SetTooltip("How far the hierarchy chain is simulated backwards from this entity");
	chainLengthSlider.SetPos(XMFLOAT2(x, y += step));
	chainLengthSlider.SetSize(XMFLOAT2(siz, hei));
	chainLengthSlider.OnSlide([&](wi::gui::EventArgs args) {
		editor->GetCurrentScene().inverse_kinematics.GetComponent(entity)->chain_length = args.iValue;
		});
	AddWidget(&chainLengthSlider);

	iterationCountSlider.Create(0, 10, 1, 10, "Iteration Count: ");
	iterationCountSlider.SetTooltip("How many iterations to compute the inverse kinematics for. Higher values are slower but more accurate.");
	iterationCountSlider.SetPos(XMFLOAT2(x, y += step));
	iterationCountSlider.SetSize(XMFLOAT2(siz, hei));
	iterationCountSlider.OnSlide([&](wi::gui::EventArgs args) {
		editor->GetCurrentScene().inverse_kinematics.GetComponent(entity)->iteration_count = args.iValue;
		});
	AddWidget(&iterationCountSlider);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void IKWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const InverseKinematicsComponent* ik = scene.inverse_kinematics.GetComponent(entity);

	if (ik != nullptr)
	{
		SetEnabled(true);

		disabledCheckBox.SetCheck(ik->IsDisabled());
		chainLengthSlider.SetValue((float)ik->chain_length);
		iterationCountSlider.SetValue((float)ik->iteration_count);

		targetCombo.ClearItems();
		targetCombo.AddItem("NO TARGET");
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			Entity entity = scene.transforms.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(entity);
			targetCombo.AddItem(name == nullptr ? std::to_string(entity) : name->name);

			if (ik->target == entity)
			{
				targetCombo.SetSelected((int)i + 1);
			}
		}
	}
	else
	{
		SetEnabled(false);
	}

}
