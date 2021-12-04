#include "stdafx.h"
#include "LayerWindow.h"
#include "Editor.h"

using namespace wi::ecs;
using namespace wi::scene;


void LayerWindow::Create(EditorComponent* editor)
{
	wi::gui::Window::Create("Layer Window");
	SetSize(XMFLOAT2(410, 290));

	float x = 30;
	float y = 25;
	float step = 25;
	float siz = 20;

	label.Create("LayerWindowLabel");
	label.SetText("The layer is a 32-bit mask (uint32_t), which can be used for filtering by multiple systems (visibility, collision, picking, etc.).\n- If all bits are disabled, it means the layer will be inactive in most systems.\n- For ray tracing, the lower 8 bits will be used as instance inclusion mask.");
	label.SetPos(XMFLOAT2(x, y));
	label.SetSize(XMFLOAT2(370, 120));
	label.SetColor(wi::Color::Transparent());
	AddWidget(&label);
	y += label.GetScale().y + 5;

	for (uint32_t i = 0; i < arraysize(layers); ++i)
	{
		layers[i].Create("");
		layers[i].SetText(std::to_string(i) + ": ");
		layers[i].SetPos(XMFLOAT2(x + (i % 8) * 50, y + (i / 8) * step));
		layers[i].OnClick([=](wi::gui::EventArgs args) {

			LayerComponent* layer = wi::scene::GetScene().layers.GetComponent(entity);
			if (layer == nullptr)
			{
				layer = &wi::scene::GetScene().layers.Create(entity);
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

	y += step * 4;

	enableAllButton.Create("Enable ALL");
	enableAllButton.SetPos(XMFLOAT2(x, y));
	enableAllButton.OnClick([this](wi::gui::EventArgs args) {
		LayerComponent* layer = wi::scene::GetScene().layers.GetComponent(entity);
		if (layer == nullptr)
		{
			layer = &wi::scene::GetScene().layers.Create(entity);
		}
		if (layer == nullptr)
			return;
		layer->layerMask = ~0;
	});
	AddWidget(&enableAllButton);

	enableNoneButton.Create("Enable NONE");
	enableNoneButton.SetPos(XMFLOAT2(x + 120, y));
	enableNoneButton.OnClick([this](wi::gui::EventArgs args) {
		LayerComponent* layer = wi::scene::GetScene().layers.GetComponent(entity);
		if (layer == nullptr)
		{
			layer = &wi::scene::GetScene().layers.Create(entity);
		}
		if (layer == nullptr)
			return;
		layer->layerMask = 0;
	});
	AddWidget(&enableNoneButton);

	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 450, 300, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void LayerWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	if (entity != INVALID_ENTITY)
	{
		SetEnabled(true);

		LayerComponent* layer = wi::scene::GetScene().layers.GetComponent(entity);
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

			HierarchyComponent* hier = wi::scene::GetScene().hierarchy.GetComponent(entity);
			if (hier != nullptr)
			{
				hier->layerMask_bind = layer->layerMask;
			}

			MaterialComponent* material = wi::scene::GetScene().materials.GetComponent(entity);
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
