#include "stdafx.h"
#include "EnvProbeWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void EnvProbeWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_ENVIRONMENTPROBE " Environment Probe", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(420, 260));

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
	infoLabel.SetText("Environment probes can be used to capture the scene from a specific location in a 360 degrees panorama. The probes will be used for reflections fallback, where a better reflection type is not available. The probes can affect the ambient colors slightly.\nTip: You can scale, rotate and move the probes to set up parallax correct rendering to affect a specific area only. The parallax correction will take effect inside the probe's bounds (indicated with a cyan colored box).");
	infoLabel.SetSize(XMFLOAT2(300, 100));
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

	exportButton.Create("Export Cubemap");
	exportButton.SetTooltip("Export the selected probe into a DDS cubemap texture file.");
	exportButton.SetPos(XMFLOAT2(x, y += step));
	exportButton.SetEnabled(false);
	exportButton.OnClick([&](wi::gui::EventArgs args) {
		Scene& scene = editor->GetCurrentScene();
		EnvironmentProbeComponent* probe = scene.probes.GetComponent(entity);
		if (probe != nullptr && probe->textureIndex >= 0)
		{
			using namespace wi::graphics;
			GraphicsDevice* device = GetDevice();
			TextureDesc desc = scene.envmapArray.desc;
			desc.array_size = 6;
			Texture singleprobe;
			bool success = device->CreateTexture(&desc, nullptr, &singleprobe);
			assert(success);
			CommandList cmd = device->BeginCommandList();
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&scene.envmapArray, desc.layout, ResourceState::COPY_SRC),
				GPUBarrier::Image(&singleprobe, desc.layout, ResourceState::COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
			for (uint32_t face = 0; face < 6; ++face)
			{
				for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
				{
					device->CopyTexture(
						&singleprobe, 0, 0, 0, mip, face,
						&scene.envmapArray, mip, probe->textureIndex * 6 + face,
						cmd
					);
				}
			}
			for (auto& x : barriers)
			{
				std::swap(x.image.layout_before, x.image.layout_after);
			}
			device->Barrier(barriers, arraysize(barriers), cmd);
			device->SubmitCommandLists();

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

					bool success = wi::helper::saveTextureToFile(singleprobe, filename_replaced);
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




	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void EnvProbeWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	const EnvironmentProbeComponent* probe = editor->GetCurrentScene().probes.GetComponent(entity);

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
	exportButton.SetSize(XMFLOAT2(width - padding * 2, exportButton.GetSize().y));
	exportButton.SetPos(XMFLOAT2(padding, y));
	y += exportButton.GetSize().y;

	y += padding;

	add_right(realTimeCheckBox);
	add_right(msaaCheckBox);


}
