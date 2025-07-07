#include "stdafx.h"
#include "LayerWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void LayerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_LAYER " Layer", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(300, 350));

	closeButton.SetTooltip("Delete LayerComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().layers.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	label.Create("LayerWindowLabel");
	label.SetText("The layer is a 32-bit mask (uint32_t), which can be used for filtering by multiple systems (visibility, collision, picking, scripts, etc.).\n- If all bits are disabled, it means the layer will be inactive in most systems.");
	label.SetFitTextEnabled(true);
	AddWidget(&label);

	for (uint32_t i = 0; i < arraysize(layers); ++i)
	{
		layers[i].Create("");
		layers[i].SetText(std::to_string(i) + ": ");
		layers[i].OnClick([=](wi::gui::EventArgs args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				LayerComponent* layer = scene.layers.GetComponent(x.entity);
				if (layer == nullptr)
				{
					layer = &editor->GetCurrentScene().layers.Create(x.entity);
				}

				if (args.bValue)
				{
					layer->layerMask |= 1 << i;
				}
				else
				{
					layer->layerMask &= ~(1 << i);
				}
			}
		});
		AddWidget(&layers[i]);
	}

	auto forEachLayer = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				LayerComponent* layer = scene.layers.GetComponent(x.entity);
				if (layer == nullptr)
				{
					layer = &editor->GetCurrentScene().layers.Create(x.entity);
				}
				if (layer != nullptr)
				{
					func(layer, args);
				}
			}
		};
	};

	enableAllButton.Create("ALL " ICON_CHECK);
	enableAllButton.OnClick(forEachLayer([] (auto layer, auto args) {
		layer->layerMask = ~0;
	}));
	AddWidget(&enableAllButton);

	enableNoneButton.Create("NONE " ICON_DISABLED);
	enableNoneButton.OnClick(forEachLayer([] (auto layer, auto args) {
		layer->layerMask = 0;
	}));
	AddWidget(&enableNoneButton);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void LayerWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		SetEnabled(true);

		LayerComponent* layer = editor->GetCurrentScene().layers.GetComponent(entity);
		if (layer == nullptr)
		{
			for (uint32_t i = 0; i < 32; ++i)
			{
				layers[i].SetCheck(true);
			}
		}
		else
		{
			for (uint32_t i = 0; i < 32; ++i)
			{
				layers[i].SetCheck(layer->GetLayerMask() & 1 << i);
			}

			HierarchyComponent* hier = editor->GetCurrentScene().hierarchy.GetComponent(entity);
			if (hier != nullptr)
			{
				hier->layerMask_bind = layer->layerMask;
			}

			MaterialComponent* material = editor->GetCurrentScene().materials.GetComponent(entity);
			if (material != nullptr)
			{
				material->SetDirty();
			}
		}
	}
	else
	{
		SetEnabled(false);
	}
}

void LayerWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 80;

	layout.add_fullwidth(label);
	enableAllButton.SetSize(XMFLOAT2(layout.width * 0.5f - layout.padding * 1.5f, enableAllButton.GetSize().y));
	enableNoneButton.SetSize(enableAllButton.GetSize());
	enableNoneButton.SetPos(XMFLOAT2(layout.width - layout.padding - enableNoneButton.GetSize().x, layout.y));
	enableAllButton.SetPos(XMFLOAT2(enableNoneButton.GetPos().x - layout.padding - enableAllButton.GetSize().x, layout.y));
	layout.y += enableNoneButton.GetSize().y;
	layout.y += layout.padding;

	float off_x = layout.padding + 30;
	for (uint32_t i = 0; i < arraysize(layers); ++i)
	{
		layers[i].SetPos(XMFLOAT2(off_x, layout.y));
		off_x += 50;
		if (off_x + layers[i].GetSize().x > layout.width - layout.padding)
		{
			off_x = layout.padding + 30;
			layout.y += layers[i].GetSize().y;
			layout.y += layout.padding;
		}
	}

}
