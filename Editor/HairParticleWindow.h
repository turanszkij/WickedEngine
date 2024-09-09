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
	wi::gui::Slider lengthSlider;
	wi::gui::Slider widthSlider;
	wi::gui::Slider stiffnessSlider;
	wi::gui::Slider randomnessSlider;
	wi::gui::Slider countSlider;
	wi::gui::Slider segmentcountSlider;
	wi::gui::Slider randomSeedSlider;
	wi::gui::Slider viewDistanceSlider;

	wi::gui::Button addSpriteButton;
	wi::vector<wi::gui::Button> sprites;
	wi::vector<wi::gui::Button> spriteRemoveButtons;
	wi::vector<wi::gui::Slider> spriteSizeSliders;

	SpriteRectWindow spriterectwnd;

	void RefreshSprites();

	void ResizeLayout() override;
};

