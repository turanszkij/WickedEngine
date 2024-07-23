#include "stdafx.h"
#include "SoundWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

namespace SoundWindow_Internal
{
	PipelineState pso_linestrip;
	PipelineState pso_linelist;

	void LoadShaders()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		PipelineStateDesc desc;
		desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
		desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
		desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
		desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEPTHDISABLED);
		desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_WIRE_SMOOTH);
		desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
		desc.pt = PrimitiveTopology::LINESTRIP;
		bool success = device->CreatePipelineState(&desc, &pso_linestrip);

		desc.pt = PrimitiveTopology::LINELIST;
		success = device->CreatePipelineState(&desc, &pso_linelist);
		assert(success);
	}
}
using namespace SoundWindow_Internal;

void SoundWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_SOUND " Sound", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(440, 400));

	closeButton.SetTooltip("Delete SoundComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().sounds.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 60;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 200;


	openButton.Create("Open File " ICON_OPEN);
	openButton.SetTooltip("Open sound file.\nSupported extensions: WAV, OGG");
	openButton.SetPos(XMFLOAT2(x, y));
	openButton.SetSize(XMFLOAT2(wid, hei));
	openButton.OnClick([&](wi::gui::EventArgs args) {
		SoundComponent* sound = editor->GetCurrentScene().sounds.GetComponent(entity);
		if (sound != nullptr)
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Sound (.wav, .ogg)";
			params.extensions = wi::resourcemanager::GetSupportedSoundExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					sound->filename = fileName;
					sound->soundResource = wi::resourcemanager::Load(fileName);
					wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
					filenameLabel.SetText(wi::helper::GetFileNameFromPath(sound->filename));
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
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			if (sound->IsPlaying())
			{
				sound->Stop();
				playstopButton.SetText(ICON_PLAY);
			}
			else
			{
				sound->Play();
				playstopButton.SetText(ICON_STOP);
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
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
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
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
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
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->SetDisable3D(args.bValue);
		}
	});
	AddWidget(&disable3dCheckbox);
	loopedCheckbox.SetEnabled(false);

	volumeSlider.Create(0, 1, 1, 1000, "Volume: ");
	volumeSlider.SetTooltip("Set volume level for the selected sound instance.");
	volumeSlider.SetPos(XMFLOAT2(x, y += step));
	volumeSlider.SetSize(XMFLOAT2(wid, hei));
	volumeSlider.OnSlide([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->volume = args.fValue;
		}
	});
	AddWidget(&volumeSlider);
	volumeSlider.SetEnabled(false);

	submixComboBox.Create("Submix: ");
	submixComboBox.SetPos(XMFLOAT2(x, y += step));
	submixComboBox.SetSize(XMFLOAT2(wid, hei));
	submixComboBox.OnSelect([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
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
	reverbComboBox.SetMaxVisibleItemCount(6);
	AddWidget(&reverbComboBox);

	waveGraph.SetSize(XMFLOAT2(100, 50));
	AddWidget(&waveGraph);

	beginInput.Create("");
	beginInput.SetDescription("Begin: ");
	beginInput.SetTooltip("Beginning of the playback in seconds, relative to the Sound it will be created from (0 = from beginning).");
	beginInput.SetSize(XMFLOAT2(wid, hei));
	beginInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->soundinstance.begin = args.fValue;
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&beginInput);

	lengthInput.Create("");
	lengthInput.SetDescription("Length: ");
	lengthInput.SetTooltip("Length in seconds (0 = until end)");
	lengthInput.SetSize(XMFLOAT2(wid, hei));
	lengthInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->soundinstance.length = args.fValue;
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&lengthInput);

	loopBeginInput.Create("");
	loopBeginInput.SetDescription("Loop Begin: ");
	loopBeginInput.SetTooltip("Loop region begin in seconds, relative to the instance begin time (0 = from beginning)");
	loopBeginInput.SetSize(XMFLOAT2(wid, hei));
	loopBeginInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->soundinstance.loop_begin = args.fValue;
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&loopBeginInput);

	loopLengthInput.Create("");
	loopLengthInput.SetDescription("Loop Length: ");
	loopLengthInput.SetTooltip("Loop region length in seconds (0 = until the end)");
	loopLengthInput.SetSize(XMFLOAT2(wid, hei));
	loopLengthInput.OnInputAccepted([&](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			SoundComponent* sound = scene.sounds.GetComponent(x.entity);
			if (sound == nullptr)
				continue;
			sound->soundinstance.loop_length = args.fValue;
			wi::audio::CreateSoundInstance(&sound->soundResource.GetSound(), &sound->soundinstance);
		}
	});
	AddWidget(&loopLengthInput);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void WaveGraph::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	GraphicsDevice* device = wi::graphics::GetDevice();
	device->EventBegin("Sound Wave", cmd);

	static bool shaders_loaded = false;
	if (!shaders_loaded)
	{
		shaders_loaded = true;
		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { SoundWindow_Internal::LoadShaders(); });
		SoundWindow_Internal::LoadShaders();
	}

	struct Vertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};
	const uint32_t vertexCount = 256;
	GraphicsDevice::GPUAllocation allocation = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);

	XMFLOAT4 base_color = font.params.color;
	base_color.w = 1;

	if (sound == nullptr || !sound->soundResource.IsValid())
	{
		// Vertices for straight line:
		Vertex vert;
		vert.color = base_color;
		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			vert.position = XMFLOAT4(float(i) / vertexCount, 0, 0, 1);
			std::memcpy((Vertex*)allocation.data + i, &vert, sizeof(vert));
		}
	}
	else
	{
		// Vertices for [-1, 1] second range of current sound sample:
		wi::audio::SampleInfo info = wi::audio::GetSampleInfo(&sound->soundResource.GetSound());
		const uint32_t step = uint32_t(info.sample_rate * 2) / vertexCount;
		const uint32_t instance_sample_offset = uint32_t(sound->soundinstance.begin * info.sample_rate);
		const uint32_t instance_sample_count = uint32_t(sound->soundinstance.length > 0 ? uint32_t(sound->soundinstance.length * info.sample_rate) : uint32_t(info.sample_count));
		uint64_t current_instance_sample = wi::audio::GetTotalSamplesPlayed(&sound->soundinstance);
		if (sound->IsLooped())
		{
			float total_time = float(current_instance_sample) / float(info.sample_rate);
			if (total_time > sound->soundinstance.loop_begin)
			{
				float loop_length = sound->soundinstance.loop_length > 0 ? sound->soundinstance.loop_length : (float(info.sample_count) / float(info.sample_rate));
				float loop_time = std::fmod(total_time - sound->soundinstance.loop_begin, loop_length);
				current_instance_sample = uint64_t(loop_time * info.sample_rate);
			}
		}
		current_instance_sample = current_instance_sample % instance_sample_count;

		uint64_t current_sound_sample = instance_sample_offset + current_instance_sample;
		// This integer divide and multiply fixes the sample positions so the wave visualizer is stable:
		current_sound_sample /= step;
		current_sound_sample *= step;
		int64_t start_sample = (int64_t)current_sound_sample - info.sample_rate;
		int64_t end_sample = current_sound_sample + info.sample_rate;

		Vertex vert;
		vert.color = base_color;
		for (uint32_t i = 0; i < vertexCount; ++i)
		{
			vert.position = XMFLOAT4(float(i) / vertexCount, 0, 0, 1);
			const int64_t sample_idx = start_sample + i * step;
			if (sample_idx > 0 && sample_idx < (int64_t)info.sample_count)
			{
				vert.position.y = float(info.samples[sample_idx * info.channel_count]) / 32768.0f;
			}
			std::memcpy((Vertex*)allocation.data + i, &vert, sizeof(vert));
		}
	}

	const uint32_t strides[] = {
		sizeof(Vertex),
	};

	MiscCB cb;
	cb.g_xColor = XMFLOAT4(1, 1, 1, 1);
	XMStoreFloat4x4(&cb.g_xTransform,
		XMMatrixScaling(GetSize().x, GetSize().y, 1) *
		XMMatrixTranslation(GetPos().x, GetPos().y + GetSize().y, 0)*
		canvas.GetProjection()
	);
	device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(MiscCB), cmd);

	// Wave:
	{
		device->BindPipelineState(&pso_linestrip, cmd);
		const GPUBuffer* vbs[] = {
			&allocation.buffer,
		};
		const uint64_t offsets[] = {
			allocation.offset,
		};
		device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
		device->BindDynamicConstantBuffer(cb, CB_GETBINDSLOT(MiscCB), cmd);
		device->Draw(vertexCount, 0, cmd);
	}
	// Other stuff:
	{
		Vertex verts[] = {
			// center:
			{ XMFLOAT4(0.5f, -1, 0, 1), XMFLOAT4(1, 0.2f, 0.2f, 1) },
			{ XMFLOAT4(0.5f, 1, 0, 1), XMFLOAT4(1, 0.2f, 0.2f, 1) },

			// rect:
			{ XMFLOAT4(0, -1, 0, 1), base_color },
			{ XMFLOAT4(1, -1, 0, 1), base_color },
			{ XMFLOAT4(1, -1, 0, 1), base_color },
			{ XMFLOAT4(1, 1, 0, 1), base_color },
			{ XMFLOAT4(1, 1, 0, 1), base_color },
			{ XMFLOAT4(0, 1, 0, 1), base_color },
			{ XMFLOAT4(0, 1, 0, 1), base_color },
			{ XMFLOAT4(0, -1, 0, 1), base_color },
		};
		allocation = device->AllocateGPU(sizeof(verts), cmd);
		std::memcpy(allocation.data, verts, sizeof(verts));

		device->BindPipelineState(&pso_linelist, cmd);
		const GPUBuffer* vbs[] = {
			&allocation.buffer,
		};
		const uint64_t offsets[] = {
			allocation.offset,
		};
		device->BindVertexBuffers(vbs, 0, 1, strides, offsets, cmd);
		device->Draw(sizeof(verts) / sizeof(Vertex), 0, cmd);
	}

	device->EventEnd(cmd);
}


void SoundWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	SoundComponent* sound = scene.sounds.GetComponent(entity);
	NameComponent* name = scene.names.GetComponent(entity);

	waveGraph.sound = sound;

	if (sound != nullptr && sound->soundResource.IsValid())
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
		beginInput.SetValue(sound->soundinstance.begin);
		lengthInput.SetValue(sound->soundinstance.length);
		loopBeginInput.SetValue(sound->soundinstance.loop_begin);
		loopLengthInput.SetValue(sound->soundinstance.loop_length);
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
		const float margin_left = 100;
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
	add_fullwidth(filenameLabel);
	add(playstopButton);
	loopedCheckbox.SetPos(XMFLOAT2(playstopButton.GetPos().x - loopedCheckbox.GetSize().x - 2, playstopButton.GetPos().y));
	add(volumeSlider);
	add_right(reverbCheckbox);
	disable3dCheckbox.SetPos(XMFLOAT2(reverbCheckbox.GetPos().x - disable3dCheckbox.GetSize().x - 100 - 2, reverbCheckbox.GetPos().y));
	add(submixComboBox);
	add(reverbComboBox);

	add(beginInput);
	add(lengthInput);
	add(loopBeginInput);
	add(loopLengthInput);

	y += 10;
	add_fullwidth(waveGraph);
}

