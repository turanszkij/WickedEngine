#include "stdafx.h"
#include "IKWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


void IKWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Inverse Kinematics (IK) Window");
	SetSize(XMFLOAT2(400, 150));

	float x = 120;
	float y = 10;
	float siz = 200;
	float hei = 18;
	float step = hei + 2;

	createButton.Create("Create");
	createButton.SetTooltip("Create/Remove IK Component to selected entity");
	createButton.SetPos(XMFLOAT2(x, y += step));
	createButton.SetSize(XMFLOAT2(siz, hei));
	AddWidget(&createButton);

	targetCombo.Create("Target: ");
	targetCombo.SetSize(XMFLOAT2(siz, hei));
	targetCombo.SetPos(XMFLOAT2(x, y += step));
	targetCombo.SetEnabled(false);
	targetCombo.OnSelect([&](wiEventArgs args) {
		Scene& scene = wiScene::GetScene();
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
	disabledCheckBox.OnClick([=](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->SetDisabled(args.bValue);
		});
	AddWidget(&disabledCheckBox);

	chainLengthSlider.Create(0, 10, 0, 10, "Chain Length: ");
	chainLengthSlider.SetTooltip("How far the hierarchy chain is simulated backwards from this entity");
	chainLengthSlider.SetPos(XMFLOAT2(x, y += step));
	chainLengthSlider.SetSize(XMFLOAT2(siz, hei));
	chainLengthSlider.OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->chain_length = args.iValue;
		});
	AddWidget(&chainLengthSlider);

	iterationCountSlider.Create(0, 10, 1, 10, "Iteration Count: ");
	iterationCountSlider.SetTooltip("How many iterations to compute the inverse kinematics for. Higher values are slower but more accurate.");
	iterationCountSlider.SetPos(XMFLOAT2(x, y += step));
	iterationCountSlider.SetSize(XMFLOAT2(siz, hei));
	iterationCountSlider.OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->iteration_count = args.iValue;
		});
	AddWidget(&iterationCountSlider);

	Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 740, 150, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void IKWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
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

	const TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
	if (transform != nullptr)
	{
		createButton.SetEnabled(true);

		if (ik == nullptr)
		{
			createButton.SetText("Create");
			createButton.OnClick([=](wiEventArgs args) {
				wiScene::GetScene().inverse_kinematics.Create(entity).chain_length = 1;
				SetEntity(entity);
				});
		}
		else
		{
			createButton.SetText("Remove");
			createButton.OnClick([=](wiEventArgs args) {
				wiScene::GetScene().inverse_kinematics.Remove_KeepSorted(entity);
				SetEntity(entity);
				});
		}
	}
}
