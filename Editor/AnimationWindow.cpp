#include "stdafx.h"
#include "AnimationWindow.h"

#include <sstream>

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


	animationsComboBox = new wiComboBox("Animation: ");
	animationsComboBox->SetSize(XMFLOAT2(300, 20));
	animationsComboBox->SetPos(XMFLOAT2(x, y += step));
	animationsComboBox->SetEnabled(false);
	animationsComboBox->OnSelect([&](wiEventArgs args) {
		entity = wiSceneSystem::GetScene().animations.GetEntity(args.iValue);
	});
	animationsComboBox->SetTooltip("Choose an animation clip...");
	animWindow->AddWidget(animationsComboBox);

	loopedCheckBox = new wiCheckBox("Looped: ");
	loopedCheckBox->SetTooltip("Toggle animation looping behaviour.");
	loopedCheckBox->SetPos(XMFLOAT2(150, y += step));
	loopedCheckBox->OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiSceneSystem::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->SetLooped(args.bValue);
		}
	});
	animWindow->AddWidget(loopedCheckBox);

	playButton = new wiButton("Play");
	playButton->SetTooltip("Play/Pause animation.");
	playButton->SetPos(XMFLOAT2(200, y));
	playButton->OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiSceneSystem::GetScene().animations.GetComponent(entity);
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
	animWindow->AddWidget(playButton);

	stopButton = new wiButton("Stop");
	stopButton->SetTooltip("Stop animation.");
	stopButton->SetPos(XMFLOAT2(310, y));
	stopButton->OnClick([&](wiEventArgs args) {
		AnimationComponent* animation = wiSceneSystem::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->Stop();
		}
	});
	animWindow->AddWidget(stopButton);

	timerSlider = new wiSlider(0, 1, 0, 100000, "Timer: ");
	timerSlider->SetSize(XMFLOAT2(250, 30));
	timerSlider->SetPos(XMFLOAT2(x, y += step * 2));
	timerSlider->OnSlide([&](wiEventArgs args) {
		AnimationComponent* animation = wiSceneSystem::GetScene().animations.GetComponent(entity);
		if (animation != nullptr)
		{
			animation->timer = args.fValue;
		}
	});
	timerSlider->SetEnabled(false);
	timerSlider->SetTooltip("Set the animation timer by hand.");
	animWindow->AddWidget(timerSlider);



	animWindow->Translate(XMFLOAT3(100, 50, 0));
	animWindow->SetVisible(false);

}


AnimationWindow::~AnimationWindow()
{
	animWindow->RemoveWidgets(true);

	GUI->RemoveWidget(animWindow);
	SAFE_DELETE(animWindow);
}

void AnimationWindow::Update()
{
	animationsComboBox->ClearItems();
	
	Scene& scene = wiSceneSystem::GetScene();

	if (!scene.animations.Contains(entity))
	{
		entity = INVALID_ENTITY;
	}

	if (scene.animations.GetCount() == 0)
	{
		animWindow->SetEnabled(false);
		return;
	}
	else
	{
		animWindow->SetEnabled(true);
	}

	for (size_t i = 0; i < scene.animations.GetCount(); ++i)
	{
		Entity e = scene.animations.GetEntity(i);
		NameComponent& name = *scene.names.GetComponent(e);

		std::stringstream ss("");
		ss << name.name << " (" << e << ")";
		animationsComboBox->AddItem(ss.str());

		if (e == entity)
		{
			animationsComboBox->SetSelected((int)i);
		}
	}

	if (entity == INVALID_ENTITY && scene.animations.GetCount() > 0)
	{
		entity = scene.animations.GetEntity(0);
		animationsComboBox->SetSelected(0);
	}

	int selected = animationsComboBox->GetSelected();
	if (selected >= 0 && selected < (int)scene.animations.GetCount())
	{
		AnimationComponent& animation = scene.animations[selected];

		if (animation.IsPlaying())
		{
			playButton->SetText("Pause");
		}
		else
		{
			playButton->SetText("Play");
		}

		loopedCheckBox->SetCheck(animation.IsLooped());

		timerSlider->SetRange(0, animation.GetLength());
		timerSlider->SetValue(animation.timer);
	}
}
