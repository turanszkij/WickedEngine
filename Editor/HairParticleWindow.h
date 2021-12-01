#pragma once
#include "WickedEngine.h"

class EditorComponent;

class MaterialWindow;

class HairParticleWindow : public wi::widget::Window
{
public:
	void Create(EditorComponent* editor);

	wi::ecs::Entity entity;
	void SetEntity(wi::ecs::Entity entity);

	void UpdateData();

	wi::HairParticleSystem* GetHair();

	wi::widget::Button addButton;
	wi::widget::ComboBox	meshComboBox;
	wi::widget::Slider lengthSlider;
	wi::widget::Slider stiffnessSlider;
	wi::widget::Slider randomnessSlider;
	wi::widget::Slider countSlider;
	wi::widget::Slider segmentcountSlider;
	wi::widget::Slider randomSeedSlider;
	wi::widget::Slider viewDistanceSlider;
	wi::widget::TextInputField framesXInput;
	wi::widget::TextInputField framesYInput;
	wi::widget::TextInputField frameCountInput;
	wi::widget::TextInputField frameStartInput;

};

