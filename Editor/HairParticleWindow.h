#pragma once
#include "SpriteRectWindow.h"

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
	wi::gui::CheckBox cameraBendCheckbox;
	wi::gui::Slider lengthSlider;
	wi::gui::Slider widthSlider;
	wi::gui::Slider stiffnessSlider;
	wi::gui::Slider dragSlider;
	wi::gui::Slider gravityPowerSlider;
	wi::gui::Slider randomnessSlider;
	wi::gui::Slider countSlider;
	wi::gui::Slider segmentcountSlider;
	wi::gui::Slider billboardcountSlider;
	wi::gui::Slider randomSeedSlider;
	wi::gui::Slider viewDistanceSlider;
	wi::gui::Slider uniformitySlider;

	wi::gui::Button addSpriteButton;
  std::deque<wi::gui::Button> sprites;
	std::deque<wi::gui::Button> spriteRemoveButtons;
	std::deque<wi::gui::Slider> spriteSizeSliders;

	SpriteRectWindow spriterectwnd;

	void RefreshSprites();

	void ResizeLayout() override;
};

