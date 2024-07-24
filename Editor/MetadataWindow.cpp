#include "stdafx.h"
#include "MetadataWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void MetadataWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_METADATA " Metadata", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE);
	SetSize(XMFLOAT2(300, 200));

	closeButton.SetTooltip("Delete MetadataComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().metadatas.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	presetCombo.Create("Preset: ");
	presetCombo.AddItem("Custom", (uint64_t)MetadataComponent::Preset::Custom);
	presetCombo.AddItem("Waypoint", (uint64_t)MetadataComponent::Preset::Waypoint);
	presetCombo.AddItem("Player", (uint64_t)MetadataComponent::Preset::Player);
	presetCombo.AddItem("Enemy", (uint64_t)MetadataComponent::Preset::Enemy);
	presetCombo.AddItem("Npc", (uint64_t)MetadataComponent::Preset::NPC);
	presetCombo.AddItem("Pickup", (uint64_t)MetadataComponent::Preset::Pickup);
	presetCombo.OnSelect([this](wi::gui::EventArgs args) {
		wi::scene::Scene& scene = editor->GetCurrentScene();
		for (auto& x : editor->translator.selected)
		{
			MetadataComponent* metadata = scene.metadatas.GetComponent(x.entity);
			if (metadata == nullptr)
				continue;
			metadata->preset = (MetadataComponent::Preset)args.userdata;
		}
	});
	AddWidget(&presetCombo);

	SetMinimized(true);
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}

void MetadataWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const MetadataComponent* metadata = scene.metadatas.GetComponent(entity);

	if (metadata != nullptr)
	{
		presetCombo.SetSelectedByUserdataWithoutCallback((uint64_t)metadata->preset);
	}
}

void MetadataWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	const float padding = 4;
	const float width = GetWidgetAreaSize().x;
	float y = padding;
	float jump = 20;

	auto add = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_left = 100;
		const float margin_right = 40;
		widget.SetPos(XMFLOAT2(margin_left, y));
		widget.SetSize(XMFLOAT2(width - margin_left - margin_right, widget.GetScale().y));
		y += widget.GetSize().y;
		y += padding;
		};
	auto add_right = [&](wi::gui::Widget& widget) {
		if (!widget.IsVisible())
			return;
		const float margin_right = padding;
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

	add(presetCombo);

}
