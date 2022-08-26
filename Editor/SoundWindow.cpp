#include "stdafx.h"
#include "SoundWindow.h"
#include "wiAudio.h"
#include "Editor.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void SoundWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SOUND " Sound", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(440, 200));

	closeButton.SetTooltip("Delete SoundComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().sounds.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 200;


	openButton.Create("Open File " ICON_OPEN);
	openButton.SetPos(XMFLOAT2(x, y));
	openButton.SetSize(XMFLOAT2(wid, hei));
	openButton.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Sound";
			params.extensions = wi::resourcemanager::GetSupportedSoundExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					sound->filename = fileName;
					sound->soundResource = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
					});
				});
		}
		});
	AddWidget(&openButton);

	filenameLabel.Create("Filename");
	filenameLabel.SetPos(XMFLOAT2(x, y += step));
	filenameLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&filenameLabel);

	playstopButton.Create(ICON_PLAY);
	playstopButton.SetTooltip("Play/Stop selected sound instance.");
	playstopButton.SetPos(XMFLOAT2(x, y += step));
	playstopButton.SetSize(XMFLOAT2(wid, hei));
	playstopButton.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
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
	loopedCheckbox.SetPos(XMFLOAT2(x, y += step));
	loopedCheckbox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckbox.SetCheckText(ICON_LOOP);
	loopedCheckbox.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->SetLooped(args.bValue);
		}
	});
	AddWidget(&loopedCheckbox);
	loopedCheckbox.SetEnabled(false);

	reverbCheckbox.Create("Reverb: ");
	reverbCheckbox.SetTooltip("Enable/disable reverb.");
	reverbCheckbox.SetPos(XMFLOAT2(x, y += step));
	reverbCheckbox.SetSize(XMFLOAT2(hei, hei));
	reverbCheckbox.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->soundinstance.SetEnableReverb(args.bValue);
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&reverbCheckbox);
	reverbCheckbox.SetEnabled(false);

	disable3dCheckbox.Create("2D: ");
	disable3dCheckbox.SetTooltip("Sounds in the scene are 3D spatial by default. Select this to disable 3D effect.");
	disable3dCheckbox.SetPos(XMFLOAT2(x, y += step));
	disable3dCheckbox.SetSize(XMFLOAT2(hei, hei));
	disable3dCheckbox.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->SetDisable3D(args.bValue);
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&disable3dCheckbox);
	loopedCheckbox.SetEnabled(false);

	volumeSlider.Create(0, 1, 1, 1000, "Volume: ");
	volumeSlider.SetTooltip("Set volume level for the selected sound instance.");
	volumeSlider.SetPos(XMFLOAT2(x, y += step));
	volumeSlider.SetSize(XMFLOAT2(wid, hei));
	volumeSlider.OnSlide([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->volume = args.fValue;
		}
	});
	AddWidget(&volumeSlider);
	volumeSlider.SetEnabled(false);

	submixComboBox.Create("Submix: ");
	submixComboBox.SetPos(XMFLOAT2(x, y += step));
	submixComboBox.SetSize(XMFLOAT2(wid, hei));
	submixComboBox.OnSelect([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			sound->soundinstance.type = (wi::audio::SUBMIX_TYPE)args.iValue;
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	submixComboBox.AddItem("SOUNDEFFECT");
	submixComboBox.AddItem("MUSIC");
	submixComboBox.AddItem("USER0");
	submixComboBox.AddItem("USER1");
	submixComboBox.SetTooltip("Set the submix channel of the sound. \nSound properties like volume can be set per sound, or per submix channel.");
	submixComboBox.SetScriptTip("SoundInstance::SetSubmixType(int submixType)");
	AddWidget(&submixComboBox);

	reverbComboBox.Create("Reverb: ");
	reverbComboBox.SetPos(XMFLOAT2(x, y += step));
	reverbComboBox.SetSize(XMFLOAT2(wid, hei));
	reverbComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::audio::SetReverb((wi::audio::REVERB_PRESET)args.iValue);
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


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}



void SoundWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	SoundComponent* sound = scene.sounds.GetComponent(entity);
	NameComponent* name = scene.names.GetComponent(entity);

	if (sound != nullptr)
	{
		filenameLabel.SetText(wi::helper::GetFileNameFromPath(sound->filename));
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
			playstopButton.SetText(ICON_STOP);
		}
		else
		{
			playstopButton.SetText(ICON_PLAY);
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
		playstopButton.SetEnabled(false);
		loopedCheckbox.SetEnabled(false);
		reverbCheckbox.SetEnabled(false);
		disable3dCheckbox.SetEnabled(false);
		volumeSlider.SetEnabled(false);
		submixComboBox.SetEnabled(false);
	}
}


void SoundWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 80;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(width - margin_right - widget.GetSize().x, y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_fullwidth = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = padding;
		const float margin_right = padding;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};

	add_fullwidth(openButton);
	add(reverbComboBox);
	add_fullwidth(filenameLabel);
	add(playstopButton);
	loopedCheckbox.SetPos(XMFLOAT2(playstopButton.GetPos().x - loopedCheckbox.GetSize().x - 2, playstopButton.GetPos().y));
	add_right(reverbCheckbox);
	disable3dCheckbox.SetPos(XMFLOAT2(reverbCheckbox.GetPos().x - disable3dCheckbox.GetSize().x - 100 - 2, reverbCheckbox.GetPos().y));
	add(volumeSlider);
	add(submixComboBox);
}

