#include "stdafx.h"
#include "LayerWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void LayerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_LAYER " Layer", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(300, 350));

	closeButton.SetTooltip("Delete LayerComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().layers.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->RefreshEntityTree();
		});

	float x = 30;
	float y = 0;
	float step = 25;
	float siz = 20;
	float wid = 250;

	label.Create("LayerWindowLabel");
	label.SetText("The layer is a 32-bit mask (uint32_t), which can be used for filtering by multiple systems (visibility, collision, picking, etc.).\n- If all bits are disabled, it means the layer will be inactive in most systems.\n- For ray tracing, the lower 8 bits will be used as instance inclusion mask.");
	label.SetPos(XMFLOAT2(x, y));
	label.SetSize(XMFLOAT2(wid, 100));
	label.SetColor(wi::Color::Transparent());
	AddWidget(&label);
	y += label.GetScale().y + 5;

	for (uint32_t i = 0; i < arraysize(layers); ++i)
	{
		layers[i].Create("");
		layers[i].SetText(std::to_string(i) + ": ");
		layers[i].SetPos(XMFLOAT2(x + 20 + (i % 5) * 50, y + (i / 5) * step));
		layers[i].OnClick([=](wi::gui::EventArgs args) {

			LayerComponent* layer = editor->GetCurrentScene().layers.GetComponent(entity);
			if (layer == nullptr)
			{
				layer = &editor->GetCurrentScene().layers.Create(entity);
			}

			if (args.bValue)
			{
				layer->layerMask |= 1 << i;
			}
			else
			{
				layer->layerMask &= ~(1 << i);
			}

			});
		AddWidget(&layers[i]);
	}

	y += step * 7;

	enableAllButton.Create("Enable ALL");
	enableAllButton.SetPos(XMFLOAT2(x, y));
	enableAllButton.OnClick([this](wi::gui::EventArgs args) {
		LayerComponent* layer = editor->GetCurrentScene().layers.GetComponent(entity);
		if (layer == nullptr)
		{
			layer = &editor->GetCurrentScene().layers.Create(entity);
		}
		if (layer == nullptr)
			return;
		layer->layerMask = ~0;
	});
	AddWidget(&enableAllButton);

	enableNoneButton.Create("Enable NONE");
	enableNoneButton.SetPos(XMFLOAT2(x + 120, y));
	enableNoneButton.OnClick([this](wi::gui::EventArgs args) {
		LayerComponent* layer = editor->GetCurrentScene().layers.GetComponent(entity);
		if (layer == nullptr)
		{
			layer = &editor->GetCurrentScene().layers.Create(entity);
		}
		if (layer == nullptr)
			return;
		layer->layerMask = 0;
	});
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
