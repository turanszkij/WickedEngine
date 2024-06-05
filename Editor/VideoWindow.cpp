#include "stdafx.h"
#include "VideoWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;


void VideoPreview::Render(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const
{
	if (video == nullptr)
		return;

	GraphicsDevice* device = wi::graphics::GetDevice();
	device->EventBegin("Video Preview", cmd);

	wi::image::Params params;
	params.pos = translation;
	params.siz = XMFLOAT2(scale.x, scale.y);
	params.blendFlag = wi::enums::BLENDMODE_OPAQUE;
	if (!video->videoinstance.output.texture.IsValid())
	{
		params.color = wi::Color::Black();
	}
	wi::image::Draw(&video->videoinstance.output.texture, params, cmd);

	device->EventEnd(cmd);
}

void VideoWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_VIDEO " Video", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(440, 840));

	closeButton.SetTooltip("Delete VideoComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().videos.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
		});

	float x = 60;
	float y = 0;
	float hei = 18;
	float step = hei + 2;
	float wid = 200;


	openButton.Create("Open File " ICON_OPEN);
	openButton.SetTooltip("Open video file.\nSupported extensions: MP4");
	openButton.SetPos(XMFLOAT2(x, y));
	openButton.SetSize(XMFLOAT2(wid, hei));
	openButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "Video (.mp4)";
			params.extensions = wi::resourcemanager::GetSupportedVideoExtensions();
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
					video->filename = fileName;
					video->videoResource = wi::resourcemanager::Load(fileName);
					wi::video::CreateVideoInstance(&video->videoResource.GetVideo(), &video->videoinstance);
					filenameLabel.SetText(wi::helper::GetFileNameFromPath(video->filename));
					});
				});
		}
		});
	AddWidget(&openButton);

	filenameLabel.Create("Filename");
	filenameLabel.SetPos(XMFLOAT2(x, y += step));
	filenameLabel.SetSize(XMFLOAT2(wid, hei));
	AddWidget(&filenameLabel);

	playpauseButton.Create(ICON_PLAY);
	playpauseButton.SetTooltip("Play/Pause selected video instance.");
	playpauseButton.SetPos(XMFLOAT2(x, y += step));
	playpauseButton.SetSize(XMFLOAT2(wid, hei));
	playpauseButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			if (video->IsPlaying())
			{
				video->Stop();
				playpauseButton.SetText(ICON_PLAY);
			}
			else
			{
				video->Play();
				playpauseButton.SetText(ICON_PAUSE);
			}
		}
		});
	AddWidget(&playpauseButton);

	stopButton.Create(ICON_STOP);
	stopButton.SetTooltip("Stop selected video instance.");
	stopButton.SetPos(XMFLOAT2(x, y += step));
	stopButton.SetSize(XMFLOAT2(hei, hei));
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			video->Stop();
			video->videoinstance.current_frame = 0;
			video->videoinstance.flags &= ~wi::video::VideoInstance::Flags::InitialFirstFrameDecoded;
			video->videoinstance.flags &= ~wi::video::VideoInstance::Flags::Playing;
		}
		});
	AddWidget(&stopButton);

	loopedCheckbox.Create("Looped: ");
	loopedCheckbox.SetTooltip("Enable looping for the selected video instance.");
	loopedCheckbox.SetPos(XMFLOAT2(x, y += step));
	loopedCheckbox.SetSize(XMFLOAT2(hei, hei));
	loopedCheckbox.SetCheckText(ICON_LOOP);
	loopedCheckbox.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			video->SetLooped(args.bValue);
		}
		});
	AddWidget(&loopedCheckbox);

	timerSlider.Create(0, 1, 0, 100000, "Timer: ");
	timerSlider.SetSize(XMFLOAT2(hei, hei));
	timerSlider.OnSlide([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr && video->videoResource.IsValid())
		{
			const wi::video::Video& videofile = video->videoResource.GetVideo();
			int target_frame = int(float(args.fValue / videofile.duration_seconds) * videofile.frames_infos.size());
			for (size_t i = 0; i < videofile.frames_infos.size(); ++i)
			{
				auto& frame_info = videofile.frames_infos[i];
				if (i >= target_frame && frame_info.type == wi::graphics::VideoFrameType::Intra)
				{
					target_frame = (int)i;
					break;
				}
			}
			video->videoinstance.current_frame = target_frame;
			video->videoinstance.flags |= wi::video::VideoInstance::Flags::DecoderReset;
			video->videoinstance.flags &= ~wi::video::VideoInstance::Flags::InitialFirstFrameDecoded;
		}
		});
	AddWidget(&timerSlider);

	preview.SetSize(XMFLOAT2(160, 90));
	AddWidget(&preview);

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(800, 500));
	AddWidget(&infoLabel);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void VideoWindow::SetEntity(Entity entity)
{
	this->entity = entity;
	Scene& scene = editor->GetCurrentScene();
	VideoComponent* video = scene.videos.GetComponent(entity);
	NameComponent* name = scene.names.GetComponent(entity);

	preview.video = video;

	if (video != nullptr)
	{
		filenameLabel.SetText(wi::helper::GetFileNameFromPath(video->filename));
		playpauseButton.SetEnabled(true);
		stopButton.SetEnabled(true);
		loopedCheckbox.SetEnabled(true);
		loopedCheckbox.SetCheck(video->IsLooped());
		if (video->IsPlaying())
		{
			playpauseButton.SetText(ICON_PAUSE);
		}
		else
		{
			playpauseButton.SetText(ICON_PLAY);
		}

		if (video->videoResource.IsValid())
		{
			const wi::video::Video& videofile = video->videoResource.GetVideo();
			std::string str;
			if (GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::VIDEO_DECODE_H264))
			{
				str += "GPU decode: Supported\n";
			}
			else
			{
				str += "GPU decode: Not supported\n";
			}
			str += "Audio track: not implemented\n";
			str += "Display buffers in use: " + std::to_string(video->videoinstance.output_textures_used.size()) + "\n\n";

			str += "Tip: if you have a material on this entity, the material textures will be replaced by video. This way you can set video textures into the 3D scene.\n\n";

			str += "title : " + videofile.title + "\n";
			str += "album : " + videofile.album + "\n";
			str += "artist : " + videofile.artist + "\n";
			str += "year : " + videofile.year + "\n";
			str += "comment : " + videofile.comment + "\n";
			str += "genre : " + videofile.genre + "\n";
			str += "width : " + std::to_string(videofile.width) + "\n";
			str += "height : " + std::to_string(videofile.height) + "\n";
			str += "duration : " + std::to_string(videofile.duration_seconds) + " seconds\n";
			str += "FPS : " + std::to_string(videofile.average_frames_per_second) + "\n";
			str += "bit rate : " + std::to_string(videofile.bit_rate) + "\n";
			str += "Picture Parameter Sets: " + std::to_string(videofile.pps_count) + "\n";
			str += "Sequence Parameter Sets: " + std::to_string(videofile.sps_count) + "\n";
			str += "Decode Picture Buffers: " + std::to_string(video->videoinstance.dpb.texture.desc.array_size) + "\n";
			infoLabel.SetText(str);

			timerSlider.SetRange(0, videofile.duration_seconds);
			timerSlider.SetValue(float(video->videoinstance.current_frame) / float(videofile.frames_infos.size()) * videofile.duration_seconds);
		}
	}
	else
	{
		filenameLabel.SetText("");
		playpauseButton.SetEnabled(false);
		stopButton.SetEnabled(false);
		loopedCheckbox.SetEnabled(false);
		infoLabel.SetText("");
	}
}

void VideoWindow::ResizeLayout()
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
	add_fullwidth(filenameLabel);

	add_fullwidth(preview);
	float h_aspect = 9.0f / 16.0f;
	if (preview.video != nullptr && preview.video->videoResource.IsValid())
	{
		const wi::video::Video& video = preview.video->videoResource.GetVideo();
		h_aspect = (float)video.height / (float)video.width;
	}
	preview.SetSize(XMFLOAT2(preview.GetSize().x, preview.GetSize().x * h_aspect));

	add(playpauseButton);
	loopedCheckbox.SetPos(XMFLOAT2(playpauseButton.GetPos().x - loopedCheckbox.GetSize().x - 2, playpauseButton.GetPos().y));
	stopButton.SetPos(XMFLOAT2(playpauseButton.GetPos().x + playpauseButton.GetSize().x + 2, playpauseButton.GetPos().y));
	stopButton.SetSize(XMFLOAT2(width - stopButton.GetPos().x - padding, playpauseButton.GetSize().y));

	add(timerSlider);

	y += jump;

	add_fullwidth(infoLabel);
}

