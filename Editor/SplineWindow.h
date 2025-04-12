#pragma once
class EditorComponent;

class SplineWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::Button addButton;

	struct Entry
	{
		wi::gui::Button removeButton;
		wi::gui::Button entityButton;
	};
	wi::vector<Entry> entries;

	void RefreshEntries();

	void ResizeLayout() override;
};

