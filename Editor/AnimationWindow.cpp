#include "stdafx.h"
#include "AnimationWindow.h"
#include "Editor.h"

#include <sstream>

using namespace wiECS;
using namespace wiScene;

void AnimationWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Animation Window");
	SetSize(XMFLOAT2(520, 140));

	float x = 140;
	float y = 10;
	float hei = 18;
	float step = hei + 2;


	animationsComboBox.Create("Animation: ");
	animationsComboBox.SetSize(XMFLOAT2(300, hei));
	animationsComboBox.SetPos(XMFLOAT2(x, y += step));
	animationsComboBox.SetEnabled(false);
	animationsComboBox.OnSelect([&](wiEventArgs args) {
		entity = wiScene::GetScene().animations.GetEntity(args.iValue);
	});
	animationsComboBox.SetTooltip("Choose an animation clip...");
	AddWidget(&animationsComboBox);

	loopedCheckBox.Create("Looped: ");
	loopedCheckBox.SetTooltip("Toggle animation looping behaviour.");
	loopedCheckBox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckBox.SetPos(XMFLOAT2(150, y += step));
	loopedCheckBox.OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->SetLooped(args.bValue);
		}
	});
	AddWidget(&loopedCheckBox);

	playButton.Create("Play");
	playButton.SetTooltip("Play/Pause animation.");
	playButton.SetSize(XMFLOAT2(100, hei));
	playButton.SetPos(XMFLOAT2(200, y));
	playButton.OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->IsPlaying())
			{
				animation->Pause();
			}
			else
			{
				animation->Play();
				animation->amount = 0;
			}
		}
	});
	AddWidget(&playButton);

	stopButton.Create("Stop");
	stopButton.SetTooltip("Stop animation.");
	stopButton.SetSize(XMFLOAT2(100, hei));
	stopButton.SetPos(XMFLOAT2(310, y));
	stopButton.OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->Stop();
		}
	});
	AddWidget(&stopButton);

	timerSlider.Create(0, 1, 0, 100000, "Timer: ");
	timerSlider.SetSize(XMFLOAT2(250, hei));
	timerSlider.SetPos(XMFLOAT2(x, y += step));
	timerSlider.OnSlide([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = args.fValue;
		}
	});
	timerSlider.SetEnabled(false);
	timerSlider.SetTooltip("Set the animation timer by hand.");
	AddWidget(&timerSlider);

	amountSlider.Create(0, 1, 0, 100000, "Amount: ");
	amountSlider.SetSize(XMFLOAT2(250, hei));
	amountSlider.SetPos(XMFLOAT2(x, y += step));
	amountSlider.OnSlide([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->amount = args.fValue;
		}
	});
	amountSlider.SetEnabled(false);
	amountSlider.SetTooltip("Set the animation blending amount by hand.");
	AddWidget(&amountSlider);

	speedSlider.Create(0, 4, 1, 100000, "Speed: ");
	speedSlider.SetSize(XMFLOAT2(250, hei));
	speedSlider.SetPos(XMFLOAT2(x, y += step));
	speedSlider.OnSlide([&](wiEventArgs args) {
		AnimationComponent* animation = wiScene::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->speed = args.fValue;
		}
	});
	speedSlider.SetEnabled(false);
	speedSlider.SetTooltip("Set the animation speed.");
	AddWidget(&speedSlider);



	Translate(XMFLOAT3(100, 50, 0));
	SetVisible(false);

}

void AnimationWindow::Update()
{
	animationsComboBox.ClearItems();
	
	Scene& scene = wiScene::GetScene();

	if (!scene.animations.Contains(entity))
	{
		entity = INVALID_ENTITY;
	}

	if (scene.animations.GetCount() == 0)
	{
		SetEnabled(false);
		return;
	}
	else
	{
		SetEnabled(true);
	}

	for (size_t i = 0; i < scene.animations.GetCount(); ++i)
	{
		Entity e = scene.animations.GetEntity(i);
		NameComponent& name = *scene.names.GetComponent(e);
		animationsComboBox.AddItem(name.name.empty() ? std::to_string(e) : name.name);

		if (e == entity)
		{
			animationsComboBox.SetSelected((int)i);
		}
	}

	if (entity == INVALID_ENTITY && scene.animations.GetCount() > 0)
	{
		entity = scene.animations.GetEntity(0);
		animationsComboBox.SetSelected(0);
	}

	int selected = animationsComboBox.GetSelected();
	if (selected >= 0 && selected < (int)scene.animations.GetCount())
	{
		AnimationComponent& animation = scene.animations[selected];

		if (animation.IsPlaying())
		{
			playButton.SetText("Pause");
			animation.amount = wiMath::Lerp(animation.amount, 1, 0.1f);
		}
		else
		{
			playButton.SetText("Play");
		}

		loopedCheckBox.SetCheck(animation.IsLooped());

		timerSlider.SetRange(0, animation.GetLength());
		timerSlider.SetValue(animation.timer);
		amountSlider.SetValue(animation.amount);
		speedSlider.SetValue(animation.speed);
	}
}
