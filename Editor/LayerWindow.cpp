#include "stdafx.h"
#include "LayerWindow.h"

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

		editor->componentsWnd.RefreshEntityTree();
	});

	float x = 30;
	float y = 0;
	float step = 25;
	float siz = 20;
	float wid = 250;

	label.Create("LayerWindowLabel");
	label.SetText("The layer is a 32-bit mask (uint32_t), which can be used for filtering by multiple systems (visibility, collision, picking, scripts, etc.).\n- If all bits are disabled, it means the layer will be inactive in most systems.");
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

	y += step * 7;

	enableAllButton.Create("ALL " ICON_CHECK);
	enableAllButton.SetPos(XMFLOAT2(x, y));
	enableAllButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			LayerComponent* layer = scene.layers.GetComponent(x.entity);
			if (layer == nullptr)
			{
				layer = &editor->GetCurrentScene().layers.Create(x.entity);
			}
			if (layer == nullptr)
				return;
			layer->layerMask = ~0;
		}
	});
	AddWidget(&enableAllButton);

	enableNoneButton.Create("NONE " ICON_DISABLED);
	enableNoneButton.SetPos(XMFLOAT2(x + 120, y));
	enableNoneButton.OnClick([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			LayerComponent* layer = scene.layers.GetComponent(x.entity);
			if (layer == nullptr)
			{
				layer = &editor->GetCurrentScene().layers.Create(x.entity);
			}
			if (layer == nullptr)
				return;
			layer->layerMask = 0;
		}
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

void LayerWindow::ResizeLayout()
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

	add_fullwidth(label);
	enableAllButton.SetSize(XMFLOAT2(width * 0.5f - padding * 1.5f, enableAllButton.GetSize().y));
	enableNoneButton.SetSize(enableAllButton.GetSize());
	enableNoneButton.SetPos(XMFLOAT2(width - padding - enableNoneButton.GetSize().x, y));
	enableAllButton.SetPos(XMFLOAT2(enableNoneButton.GetPos().x - padding - enableAllButton.GetSize().x, y));
	y += enableNoneButton.GetSize().y;
	y += padding;

	float off_x = padding + 30;
	for (uint32_t i = 0; i < arraysize(layers); ++i)
	{
		layers[i].SetPos(XMFLOAT2(off_x, y));
		off_x += 50;
		if (off_x + layers[i].GetSize().x > width - padding)
		{
			off_x = padding + 30;
			y += layers[i].GetSize().y;
			y += padding;
		}
	}

}
