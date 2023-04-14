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
	if (!video->videoinstance.output_rgb.IsValid())
	{
		params.color = wi::Color::Black();
	}
	wi::image::Draw(&video->videoinstance.output_rgb, params, cmd);

	device->EventEnd(cmd);
}

void VideoWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_VIDEO " Video", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(440, 300));

	closeButton.SetTooltip("Delete VideoComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().videos.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
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
					video->videoResource = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
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

	playstopButton.Create(ICON_PLAY);
	playstopButton.SetTooltip("Play/Stop selected video instance.");
	playstopButton.SetPos(XMFLOAT2(x, y += step));
	playstopButton.SetSize(XMFLOAT2(wid, hei));
	playstopButton.OnClick([&](wi::gui::EventArgs args) {
		VideoComponent* video = editor->GetCurrentScene().videos.GetComponent(entity);
		if (video != nullptr)
		{
			if (video->IsPlaying())
			{
				video->Stop();
				playstopButton.SetText(ICON_PLAY);
			}
			else
			{
				video->Play();
				playstopButton.SetText(ICON_STOP);
			}
		}
		});
	AddWidget(&playstopButton);

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

	preview.SetSize(XMFLOAT2(160, 90));
	AddWidget(&preview);


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
		playstopButton.SetEnabled(true);
		loopedCheckbox.SetEnabled(true);
		loopedCheckbox.SetCheck(video->IsLooped());
		if (video->IsPlaying())
		{
			playstopButton.SetText(ICON_STOP);
		}
		else
		{
			playstopButton.SetText(ICON_PLAY);
		}
	}
	else
	{
		filenameLabel.SetText("");
		playstopButton.SetEnabled(false);
		loopedCheckbox.SetEnabled(false);
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
	add(playstopButton);
	loopedCheckbox.SetPos(XMFLOAT2(playstopButton.GetPos().x - loopedCheckbox.GetSize().x - 2, playstopButton.GetPos().y));

	add_fullwidth(preview);
	float h_aspect = 9.0f / 16.0f;
	if (preview.video != nullptr && preview.video->videoResource.IsValid())
	{
		const wi::video::Video& video = preview.video->videoResource.GetVideo();
		h_aspect = (float)video.height / (float)video.width;
	}
	preview.SetSize(XMFLOAT2(preview.GetSize().x, preview.GetSize().x* h_aspect));
}

