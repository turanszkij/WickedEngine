#pragma once
class EditorComponent;

class ColliderWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	void SetEntity(wi::ecs::Entity entity);

	wi::gui::Label infoLabel;
	wi::gui::CheckBox cpuCheckBox;
	wi::gui::CheckBox gpuCheckBox;
	wi::gui::ComboBox shapeCombo;
	wi::gui::Slider radiusSlider;
	wi::gui::Slider offsetX;
	wi::gui::Slider offsetY;
	wi::gui::Slider offsetZ;
	wi::gui::Slider tailX;
	wi::gui::Slider tailY;
	wi::gui::Slider tailZ;

	void ResizeLayout() override;
};

