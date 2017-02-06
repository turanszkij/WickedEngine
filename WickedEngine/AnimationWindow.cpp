#include "stdafx.h"
#include "AnimationWindow.h"


AnimationWindow::AnimationWindow(wiGUI* gui) :GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	animWindow = new wiWindow(GUI, "Animation Window");
	animWindow->SetSize(XMFLOAT2(700, 300));
	GUI->AddWidget(animWindow);

	float x = 200;
	float y = 0;
	float step = 35;

	actionsComboBox = new wiComboBox("Actions: ");
	actionsComboBox->SetSize(XMFLOAT2(400, 20));
	actionsComboBox->SetPos(XMFLOAT2(x, y += step));
	actionsComboBox->OnSelect([&](wiEventArgs args) {
		if (armature != nullptr && args.iValue >= 0)
		{
			armature->ChangeAction(args.sValue, blendSlider->GetValue());
		}
	});
	actionsComboBox->SetEnabled(false);
	actionsComboBox->SetTooltip("Choose an animation clip...");
	animWindow->AddWidget(actionsComboBox);

	blendSlider = new wiSlider(0.0f, 60, 0, 100000, "Blend Frames: ");
	blendSlider->SetSize(XMFLOAT2(100, 30));
	blendSlider->SetPos(XMFLOAT2(x, y += step));
	blendSlider->OnSlide([&](wiEventArgs args) {
		// no operation, will only be queried
	});
	blendSlider->SetEnabled(false);
	blendSlider->SetTooltip("Adjust the blend length in frames when changing actions...");
	animWindow->AddWidget(blendSlider);


	animWindow->Translate(XMFLOAT3(100, 50, 0));
	animWindow->SetVisible(false);

	SetArmature(nullptr);
}


AnimationWindow::~AnimationWindow()
{
	SAFE_DELETE(animWindow);
	SAFE_DELETE(actionsComboBox);
	SAFE_DELETE(blendSlider);
}

void AnimationWindow::SetArmature(Armature* armature)
{
	this->armature = armature;

	if (armature != nullptr)
	{
		actionsComboBox->ClearItems();
		actionsComboBox->SetEnabled(true);
		for (auto& x : armature->actions)
		{
			actionsComboBox->AddItem(x.name);
		}
		actionsComboBox->SetSelected(armature->GetPrimaryAnimation()->activeAction);
		blendSlider->SetEnabled(true);
	}
	else
	{
		actionsComboBox->ClearItems();
		actionsComboBox->SetEnabled(false);
		blendSlider->SetEnabled(false);
	}
}
