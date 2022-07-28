#include "stdafx.h"
#include "AnimationWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;

void AnimationWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_ANIMATION " Animation", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(520, 140));

	float x = 80;
	float y = 0;
	float hei = 18;
	float wid = 200;
	float step = hei + 2;


	animationsComboBox.Create("Animation: ");
	animationsComboBox.SetSize(XMFLOAT2(wid, hei));
	animationsComboBox.SetPos(XMFLOAT2(x, y));
	animationsComboBox.SetEnabled(false);
	animationsComboBox.OnSelect([&](wi::gui::EventArgs args) {
		entity = editor->GetCurrentScene().animations.GetEntity(args.iValue);
	});
	animationsComboBox.SetTooltip("Choose an animation clip...");
	AddWidget(&animationsComboBox);

	loopedCheckBox.Create("Looped: ");
	loopedCheckBox.SetTooltip("Toggle animation looping behaviour.");
	loopedCheckBox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckBox.SetPos(XMFLOAT2(x, y += step));
	loopedCheckBox.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->SetLooped(args.bValue);
		}
	});
	AddWidget(&loopedCheckBox);

	playButton.Create("Play");
	playButton.SetTooltip("Play/Pause animation.");
	playButton.SetSize(XMFLOAT2(100, hei));
	playButton.SetPos(XMFLOAT2(loopedCheckBox.GetPos().x + loopedCheckBox.GetSize().x + 5, y));
	playButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			if (animation->IsPlaying())
			{
				animation->Pause();
			}
			else
			{
				animation->Play();
			}
		}
	});
	AddWidget(&playButton);

	stopButton.Create("Stop");
	stopButton.SetTooltip("Stop animation.");
	stopButton.SetSize(XMFLOAT2(100, hei));
	stopButton.SetPos(XMFLOAT2(playButton.GetPos().x + playButton.GetSize().x + 5, y));
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->Stop();
		}
	});
	AddWidget(&stopButton);

	timerSlider.Create(0, 1, 0, 100000, "Timer: ");
	timerSlider.SetSize(XMFLOAT2(wid, hei));
	timerSlider.SetPos(XMFLOAT2(x, y += step));
	timerSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = args.fValue;
		}
	});
	timerSlider.SetEnabled(false);
	timerSlider.SetTooltip("Set the animation timer by hand.");
	AddWidget(&timerSlider);

	amountSlider.Create(0, 1, 1, 100000, "Amount: ");
	amountSlider.SetSize(XMFLOAT2(wid, hei));
	amountSlider.SetPos(XMFLOAT2(x, y += step));
	amountSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->amount = args.fValue;
		}
	});
	amountSlider.SetEnabled(false);
	amountSlider.SetTooltip("Set the animation blending amount by hand.");
	AddWidget(&amountSlider);

	speedSlider.Create(0, 4, 1, 100000, "Speed: ");
	speedSlider.SetSize(XMFLOAT2(wid, hei));
	speedSlider.SetPos(XMFLOAT2(x, y += step));
	speedSlider.OnSlide([&](wi::gui::EventArgs args) {
		AnimationComponent* animation = editor->GetCurrentScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->speed = args.fValue;
		}
	});
	speedSlider.SetEnabled(false);
	speedSlider.SetTooltip("Set the animation speed.");
	AddWidget(&speedSlider);



	SetMinimized(true);
	SetVisible(false);

}

void AnimationWindow::Update()
{
	animationsComboBox.ClearItems();
	
	Scene& scene = editor->GetCurrentScene();

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
