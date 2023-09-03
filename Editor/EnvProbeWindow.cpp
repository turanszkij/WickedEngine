#include "stdafx.h"
#include "EnvProbeWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

const std::string default_text = "Environment probes can be used to capture the scene from a specific location in a 360 degrees panorama. The probes will be used for reflections fallback, where a better reflection type is not available. The probes can affect the ambient colors slightly.\nTip: You can scale, rotate and move the probes to set up parallax correct rendering to affect a specific area only. The parallax correction will take effect inside the probe's bounds (indicated with a cyan colored box).";

void EnvProbeWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_ENVIRONMENTPROBE " Environment Probe", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(420, 340));

	closeButton.SetTooltip("Delete EnvironmentProbeComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().probes.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->optionsWnd.RefreshEntityTree();
		});

	float x = 5, y = 0, step = 35;

	infoLabel.Create("");
	infoLabel.SetSize(XMFLOAT2(300, 120));
	infoLabel.SetPos(XMFLOAT2(x, y));
	infoLabel.SetColor(wi::Color::Transparent());
	AddWidget(&infoLabel);
	y += infoLabel.GetScale().y + 5;

	realTimeCheckBox.Create("RealTime: ");
	realTimeCheckBox.SetTooltip("Enable continuous rendering of the probe in every frame.");
	realTimeCheckBox.SetPos(XMFLOAT2(x + 100, y));
	realTimeCheckBox.SetEnabled(false);
	realTimeCheckBox.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetRealTime(args.bValue);
			probe->SetDirty();
		}
	});
	AddWidget(&realTimeCheckBox);

	msaaCheckBox.Create("MSAA: ");
	msaaCheckBox.SetTooltip("Enable Multi Sampling Anti Aliasing for the probe, this will improve its quality.");
	msaaCheckBox.SetPos(XMFLOAT2(x + 200, y));
	msaaCheckBox.SetEnabled(false);
	msaaCheckBox.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetMSAA(args.bValue);
			probe->SetDirty();
		}
		});
	AddWidget(&msaaCheckBox);

	refreshButton.Create("Refresh");
	refreshButton.SetTooltip("Re-renders the selected probe.");
	refreshButton.SetPos(XMFLOAT2(x, y+= step));
	refreshButton.SetEnabled(false);
	refreshButton.OnClick([&](wi::gui::EventArgs args) {
		EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->SetDirty();
		}
	});
	AddWidget(&refreshButton);

	refreshAllButton.Create("Refresh All");
	refreshAllButton.SetTooltip("Re-renders all probes in the scene.");
	refreshAllButton.SetPos(XMFLOAT2(x + 120, y));
	refreshAllButton.SetEnabled(true);
	refreshAllButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		for (size_t i = 0; i < scene.probes.GetCount(); ++i)
		{
			EnvironmentProbeComponent& probe = scene.probes[i];
			probe.SetDirty();
		}
	});
	AddWidget(&refreshAllButton);

	importButton.Create("Import Cubemap");
	importButton.SetTooltip("Import a DDS texture file into the selected environment probe.");
	importButton.SetPos(XMFLOAT2(x, y += step));
	importButton.SetEnabled(false);
	importButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		EnvironmentProbeComponent* probe = scene.probes.GetComponent(entity);
		if (probe != nullptr && probe->texture.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::OPEN;
			params.description = "DDS";
			params.extensions = { "DDS" };
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

					wi::Resource resource = wi::resourcemanager::Load(fileName, wi::resourcemanager::Flags::IMPORT_RETAIN_FILEDATA);
					if (has_flag(resource.GetTexture().GetDesc().misc_flags, wi::graphics::ResourceMiscFlag::TEXTURECUBE))
					{
						probe->textureName = fileName;
						probe->CreateRenderData();
					}
					else
					{
						wi::helper::messageBox("Error!", "The texture you tried to open is not a cubemap texture, so it won't be imported!");
					}

					});
				});

		}
		});
	AddWidget(&importButton);

	exportButton.Create("Export Cubemap");
	exportButton.SetTooltip("Export the selected probe into a DDS cubemap texture file.");
	exportButton.SetPos(XMFLOAT2(x, y += step));
	exportButton.SetEnabled(false);
	exportButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		EnvironmentProbeComponent* probe = scene.probes.GetComponent(entity);
		if (probe != nullptr && probe->texture.IsValid())
		{
			wi::helper::FileDialogParams params;
			params.type = wi::helper::FileDialogParams::SAVE;
			params.description = "DDS";
			params.extensions = { "DDS" };
			wi::helper::FileDialog(params, [=](std::string fileName) {
				wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {

					std::string extension = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(fileName));
					std::string filename_replaced = fileName;
					if (extension != "DDS")
					{
						filename_replaced = wi::helper::ReplaceExtension(fileName, "DDS");
					}

					bool success = wi::helper::saveTextureToFile(probe->texture, filename_replaced);
					assert(success);

					if (success)
					{
						editor->PostSaveText("Exported environment cubemap: ", filename_replaced);
					}

					});
				});

		}
	});
	AddWidget(&exportButton);


	resolutionCombo.Create("Resolution: ");
	resolutionCombo.SetTooltip("Set the resolution of the selected environment probe. Only takes effect if this is a rendered probe, not for probes that are imported from files.");
	resolutionCombo.AddItem("32", 32);
	resolutionCombo.AddItem("64", 64);
	resolutionCombo.AddItem("128", 128);
	resolutionCombo.AddItem("256", 256);
	resolutionCombo.AddItem("512", 512);
	resolutionCombo.AddItem("1024", 1024);
	resolutionCombo.AddItem("2048", 2048);
	resolutionCombo.OnSelect([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		EnvironmentProbeComponent* probe = scene.probes.GetComponent(entity);
		if (probe != nullptr)
		{
			probe->resolution = (uint32_t)args.userdata;
			probe->CreateRenderData();
		}
	});
	AddWidget(&resolutionCombo);


	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void EnvProbeWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const EnvironmentProbeComponent* probe = scene.probes.GetComponent(entity);

	if (probe == nullptr)
	{
		realTimeCheckBox.SetEnabled(false);
		msaaCheckBox.SetEnabled(false);
		refreshButton.SetEnabled(false);
	}
	else
	{
		realTimeCheckBox.SetCheck(probe->IsRealTime());
		realTimeCheckBox.SetEnabled(true);
		msaaCheckBox.SetCheck(probe->IsMSAA());
		msaaCheckBox.SetEnabled(true);
		refreshButton.SetEnabled(true);
		resolutionCombo.SetSelectedByUserdata(probe->resolution);

		std::string text =
			"GPU Memory usage: " + wi::helper::GetMemorySizeText(probe->GetMemorySizeInBytes()) + "\n" +
			"Resolution: " + std::to_string(probe->texture.desc.width) + "\n" +
			"Mipmaps: " + std::to_string(probe->texture.desc.mip_levels) + "\n" +
			"Format: " + std::string(wi::graphics::GetFormatString(probe->texture.desc.format)) + "\n"
			;
		if (!probe->textureName.empty())
		{
			text += "Filename: " + probe->textureName + "\n";
		}
		text += "\n" + default_text;
		infoLabel.SetText(text);
	}

}


void EnvProbeWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	const float margin_left = 80;
	const float margin_right = padding;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
	};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
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

	add_fullwidth(infoLabel);

	refreshButton.SetSize(XMFLOAT2(width * 0.5f - padding * 1.5f, refreshButton.GetSize().y));
	refreshAllButton.SetSize(refreshButton.GetSize());
	refreshAllButton.SetPos(XMFLOAT2(width - padding - refreshButton.GetSize().x, y));
	refreshButton.SetPos(XMFLOAT2(refreshAllButton.GetPos().x - padding - refreshButton.GetSize().x, y));
	y += refreshAllButton.GetSize().y;
	y += padding;

	importButton.SetSize(XMFLOAT2(width - padding * 2, importButton.GetSize().y));
	importButton.SetPos(XMFLOAT2(padding, y));
	y += importButton.GetSize().y;
	y += padding;

	exportButton.SetSize(XMFLOAT2(width - padding * 2, exportButton.GetSize().y));
	exportButton.SetPos(XMFLOAT2(padding, y));
	y += exportButton.GetSize().y;
	y += padding;

	resolutionCombo.SetSize(XMFLOAT2(width - 100 - resolutionCombo.GetSize().y - padding, resolutionCombo.GetSize().y));
	resolutionCombo.SetPos(XMFLOAT2(100, y));
	y += resolutionCombo.GetSize().y;
	y += padding;

	add_right(realTimeCheckBox);
	add_right(msaaCheckBox);


}
