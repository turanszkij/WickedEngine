#include "stdafx.h"
#include "SoundWindow.h"
#include "wiAudio.h"
#include "Editor.h"

#include <sstream>

using namespace std;
using namespace wiGraphics;
using namespace wiECS;
using namespace wiScene;

void SoundWindow::Create(EditorComponent* editor)
{
	wiWindow::Create("Sound Window");
	SetSize(XMFLOAT2(440, 220));

	float x = 20;
	float y = 10;
	float hei = 18;
	float step = hei + 2;

	reverbComboBox.Create("Reverb: ");
	reverbComboBox.SetPos(XMFLOAT2(x + 80, y += step));
	reverbComboBox.SetSize(XMFLOAT2(180, hei));
	reverbComboBox.OnSelect([&](wiEventArgs args) {
		wiAudio::SetReverb((wiAudio::REVERB_PRESET)args.iValue);
	});
	reverbComboBox.AddItem("DEFAULT");
	reverbComboBox.AddItem("GENERIC");
	reverbComboBox.AddItem("FOREST");
	reverbComboBox.AddItem("PADDEDCELL");
	reverbComboBox.AddItem("ROOM");
	reverbComboBox.AddItem("BATHROOM");
	reverbComboBox.AddItem("LIVINGROOM");
	reverbComboBox.AddItem("STONEROOM");
	reverbComboBox.AddItem("AUDITORIUM");
	reverbComboBox.AddItem("CONCERTHALL");
	reverbComboBox.AddItem("CAVE");
	reverbComboBox.AddItem("ARENA");
	reverbComboBox.AddItem("HANGAR");
	reverbComboBox.AddItem("CARPETEDHALLWAY");
	reverbComboBox.AddItem("HALLWAY");
	reverbComboBox.AddItem("STONECORRIDOR");
	reverbComboBox.AddItem("ALLEY");
	reverbComboBox.AddItem("CITY");
	reverbComboBox.AddItem("MOUNTAINS");
	reverbComboBox.AddItem("QUARRY");
	reverbComboBox.AddItem("PLAIN");
	reverbComboBox.AddItem("PARKINGLOT");
	reverbComboBox.AddItem("SEWERPIPE");
	reverbComboBox.AddItem("UNDERWATER");
	reverbComboBox.AddItem("SMALLROOM");
	reverbComboBox.AddItem("MEDIUMROOM");
	reverbComboBox.AddItem("LARGEROOM");
	reverbComboBox.AddItem("MEDIUMHALL");
	reverbComboBox.AddItem("LARGEHALL");
	reverbComboBox.AddItem("PLATE");
	reverbComboBox.SetTooltip("Set the global reverb setting. Sound instances need to enable reverb to take effect!");
	AddWidget(&reverbComboBox);

	y += step;

	addButton.Create("Add Sound");
	addButton.SetTooltip("Add a sound file to the scene.");
	addButton.SetPos(XMFLOAT2(x, y += step));
	addButton.SetSize(XMFLOAT2(80, hei));
	addButton.OnClick([=](wiEventArgs args) {
		wiHelper::FileDialogParams params;
		params.type = wiHelper::FileDialogParams::OPEN;
		params.description = "Sound";
		params.extensions.push_back("wav");
		params.extensions.push_back("ogg");
		wiHelper::FileDialog(params, [=](std::string fileName) {
			wiEvent::Subscribe_Once(SYSTEM_EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
				Entity entity = GetScene().Entity_CreateSound("editorSound", fileName);
				editor->ClearSelected();
				editor->AddSelected(entity);
				editor->RefreshSceneGraphView();
				SetEntity(entity);
			});
		});
	});
	AddWidget(&addButton);

	filenameLabel.Create("Filename");
	filenameLabel.SetPos(XMFLOAT2(x, y += step));
	filenameLabel.SetSize(XMFLOAT2(400, hei));
	AddWidget(&filenameLabel);

	nameField.Create("SoundName");
	nameField.SetTooltip("Enter a sound name to identify this entity...");
	nameField.SetPos(XMFLOAT2(x, y += step));
	nameField.SetSize(XMFLOAT2(300, hei));
	nameField.OnInputAccepted([=](wiEventArgs args) {
		NameComponent* name = wiScene::GetScene().names.GetComponent(entity);
		if (name == nullptr)
		{
			name = &wiScene::GetScene().names.Create(entity);
		}
		*name = args.sValue;

		editor->RefreshSceneGraphView();
	});
	AddWidget(&nameField);
	nameField.SetEnabled(false);

	playstopButton.Create("Play");
	playstopButton.SetTooltip("Play/Stop selected sound instance.");
	playstopButton.SetPos(XMFLOAT2(x, y += step));
	playstopButton.SetSize(XMFLOAT2(80, hei));
	playstopButton.OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			if (sound->IsPlaying())
			{
				sound->Stop();
				playstopButton.SetText("Play");
			}
			else
			{
				sound->Play();
				playstopButton.SetText("Stop");
			}
		}
	});
	AddWidget(&playstopButton);
	playstopButton.SetEnabled(false);

	loopedCheckbox.Create("Looped: ");
	loopedCheckbox.SetTooltip("Enable looping for the selected sound instance.");
	loopedCheckbox.SetPos(XMFLOAT2(x + 150, y));
	loopedCheckbox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckbox.OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->SetLooped(args.bValue);
		}
	});
	AddWidget(&loopedCheckbox);
	loopedCheckbox.SetEnabled(false);

	reverbCheckbox.Create("Reverb: ");
	reverbCheckbox.SetTooltip("Enable/disable reverb.");
	reverbCheckbox.SetPos(XMFLOAT2(x + 240, y));
	reverbCheckbox.SetSize(XMFLOAT2(hei, hei));
	reverbCheckbox.OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->soundinstance.SetEnableReverb(args.bValue);
			wiAudio::CreateSoundInstance(sound->soundResource->sound, &sound->soundinstance);
		}
	});
	AddWidget(&reverbCheckbox);
	reverbCheckbox.SetEnabled(false);

	disable3dCheckbox.Create("2D: ");
	disable3dCheckbox.SetTooltip("Sounds in the scene are 3D spatial by default. Select this to disable 3D effect.");
	disable3dCheckbox.SetPos(XMFLOAT2(x + 300, y));
	disable3dCheckbox.SetSize(XMFLOAT2(hei, hei));
	disable3dCheckbox.OnClick([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->SetDisable3D(args.bValue);
			wiAudio::CreateSoundInstance(sound->soundResource->sound, &sound->soundinstance);
		}
	});
	AddWidget(&disable3dCheckbox);
	loopedCheckbox.SetEnabled(false);

	volumeSlider.Create(0, 1, 1, 1000, "Volume: ");
	volumeSlider.SetTooltip("Set volume level for the selected sound instance.");
	volumeSlider.SetPos(XMFLOAT2(x + 60, y += step));
	volumeSlider.SetSize(XMFLOAT2(240, hei));
	volumeSlider.OnSlide([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->volume = args.fValue;
		}
	});
	AddWidget(&volumeSlider);
	volumeSlider.SetEnabled(false);

	submixComboBox.Create("Submix: ");
	submixComboBox.SetPos(XMFLOAT2(x + 80, y += step));
	submixComboBox.SetSize(XMFLOAT2(180, hei));
	submixComboBox.OnSelect([&](wiEventArgs args) {
		SoundComponent* sound = GetScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->soundinstance.type = (wiAudio::SUBMIX_TYPE)args.iValue;
			wiAudio::CreateSoundInstance(sound->soundResource->sound, &sound->soundinstance);
		}
	});
	submixComboBox.AddItem("SOUNDEFFECT");
	submixComboBox.AddItem("MUSIC");
	submixComboBox.AddItem("USER0");
	submixComboBox.AddItem("USER1");
	submixComboBox.SetTooltip("Set the submix channel of the sound. \nSound properties like volume can be set per sound, or per submix channel.");
	submixComboBox.SetScriptTip("SoundInstance::SetSubmixType(int submixType)");
	AddWidget(&submixComboBox);

	Translate(XMFLOAT3(400, 120, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}



void SoundWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = wiScene::GetScene();
	SoundComponent* sound = scene.sounds.GetComponent(entity);
	NameComponent* name = scene.names.GetComponent(entity);

	if (sound != nullptr)
	{
		filenameLabel.SetText(sound->filename);
		if (name == nullptr)
		{
			nameField.SetText("Enter a sound name...");
		}
		else
		{
			nameField.SetText(name->name);
		}
		nameField.SetEnabled(true);
		playstopButton.SetEnabled(true);
		loopedCheckbox.SetEnabled(true);
		loopedCheckbox.SetCheck(sound->IsLooped());
		reverbCheckbox.SetEnabled(true);
		reverbCheckbox.SetCheck(sound->soundinstance.IsEnableReverb());
		disable3dCheckbox.SetEnabled(true);
		disable3dCheckbox.SetCheck(sound->IsDisable3D());
		volumeSlider.SetEnabled(true);
		volumeSlider.SetValue(sound->volume);
		if (sound->IsPlaying())
		{
			playstopButton.SetText("Stop");
		}
		else
		{
			playstopButton.SetText("Play");
		}
		submixComboBox.SetEnabled(true);
		if (submixComboBox.GetSelected() != (int)sound->soundinstance.type)
		{
			submixComboBox.SetSelected((int)sound->soundinstance.type);
		}
	}
	else
	{
		filenameLabel.SetText("");
		nameField.SetText("");
		nameField.SetEnabled(false);
		playstopButton.SetEnabled(false);
		loopedCheckbox.SetEnabled(false);
		reverbCheckbox.SetEnabled(false);
		disable3dCheckbox.SetEnabled(false);
		volumeSlider.SetEnabled(false);
		submixComboBox.SetEnabled(false);
	}
}

