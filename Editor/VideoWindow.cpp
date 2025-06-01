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
	wi::gui::Window::Create(ICON_VIDEO " Video", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
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


	openButton.Create("Open File " ICON_OPEN);
	openButton.SetTooltip("Open video file.\nSupported extensions: MP4");
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
					if (video->videoResource.IsValid())
					{
						wi::video::CreateVideoInstance(&video->videoResource.GetVideo(), &video->videoinstance);
						filenameLabel.SetText(wi::helper::GetFileNameFromPath(video->filename));
					}
					});
				});
		}
		});
	AddWidget(&openButton);

	filenameLabel.Create("Filename");
	AddWidget(&filenameLabel);

	playpauseButton.Create(ICON_PLAY);
	playpauseButton.SetTooltip("Play/Pause selected video instance.");
	playpauseButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			if (video->IsPlaying())
			{
				video->Pause();
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
	stopButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			video->Stop();
		}
		});
	AddWidget(&stopButton);

	loopedCheckbox.Create("Looped: ");
	loopedCheckbox.SetTooltip("Enable looping for the selected video instance.");
	loopedCheckbox.SetCheckText(ICON_LOOP);
	loopedCheckbox.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			video->SetLooped(args.bValue);
		}
		});
	AddWidget(&loopedCheckbox);

	timerSlider.Create(0, 1, 0, 1000, "Timer: ");
	timerSlider.OnSlide([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		video->Seek(args.fValue);
	});
	AddWidget(&timerSlider);

	AddWidget(&preview);

	infoLabel.Create("");
	infoLabel.SetFitTextEnabled(true);
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
			if (videofile.profile == wi::graphics::VideoProfile::H264)
			{
				str += "Format: H264 (AVC)\n";
				if (GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::VIDEO_DECODE_H264))
				{
					str += "GPU decode: Supported\n";
				}
				else
				{
					str += "GPU decode: Not supported\n";
				}
			}
			else if (videofile.profile == wi::graphics::VideoProfile::H265)
			{
				str += "Format: H265 (HEVC)\n";
				if (GetDevice()->CheckCapability(wi::graphics::GraphicsDeviceCapability::VIDEO_DECODE_H265))
				{
					str += "GPU decode: Supported\n";
				}
				else
				{
					str += "GPU decode: Not supported\n";
				}
			}
			else
			{
				str += "Format: Unsupported\n";
			}
			str += "Audio track: not implemented\n";
			str += "Size: " + wi::helper::GetMemorySizeText(video->videoResource.GetVideo().data_stream.GetDesc().size) + "\n\n";

			str += "Tip: if you have a material on this entity, the material basecolor and emissive textures will be replaced by video. If it has light, then the video will be used as light multiplier. This way you can set video textures into the 3D scene.\n\n";

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
			str += "Target display order: " + std::to_string(video->videoinstance.target_display_order) + "\n";
			str += "Display buffers free: " + std::to_string(video->videoinstance.output_textures_free.size()) + "\n";
			str += "Display buffers in use: " + std::to_string(video->videoinstance.output_textures_used.size()) + "\n\n";
			infoLabel.SetText(str);

			timerSlider.SetRange(0, videofile.duration_seconds);
			timerSlider.SetValue(video->currentTimer);
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
	layout.margin_left = 80;

	layout.add_fullwidth(openButton);
	layout.add_fullwidth(filenameLabel);

	layout.add_fullwidth(preview);
	float h_aspect = 9.0f / 16.0f;
	if (preview.video != nullptr && preview.video->videoResource.IsValid())
	{
		const wi::video::Video& video = preview.video->videoResource.GetVideo();
		h_aspect = (float)video.height / (float)video.width;
	}
	preview.SetSize(XMFLOAT2(preview.GetSize().x, preview.GetSize().x * h_aspect));

	layout.add(playpauseButton);
	playpauseButton.SetSize(XMFLOAT2(layout.width - 140, playpauseButton.GetSize().y));
	loopedCheckbox.SetPos(XMFLOAT2(playpauseButton.GetPos().x - loopedCheckbox.GetSize().x - 2, playpauseButton.GetPos().y));
	stopButton.SetPos(XMFLOAT2(playpauseButton.GetPos().x + playpauseButton.GetSize().x + 2, playpauseButton.GetPos().y));
	stopButton.SetSize(XMFLOAT2(layout.width - stopButton.GetPos().x - layout.padding, playpauseButton.GetSize().y));

	layout.add(timerSlider);

	layout.jump();

	layout.add_fullwidth(infoLabel);
}

