#include "stdafx.h"
#include "MaterialWindow.h"

using namespace wi::graphics;
using namespace wi::ecs;
using namespace wi::scene;

void MaterialPickerWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create("Materials", wi::gui::Window::WindowControls::COLLAPSE);
	SetSize(XMFLOAT2(300, 400));

	zoomSlider.Create(10, 100, 100, 100 - 10, "Zoom: ");
	AddWidget(&zoomSlider);

	SetCollapsed(true);
}

void MaterialPickerWindow::RecreateButtons()
{
	if (editor == nullptr)
		return;
	const wi::scene::Scene& scene = editor->GetCurrentScene();
	for (auto& x : buttons)
	{
		RemoveWidget(&x);
	}
	buttons.resize(scene.materials.GetCount());

	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		Entity entity = scene.materials.GetEntity(i);

		wi::gui::Button& button = buttons[i];
		button.Create("");
		AddWidget(&button);
		button.SetVisible(false);

		button.OnClick([entity, this](wi::gui::EventArgs args) {

			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			editor->RecordSelection(archive);

			if (!editor->translator.selected.empty() && wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT))
			{
				// Union selection:
				wi::vector<wi::scene::PickResult> saved = editor->translator.selected;
				editor->translator.selected.clear();
				for (const wi::scene::PickResult& picked : saved)
				{
					editor->AddSelected(picked);
				}
				editor->AddSelected(entity);
			}
			else
			{
				// Replace selection:
				editor->translator.selected.clear();
				editor->AddSelected(entity);
			}

			// record NEW selection state...
			editor->RecordSelection(archive);
			});
	}

	ResizeLayout();
}

void MaterialPickerWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();

	if (editor == nullptr || IsCollapsed() || !IsVisible())
	{
		return;
	}

	const wi::scene::Scene& scene = editor->GetCurrentScene();
	if (buttons.size() != scene.materials.GetCount())
	{
		RecreateButtons();
		return;
	}

	zoomSlider.SetPos(XMFLOAT2(55, 0));
	zoomSlider.SetSize(XMFLOAT2(GetWidgetAreaSize().x - 100 - 5, 20));

	wi::gui::Theme theme;
	theme.image.CopyFrom(sprites[wi::gui::IDLE].params);
	theme.image.background = false;
	theme.image.blendFlag = wi::enums::BLENDMODE_ALPHA;
	theme.font.CopyFrom(font.params);
	theme.shadow_color = wi::Color::lerp(theme.font.color, wi::Color::Transparent(), 0.25f);
	theme.tooltipFont.CopyFrom(tooltipFont.params);
	theme.tooltipImage.CopyFrom(tooltipSprite.params);

	const float preview_size = zoomSlider.GetValue();
	const float border = 20 * preview_size / 100.0f;
	int cells = std::max(1, int(GetWidgetAreaSize().x / (preview_size + border)));
	float offset_y = border + zoomSlider.GetSize().y;

	for (size_t i = 0; i < scene.materials.GetCount(); ++i)
	{
		const MaterialComponent& material = scene.materials[i];
		Entity entity = scene.materials.GetEntity(i);

		wi::gui::Button& button = buttons[i];
		button.SetVisible(IsVisible() && !IsCollapsed());

		button.SetTheme(theme);
		button.SetColor(wi::Color::White());
		button.SetColor(wi::Color(255, 255, 255, 150), wi::gui::IDLE);
		button.SetShadowRadius(0);

		for (auto& picked : editor->translator.selected)
		{
			if (picked.entity == entity)
			{
				button.SetColor(wi::Color::White());
				button.SetShadowRadius(3);
				break;
			}
		}

		// find first good texture that we can show:
		button.SetImage({});
		for (auto& slot : material.textures)
		{
			if (slot.resource.IsValid())
			{
				button.SetImage(slot.resource);
				break;
			}
		}

		const NameComponent* name = scene.names.GetComponent(entity);
		if (name != nullptr)
		{
			if (preview_size >= 75)
			{
				button.SetText(name->name);
			}
			else
			{
				button.SetText("");
			}
			button.SetTooltip(name->name);
		}
		button.font.params.h_align = wi::font::WIFALIGN_CENTER;
		button.font.params.v_align = wi::font::WIFALIGN_BOTTOM;

		button.SetSize(XMFLOAT2(preview_size, preview_size));
		button.SetPos(XMFLOAT2((i % cells) * (preview_size + border) + border, offset_y));
		if ((i % cells) == (cells - 1))
		{
			offset_y += preview_size + border;
		}
	}
}
