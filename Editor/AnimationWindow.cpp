#include "stdafx.h"
#include "AnimationWindow.h"

using namespace wiECS;
using namespace wiSceneSystem;

AnimationWindow::AnimationWindow(wiGUI* gui) :GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	animWindow = new wiWindow(GUI, "Animation Window");
	animWindow->SetSize(XMFLOAT2(600, 450));
	GUI->AddWidget(animWindow);

	float x = 200;
	float y = 0;
	float step = 35;

	////////////////////////////////////

	//actionsComboBox = new wiComboBox("Primary Action: ");
	//actionsComboBox->SetSize(XMFLOAT2(300, 20));
	//actionsComboBox->SetPos(XMFLOAT2(x, y += step));
	//actionsComboBox->OnSelect([&](wiEventArgs args) {
	//	if (armature != nullptr && args.iValue >= 0)
	//	{
	//		armature->ChangeAction(args.sValue, blendSlider->GetValue());
	//	}
	//});
	//actionsComboBox->SetEnabled(false);
	//actionsComboBox->SetTooltip("Choose an animation clip...");
	//animWindow->AddWidget(actionsComboBox);

	//blendSlider = new wiSlider(0.0f, 60, 0, 100000, "Blend Frames: ");
	//blendSlider->SetSize(XMFLOAT2(100, 30));
	//blendSlider->SetPos(XMFLOAT2(x, y += step));
	//blendSlider->OnSlide([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer = armature->GetPrimaryAnimation();
	//		if (layer != nullptr)
	//		{
	//			layer->blendFrames = args.fValue;
	//		}
	//	}
	//});
	//blendSlider->SetEnabled(false);
	//blendSlider->SetTooltip("Adjust the blend length in frames when changing actions...");
	//animWindow->AddWidget(blendSlider);

	//loopedCheckBox = new wiCheckBox("Looped: ");
	//loopedCheckBox->SetTooltip("Toggle animation looping behaviour.");
	//loopedCheckBox->SetPos(XMFLOAT2(470, y));
	//loopedCheckBox->OnClick([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer = armature->GetPrimaryAnimation();
	//		if (layer != nullptr)
	//		{
	//			layer->looped = args.bValue;
	//		}
	//	}
	//});
	//animWindow->AddWidget(loopedCheckBox);

	///////////////////////////////////////


	//actionsComboBox1 = new wiComboBox("Secondary Action 1: ");
	//actionsComboBox1->SetSize(XMFLOAT2(300, 20));
	//actionsComboBox1->SetPos(XMFLOAT2(x, y += step*2));
	//actionsComboBox1->OnSelect([&](wiEventArgs args) {
	//	if (armature != nullptr && args.iValue >= 0)
	//	{
	//		armature->ChangeAction(args.sValue, blendSlider1->GetValue(), "SECONDARY1", weightSlider1->GetValue());
	//	}
	//});
	//actionsComboBox1->SetEnabled(false);
	//actionsComboBox1->SetTooltip("Choose an animation clip for a secondary slot...");
	//animWindow->AddWidget(actionsComboBox1);

	//blendSlider1 = new wiSlider(0.0f, 60, 0, 100000, "Blend Frames 1: ");
	//blendSlider1->SetSize(XMFLOAT2(100, 30));
	//blendSlider1->SetPos(XMFLOAT2(x, y += step));
	//blendSlider1->OnSlide([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer1 = armature->GetAnimLayer("SECONDARY1");
	//		if (layer1 != nullptr)
	//		{
	//			layer1->blendFrames = args.fValue;
	//		}
	//	}
	//});
	//blendSlider1->SetEnabled(false);
	//blendSlider1->SetTooltip("Adjust the blend length in frames when changing actions...");
	//animWindow->AddWidget(blendSlider1);

	//loopedCheckBox1 = new wiCheckBox("Looped: ");
	//loopedCheckBox1->SetTooltip("Toggle animation looping behaviour.");
	//loopedCheckBox1->SetPos(XMFLOAT2(470, y));
	//loopedCheckBox1->OnClick([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer1 = armature->GetAnimLayer("SECONDARY1");
	//		if (layer1 != nullptr)
	//		{
	//			layer1->looped = args.bValue;
	//		}
	//	}
	//});
	//animWindow->AddWidget(loopedCheckBox1);

	//weightSlider1 = new wiSlider(0.0f, 1.0f, 1.0f, 100000, "Blend Weight 1: ");
	//weightSlider1->SetSize(XMFLOAT2(100, 30));
	//weightSlider1->SetPos(XMFLOAT2(x, y += step));
	//weightSlider1->OnSlide([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer1 = armature->GetAnimLayer("SECONDARY1");
	//		if (layer1 != nullptr)
	//		{
	//			layer1->weight = args.fValue;
	//		}
	//	}
	//});
	//weightSlider1->SetEnabled(false);
	//weightSlider1->SetTooltip("Adjust the blend weight for the secondary action...");
	//animWindow->AddWidget(weightSlider1);

	///////////////////////////////////////


	//actionsComboBox2 = new wiComboBox("Secondary Action 2: ");
	//actionsComboBox2->SetSize(XMFLOAT2(300, 20));
	//actionsComboBox2->SetPos(XMFLOAT2(x, y += step * 2));
	//actionsComboBox2->OnSelect([&](wiEventArgs args) {
	//	if (armature != nullptr && args.iValue >= 0)
	//	{
	//		armature->ChangeAction(args.sValue, blendSlider2->GetValue(), "SECONDARY2", weightSlider2->GetValue());
	//	}
	//});
	//actionsComboBox2->SetEnabled(false);
	//actionsComboBox2->SetTooltip("Choose an animation clip for a secondary slot...");
	//animWindow->AddWidget(actionsComboBox2);

	//blendSlider2 = new wiSlider(0.0f, 60, 0, 100000, "Blend Frames 2: ");
	//blendSlider2->SetSize(XMFLOAT2(100, 30));
	//blendSlider2->SetPos(XMFLOAT2(x, y += step));
	//blendSlider2->OnSlide([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer2 = armature->GetAnimLayer("SECONDARY2");
	//		if (layer2 != nullptr)
	//		{
	//			layer2->blendFrames = args.fValue;
	//		}
	//	}
	//});
	//blendSlider2->SetEnabled(false);
	//blendSlider2->SetTooltip("Adjust the blend length in frames when changing actions...");
	//animWindow->AddWidget(blendSlider2);

	//loopedCheckBox2 = new wiCheckBox("Looped: ");
	//loopedCheckBox2->SetTooltip("Toggle animation looping behaviour.");
	//loopedCheckBox2->SetPos(XMFLOAT2(470, y));
	//loopedCheckBox2->OnClick([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer2 = armature->GetAnimLayer("SECONDARY2");
	//		if (layer2 != nullptr)
	//		{
	//			layer2->looped = args.bValue;
	//		}
	//	}
	//});
	//animWindow->AddWidget(loopedCheckBox2);

	//weightSlider2 = new wiSlider(0.0f, 1.0f, 1.0f, 100000, "Blend Weight 2: ");
	//weightSlider2->SetSize(XMFLOAT2(100, 30));
	//weightSlider2->SetPos(XMFLOAT2(x, y += step));
	//weightSlider2->OnSlide([&](wiEventArgs args) {
	//	if (armature != nullptr)
	//	{
	//		AnimationLayer* layer2 = armature->GetAnimLayer("SECONDARY2");
	//		if (layer2 != nullptr)
	//		{
	//			layer2->weight = args.fValue;
	//		}
	//	}
	//});
	//weightSlider2->SetEnabled(false);
	//weightSlider2->SetTooltip("Adjust the blend weight for the secondary action...");
	//animWindow->AddWidget(weightSlider2);


	/////////////////////////////////////////////////////


	//animWindow->Translate(XMFLOAT3(100, 50, 0));
	//animWindow->SetVisible(false);

	//SetArmature(nullptr);
}


AnimationWindow::~AnimationWindow()
{
	animWindow->RemoveWidgets(true);

	GUI->RemoveWidget(animWindow);
	SAFE_DELETE(animWindow);
}

void AnimationWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	//if (this->armature == armature)
	//	return;

	//this->armature = armature;

	//if (armature != nullptr)
	//{
	//	actionsComboBox->ClearItems();
	//	actionsComboBox->SetEnabled(true);
	//	for (auto& x : armature->actions)
	//	{
	//		actionsComboBox->AddItem(x.name);
	//	}
	//	actionsComboBox->SetSelected(armature->GetPrimaryAnimation()->activeAction);
	//	blendSlider->SetEnabled(true);
	//	AnimationLayer* layer = armature->GetPrimaryAnimation();
	//	if (layer != nullptr)
	//	{
	//		loopedCheckBox->SetCheck(layer->looped);
	//	}
	//	loopedCheckBox->SetEnabled(true);

	//	actionsComboBox1->ClearItems();
	//	actionsComboBox1->SetEnabled(true);
	//	for (auto& x : armature->actions)
	//	{
	//		actionsComboBox1->AddItem(x.name);
	//	}
	//	actionsComboBox1->SetSelected(0);
	//	blendSlider1->SetEnabled(true);
	//	weightSlider1->SetEnabled(true);
	//	AnimationLayer* layer1 = armature->GetAnimLayer("SECONDARY1");
	//	if (layer1 != nullptr)
	//	{
	//		loopedCheckBox1->SetCheck(layer1->looped);
	//	}
	//	loopedCheckBox1->SetEnabled(true);

	//	actionsComboBox2->ClearItems();
	//	actionsComboBox2->SetEnabled(true);
	//	for (auto& x : armature->actions)
	//	{
	//		actionsComboBox2->AddItem(x.name);
	//	}
	//	actionsComboBox2->SetSelected(0);
	//	blendSlider2->SetEnabled(true);
	//	weightSlider2->SetEnabled(true);
	//	AnimationLayer* layer2 = armature->GetAnimLayer("SECONDARY2");
	//	if (layer2 != nullptr)
	//	{
	//		loopedCheckBox2->SetCheck(layer2->looped);
	//	}
	//	loopedCheckBox2->SetEnabled(true);
	//}
	//else
	//{
	//	actionsComboBox->ClearItems();
	//	actionsComboBox->SetEnabled(false);
	//	blendSlider->SetEnabled(false);
	//	loopedCheckBox->SetEnabled(false);

	//	actionsComboBox1->ClearItems();
	//	actionsComboBox1->SetEnabled(false);
	//	blendSlider1->SetEnabled(false);
	//	weightSlider1->SetEnabled(false);
	//	loopedCheckBox1->SetEnabled(false);

	//	actionsComboBox2->ClearItems();
	//	actionsComboBox2->SetEnabled(false);
	//	blendSlider2->SetEnabled(false);
	//	weightSlider2->SetEnabled(false);
	//	loopedCheckBox2->SetEnabled(false);
	//}
}
