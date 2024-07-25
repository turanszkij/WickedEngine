#pragma once
class EditorComponent;

class MetadataWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::ComboBox presetCombo;
	wi::gui::ComboBox addCombo;

	struct Entry
	{
		wi::gui::Button remove;
		wi::gui::TextInputField name;
		wi::gui::TextInputField value;
		wi::gui::CheckBox check;
		bool is_bool = false;
	};
	wi::vector<Entry> entries;

	void RefreshEntries();

	void ResizeLayout() override;
};

