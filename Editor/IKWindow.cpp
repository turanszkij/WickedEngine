#include "stdafx.h"
#include "IKWindow.h"
#include "Editor.h"

using namespace wiECS;
using namespace wiScene;


IKWindow::IKWindow(EditorComponent* editor) : GUI(&editor->GetGUI())
{
	assert(GUI && "Invalid GUI!");

	window = new wiWindow(GUI, "Inverse Kinematics (IK) Window");
	window->SetSize(XMFLOAT2(460, 200));
	GUI->AddWidget(window);

	float x = 100;
	float y = 0;
	float step = 25;
	float siz = 200;
	float hei = 20;

	createButton = new wiButton("Create");
	createButton->SetTooltip("Create/Remove IK Component to selected entity");
	createButton->SetPos(XMFLOAT2(x, y += step));
	createButton->SetSize(XMFLOAT2(siz, hei));
	window->AddWidget(createButton);

	targetCombo = new wiComboBox("Target: ");
	targetCombo->SetSize(XMFLOAT2(siz, hei));
	targetCombo->SetPos(XMFLOAT2(x, y += step));
	targetCombo->SetEnabled(false);
	targetCombo->OnSelect([&](wiEventArgs args) {
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
	targetCombo->SetTooltip("Choose a target entity (with transform) that the IK will follow");
	window->AddWidget(targetCombo);

	disabledCheckBox = new wiCheckBox("Disabled: ");
	disabledCheckBox->SetTooltip("Disable simulation.");
	disabledCheckBox->SetPos(XMFLOAT2(x, y += step));
	disabledCheckBox->SetSize(XMFLOAT2(hei, hei));
	disabledCheckBox->OnClick([=](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->SetDisabled(args.bValue);
		});
	window->AddWidget(disabledCheckBox);

	chainLengthSlider = new wiSlider(0, 10, 0, 10, "Chain Length: ");
	chainLengthSlider->SetTooltip("How far the hierarchy chain is simulated backwards from this entity");
	chainLengthSlider->SetPos(XMFLOAT2(x, y += step));
	chainLengthSlider->SetSize(XMFLOAT2(siz, hei));
	chainLengthSlider->OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->chain_length = args.iValue;
		});
	window->AddWidget(chainLengthSlider);

	iterationCountSlider = new wiSlider(0, 10, 1, 10, "Iteration Count: ");
	iterationCountSlider->SetTooltip("How many iterations to compute the inverse kinematics for. Higher values are slower but more accurate.");
	iterationCountSlider->SetPos(XMFLOAT2(x, y += step));
	iterationCountSlider->SetSize(XMFLOAT2(siz, hei));
	iterationCountSlider->OnSlide([&](wiEventArgs args) {
		wiScene::GetScene().inverse_kinematics.GetComponent(entity)->iteration_count = args.iValue;
		});
	window->AddWidget(iterationCountSlider);

	window->Translate(XMFLOAT3((float)wiRenderer::GetDevice()->GetScreenWidth() - 740, 150, 0));
	window->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


IKWindow::~IKWindow()
{
	window->RemoveWidgets(true);
	GUI->RemoveWidget(window);
	delete window;
}

void IKWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
	const InverseKinematicsComponent* ik = scene.inverse_kinematics.GetComponent(entity);

	if (ik != nullptr)
	{
		window->SetEnabled(true);

		disabledCheckBox->SetCheck(ik->IsDisabled());
		chainLengthSlider->SetValue((float)ik->chain_length);
		iterationCountSlider->SetValue((float)ik->iteration_count);

		targetCombo->ClearItems();
		targetCombo->AddItem("NO TARGET");
		for (size_t i = 0; i < scene.transforms.GetCount(); ++i)
		{
			Entity entity = scene.transforms.GetEntity(i);
			const NameComponent* name = scene.names.GetComponent(entity);
			targetCombo->AddItem(name == nullptr ? std::to_string(entity) : name->name);

			if (ik->target == entity)
			{
				targetCombo->SetSelected((int)i + 1);
			}
		}
	}
	else
	{
		window->SetEnabled(false);
	}

	const TransformComponent* transform = wiScene::GetScene().transforms.GetComponent(entity);
	if (transform != nullptr)
	{
		createButton->SetEnabled(true);

		if (ik == nullptr)
		{
			createButton->SetText("Create");
			createButton->OnClick([=](wiEventArgs args) {
				wiScene::GetScene().inverse_kinematics.Create(entity).chain_length = 1;
				SetEntity(entity);
				});
		}
		else
		{
			createButton->SetText("Remove");
			createButton->OnClick([=](wiEventArgs args) {
				wiScene::GetScene().inverse_kinematics.Remove_KeepSorted(entity);
				SetEntity(entity);
				});
		}
	}
}
