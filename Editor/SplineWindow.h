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
	wi::gui::CheckBox loopedCheck;
	wi::gui::CheckBox alignedCheck;
	wi::gui::Slider widthSlider;
	wi::gui::Slider rotSlider;
	wi::gui::Slider subdivSlider;
	wi::gui::Slider subdivVerticalSlider;
	wi::gui::Slider terrainSlider;
	wi::gui::Slider terrainTexSlider;
	wi::gui::Slider terrainPushdownSlider;
	wi::gui::Button addButton;

	struct Entry
	{
		wi::gui::Button removeButton;
		wi::gui::Button entityButton;
	};
  std::deque<Entry> entries;

	void RefreshEntries();

	void NewNode();

	void ResizeLayout() override;
};

