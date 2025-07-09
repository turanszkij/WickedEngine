#include "stdafx.h"
#include "MetadataWindow.h"

using namespace wi::ecs;
using namespace wi::scene;

void MetadataWindow::Create(EditorComponent* _editor)
{
	editor = _editor;
	wi::gui::Window::Create(ICON_METADATA " Metadata", wi::gui::Window::WindowControls::COLLAPSE | wi::gui::Window::WindowControls::CLOSE | wi::gui::Window::WindowControls::FIT_ALL_WIDGETS_VERTICAL);
	SetSize(XMFLOAT2(300, 240));

	closeButton.SetTooltip("Delete MetadataComponent");
	OnClose([=](wi::gui::EventArgs args) {

		wi::Archive& archive = editor->AdvanceHistory();
		archive << EditorComponent::HISTORYOP_COMPONENT_DATA;
		editor->RecordEntity(archive, entity);

		editor->GetCurrentScene().metadatas.Remove(entity);

		editor->RecordEntity(archive, entity);

		editor->componentsWnd.RefreshEntityTree();
	});

	auto forEachSelected = [this] (auto func) {
		return [this, func] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				MetadataComponent* metadata = scene.metadatas.GetComponent(x.entity);
				if (metadata != nullptr)
				{
					func(metadata, args);
				}
			}
		};
	};

	presetCombo.Create("Preset: ");
	presetCombo.AddItem("Custom", (uint64_t)MetadataComponent::Preset::Custom);
	presetCombo.AddItem("Waypoint", (uint64_t)MetadataComponent::Preset::Waypoint);
	presetCombo.AddItem("Player", (uint64_t)MetadataComponent::Preset::Player);
	presetCombo.AddItem("Enemy", (uint64_t)MetadataComponent::Preset::Enemy);
	presetCombo.AddItem("Npc", (uint64_t)MetadataComponent::Preset::NPC);
	presetCombo.AddItem("Pickup", (uint64_t)MetadataComponent::Preset::Pickup);
	presetCombo.AddItem("Vehicle", (uint64_t)MetadataComponent::Preset::Vehicle);
	presetCombo.OnSelect(forEachSelected([] (auto metadata, auto args) {
		metadata->preset = (MetadataComponent::Preset)args.userdata;
	}));
	AddWidget(&presetCombo);

	addCombo.Create("");
	addCombo.SetInvalidSelectionText("+");
	addCombo.SetDropArrowEnabled(false);
	addCombo.AddItem("bool");
	addCombo.AddItem("int");
	addCombo.AddItem("float");
	addCombo.AddItem("string");
	addCombo.OnSelect([=] (auto args) {
		forEachSelected([] (auto metadata, auto args) {
			std::string property_name = "name";
			switch (args.iValue)
			{
			default:
			case 0:
				while (metadata->bool_values.has(property_name))
					property_name += "0";
				metadata->bool_values.set(property_name, false);
				break;
			case 1:
				while (metadata->int_values.has(property_name))
					property_name += "0";
				metadata->int_values.set(property_name, 0);
				break;
			case 2:
				while (metadata->float_values.has(property_name))
					property_name += "0";
				metadata->float_values.set(property_name, 0.0f);
				break;
			case 3:
				while (metadata->string_values.has(property_name))
					property_name += "0";
				metadata->string_values.set(property_name, "");
				break;
			}
		})(args);
		addCombo.SetSelectedWithoutCallback(-1);
		RefreshEntries();
	});
	AddWidget(&addCombo);

	SetMinimized(true);
	SetVisible(false);

	SetLocalizationEnabled(false);

	SetEntity(INVALID_ENTITY);
}

void MetadataWindow::SetEntity(Entity entity)
{
	bool changed = entity != this->entity;
	this->entity = entity;

	Scene& scene = editor->GetCurrentScene();
	const MetadataComponent* metadata = scene.metadatas.GetComponent(entity);

	if (metadata != nullptr)
	{
		presetCombo.SetSelectedByUserdataWithoutCallback((uint64_t)metadata->preset);

		if (changed)
		{
			RefreshEntries();
		}

		SetEnabled(true);
	}
	else
	{
		this->entity = INVALID_ENTITY;
	}
}

void MetadataWindow::RefreshEntries()
{
	for (auto& entry : entries)
	{
		RemoveWidget(&entry.name);
		RemoveWidget(&entry.value);
		RemoveWidget(&entry.check);
		RemoveWidget(&entry.remove);
	}
	entries.clear();

	Scene& scene = editor->GetCurrentScene();
	MetadataComponent* metadata = scene.metadatas.GetComponent(entity);
	if (metadata == nullptr)
		return;

	entries.reserve(
		metadata->bool_values.size() +
		metadata->int_values.size() +
		metadata->float_values.size() +
		metadata->string_values.size()
	);


	auto forEachSelectedWithRefresh = [this] (auto name, auto func) {
		return [this, func, name] (auto args) {
			wi::scene::Scene& scene = editor->GetCurrentScene();
			for (auto& x : editor->translator.selected)
			{
				MetadataComponent* metadata = scene.metadatas.GetComponent(x.entity);
				if (metadata != nullptr)
				{
					func(metadata, args);
				}
			}
			wi::eventhandler::Subscribe_Once(wi::eventhandler::EVENT_THREAD_SAFE_POINT, [this](uint64_t userdata) {
				RefreshEntries();
			});
		};
	};

	// Note: to not disturb the ordering of entries while editing them, we iterate by the ordered names array in each table

	for (auto& name : metadata->bool_values.names)
	{
		Entry& entry = entries.emplace_back();
		entry.name.Create("");
		entry.name.SetText(name);
		entry.name.OnInputAccepted(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->bool_values.has(name))
				return;
			auto value = metadata->bool_values.get(name);
			metadata->bool_values.erase(name);
			metadata->bool_values.set(args.sValue, value);
		}));
		AddWidget(&entry.name);

		entry.is_bool = true;
		entry.check.Create("");
		entry.check.SetText(" = (bool) ");
		entry.check.SetCheck(metadata->bool_values.get(name));
		entry.check.OnClick(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->bool_values.has(name))
				return;
			metadata->bool_values.set(name, args.bValue);
		}));
		AddWidget(&entry.check);

		entry.remove.Create("");
		entry.remove.SetText("X");
		entry.remove.SetSize(XMFLOAT2(entry.remove.GetSize().y, entry.remove.GetSize().y));
		entry.remove.OnClick(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->bool_values.has(name))
				return;
			metadata->bool_values.erase(name);
		}));
		AddWidget(&entry.remove);
	}

	for (auto& name : metadata->int_values.names)
	{
		Entry& entry = entries.emplace_back();
		entry.name.Create("");
		entry.name.SetText(name);
		entry.name.OnInputAccepted(forEachSelectedWithRefresh(name, [name] (auto metadata, auto args) {
			if (!metadata->int_values.has(name))
				return;
			auto value = metadata->int_values.get(name);
			metadata->int_values.erase(name);
			metadata->int_values.set(args.sValue, value);
		}));
		AddWidget(&entry.name);

		entry.is_bool = false;
		entry.value.Create("");
		entry.value.SetDescription(" = (int) ");
		entry.value.SetSize(XMFLOAT2(60, entry.value.GetSize().y));
		entry.value.SetValue(metadata->int_values.get(name));
		entry.value.OnInputAccepted(forEachSelectedWithRefresh(name, [name] (auto metadata, auto args) {
			if (!metadata->int_values.has(name))
				return;
			metadata->int_values.set(name, args.iValue);
		}));
		AddWidget(&entry.value);

		entry.remove.Create("");
		entry.remove.SetText("X");
		entry.remove.SetSize(XMFLOAT2(entry.remove.GetSize().y, entry.remove.GetSize().y));
		entry.remove.OnClick(forEachSelectedWithRefresh(name, [name] (auto metadata, auto args) {
			if (!metadata->int_values.has(name))
				return;
			metadata->int_values.erase(name);
		}));
		AddWidget(&entry.remove);
	}

	for (auto& name : metadata->float_values.names)
	{
		Entry& entry = entries.emplace_back();
		entry.name.Create("");
		entry.name.SetText(name);
		entry.name.OnInputAccepted(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->float_values.has(name))
				return;
			auto value = metadata->float_values.get(name);
			metadata->float_values.erase(name);
			metadata->float_values.set(args.sValue, value);
		}));
		AddWidget(&entry.name);

		entry.is_bool = false;
		entry.value.Create("");
		entry.value.SetDescription(" = (float) ");
		entry.value.SetSize(XMFLOAT2(60, entry.value.GetSize().y));
		entry.value.SetValue(metadata->float_values.get(name));
		entry.value.OnInputAccepted(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->float_values.has(name))
				return;
			metadata->float_values.set(name, args.fValue);
		}));
		AddWidget(&entry.value);

		entry.remove.Create("");
		entry.remove.SetText("X");
		entry.remove.SetSize(XMFLOAT2(entry.remove.GetSize().y, entry.remove.GetSize().y));
		entry.remove.OnClick(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->float_values.has(name))
				return;
			metadata->float_values.erase(name);
		}));
		AddWidget(&entry.remove);
	}

	for (auto& name : metadata->string_values.names)
	{
		Entry& entry = entries.emplace_back();
		entry.name.Create("");
		entry.name.SetText(name);
		entry.name.OnInputAccepted(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->string_values.has(name))
				return;
			auto value = metadata->string_values.get(name);
			metadata->string_values.erase(name);
			metadata->string_values.set(args.sValue, value);
		}));
		AddWidget(&entry.name);

		entry.is_bool = false;
		entry.value.Create("");
		entry.value.SetDescription(" = (string) ");
		entry.value.SetSize(XMFLOAT2(120, entry.value.GetSize().y));
		entry.value.SetValue(metadata->string_values.get(name));
		entry.value.OnInputAccepted(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->string_values.has(name))
				return;
			metadata->string_values.set(name, args.sValue);
		}));
		AddWidget(&entry.value);

		entry.remove.Create("");
		entry.remove.SetText("X");
		entry.remove.SetSize(XMFLOAT2(entry.remove.GetSize().y, entry.remove.GetSize().y));
		entry.remove.OnClick(forEachSelectedWithRefresh(name, [&name] (auto metadata, auto args) {
			if (!metadata->string_values.has(name))
				return;
			metadata->string_values.erase(name);
		}));
		AddWidget(&entry.remove);
	}

	editor->generalWnd.RefreshTheme();
}

void MetadataWindow::ResizeLayout()
{
	wi::gui::Window::ResizeLayout();
	layout.margin_left = 100;

	layout.add(presetCombo);
	layout.add_fullwidth(addCombo);

	layout.jump();

	for (auto& entry : entries)
	{
		entry.remove.SetPos(XMFLOAT2(layout.padding, layout.y));

		if (entry.is_bool)
		{
			entry.check.SetPos(XMFLOAT2(layout.width - layout.padding - entry.check.GetSize().x, layout.y));

			entry.name.SetSize(XMFLOAT2(layout.width - wi::font::TextWidth(entry.check.GetText(), entry.check.font.params) - layout.padding * 3 - entry.check.GetSize().x - entry.remove.GetSize().x, entry.name.GetSize().y));
		}
		else
		{
			entry.value.SetSize(XMFLOAT2(std::max(wi::font::TextWidth(entry.value.GetCurrentInputValue(), entry.value.font.params) + layout.padding, entry.value.GetSize().y), entry.value.GetSize().y));
			entry.value.SetPos(XMFLOAT2(layout.width - layout.padding - entry.value.GetSize().x, layout.y));

			entry.name.SetSize(XMFLOAT2(layout.width - wi::font::TextWidth(entry.value.GetDescription(), entry.value.font.params) - layout.padding * 3 - entry.value.GetSize().x - entry.remove.GetSize().x, entry.name.GetSize().y));
		}

		entry.name.SetPos(XMFLOAT2(entry.remove.GetPos().x + entry.remove.GetSize().x + layout.padding, layout.y));

		layout.y += entry.name.GetSize().y;
		layout.y += layout.padding;
		layout.y += entry.name.GetShadowRadius();
	}

}
