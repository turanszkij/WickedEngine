#pragma once
class EditorComponent;

class MaterialPickerWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;

	std::deque<wi::gui::Button> buttons;

	wi::gui::Slider zoomSlider;
	wi::gui::CheckBox showNoTransformCheckBox;
	wi::gui::CheckBox showInternalCheckBox;

	void ResizeLayout() override;

	// Recreating buttons shouldn't be done every frame because interaction states need multi-frame execution
	void RecreateButtons();

private:
	bool ShouldShowMaterial(const wi::scene::MaterialComponent& material, wi::ecs::Entity entity) const;
	bool MaterialHasTransform(wi::ecs::Entity materialEntity) const;
};

