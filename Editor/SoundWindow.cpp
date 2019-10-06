#include "stdafx.h"
#include "SoundWindow.h"

#include <sstream>

#include <Commdlg.h> // openfile
#include <WinBase.h>

using namespace std;
using namespace wiGraphics;
using namespace wiECS;
using namespace wiSceneSystem;

SoundWindow::SoundWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	soundWindow = new wiWindow(GUI, "Sound Window");
	soundWindow->SetSize(XMFLOAT2(440, 200));
	soundWindow->SetEnabled(false);
	GUI->AddWidget(soundWindow);

	float x = 20;
	float y = 0;
	float step = 35;

	addButton = new wiButton("Add Sound");
	addButton->SetTooltip("Add a sound file to the scene.");
	addButton->SetPos(XMFLOAT2(x, y += step));
	addButton->SetSize(XMFLOAT2(80, 30));
	addButton->OnClick([&](wiEventArgs args) {
		char szFile[260];

		OPENFILENAMEA ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = szFile;
		// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
		// use the contents of szFile to initialize itself.
		ofn.lpstrFile[0] = '\0';
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = "Sound\0*.wav;\0";
		ofn.nFilterIndex = 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = NULL;
		ofn.Flags = 0;
		if (GetOpenFileNameA(&ofn) == TRUE) {
			string fileName = ofn.lpstrFile;
			Entity entity = GetScene().Entity_CreateSound(fileName);
		}
	});
	soundWindow->AddWidget(addButton);

	filenameLabel = new wiLabel("Filename");
	filenameLabel->SetPos(XMFLOAT2(x, y += step));
	filenameLabel->SetSize(XMFLOAT2(400, 20));
	soundWindow->AddWidget(filenameLabel);

	playstopButton = new wiButton("Play");
	playstopButton->SetTooltip("Play/Stop selected sound instance.");
	playstopButton->SetPos(XMFLOAT2(x, y += step));
	playstopButton->SetSize(XMFLOAT2(80, 30));
	playstopButton->OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			if (sound->IsPlaying())
			{
				sound->Stop();
				playstopButton->SetText("Play");
			}
			else
			{
				sound->Play();
				playstopButton->SetText("Stop");
			}
		}
	});
	soundWindow->AddWidget(playstopButton);
	playstopButton->SetEnabled(false);

	loopedCheckbox = new wiCheckBox("Looped: ");
	loopedCheckbox->SetTooltip("Enable looping for the selected sound instance.");
	loopedCheckbox->SetPos(XMFLOAT2(x + 150, y));
	loopedCheckbox->SetSize(XMFLOAT2(30, 30));
	loopedCheckbox->OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->SetLooped(args.bValue);
		}
	});
	soundWindow->AddWidget(loopedCheckbox);
	loopedCheckbox->SetEnabled(false);

	volumeSlider = new wiSlider(0, 1, 1, 1000, "Volume: ");
	volumeSlider->SetTooltip("Enable looping for the selected sound instance.");
	volumeSlider->SetPos(XMFLOAT2(x + 60, y += step));
	volumeSlider->SetSize(XMFLOAT2(240, 30));
	volumeSlider->OnSlide([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->volume = args.fValue;
		}
	});
	soundWindow->AddWidget(volumeSlider);
	volumeSlider->SetEnabled(false);

	soundWindow->Translate(XMFLOAT3(400, 120, 0));
	soundWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

SoundWindow::~SoundWindow()
{
	soundWindow->RemoveWidgets(true);
	GUI->RemoveWidget(soundWindow);
	SAFE_DELETE(soundWindow);
}



void SoundWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wiSceneSystem::GetScene();
	SoundComponent* sound = scene.sounds.GetComponent(entity);

	if (sound != nullptr)
	{
		filenameLabel->SetText(sound->filename);
		playstopButton->SetEnabled(true);
		loopedCheckbox->SetEnabled(true);
		loopedCheckbox->SetCheck(sound->IsLooped());
		volumeSlider->SetEnabled(true);
		volumeSlider->SetValue(sound->volume);
		if (sound->IsPlaying())
		{
			playstopButton->SetText("Stop");
		}
		else
		{
			playstopButton->SetText("Play");
		}
	}
	else
	{
		filenameLabel->SetText("");
		playstopButton->SetEnabled(false);
		loopedCheckbox->SetEnabled(false);
		volumeSlider->SetEnabled(false);
	}
}

