#include "stdafx.h"
#include "MaterialWindow.h"
#include "Editor.h"

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

	SetEntity(INVALID_ENTITY);
}



void MaterialPickerWindow::SetEntity(Entity entity)
{
	this->entity = entity;
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

		button.OnClick([entity, this](wi::gui::EventArgs args) {

			wi::Archive& archive = editor->AdvanceHistory();
			archive << EditorComponent::HISTORYOP_SELECTION;
			// record PREVIOUS selection state...
			editor->RecordSelection(archive);

			if (!wi::input::Down(wi::input::KEYBOARD_BUTTON_LSHIFT))
			{
				editor->ClearSelected();
			}
			editor->AddSelected(entity);

			// record NEW selection state...
			editor->RecordSelection(archive);
			});
	}

	Update();
}

void MaterialPickerWindow::Update()
{
	if (editor == nullptr || IsCollapsed() || !IsVisible())
	{
		return;
	}

	const wi::scene::Scene& scene = editor->GetCurrentScene();

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

	const float border = 20;
	const float preview_size = zoomSlider.GetValue();
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
		button.SetShadowRadius(3);

		if (this->entity != entity)
		{
			button.SetColor(wi::Color(255, 255, 255, 150), wi::gui::IDLE);
			button.SetShadowRadius(0);
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
				button.SetDescription(name->name);
			}
			else
			{
				button.SetDescription("");
			}
			button.SetTooltip(name->name);
		}
		button.font_description.params.h_align = wi::font::WIFALIGN_CENTER;
		button.font_description.params.v_align = wi::font::WIFALIGN_TOP;

		button.SetSize(XMFLOAT2(preview_size, preview_size));
		button.SetPos(XMFLOAT2((i % cells) * (preview_size + border) + border, offset_y));
		if ((i % cells) == (cells - 1))
		{
			offset_y += preview_size + border;
		}
	}

	wi::gui::Window::Update(*editor, 0);
}
