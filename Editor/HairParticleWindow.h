#pragma once
class EditorComponent;

class MaterialWindow;

class HairParticleWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void UpdateData();

	wi::HairParticleSystem* GetHair();

	wi::gui::Label infoLabel;
	wi::gui::ComboBox meshComboBox;
	wi::gui::Slider lengthSlider;
	wi::gui::Slider stiffnessSlider;
	wi::gui::Slider randomnessSlider;
	wi::gui::Slider countSlider;
	wi::gui::Slider segmentcountSlider;
	wi::gui::Slider randomSeedSlider;
	wi::gui::Slider viewDistanceSlider;
	wi::gui::TextInputField framesXInput;
	wi::gui::TextInputField framesYInput;
	wi::gui::TextInputField frameCountInput;
	wi::gui::TextInputField frameStartInput;

	void ResizeLayout() override;
};

