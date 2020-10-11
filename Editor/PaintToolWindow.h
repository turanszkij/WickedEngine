#pragma once
#include "WickedEngine.h"

class EditorComponent;

class PaintToolWindow : public wiWindow
{
	float rot = 0;
	float stroke_dist = 0;
	bool history_needs_recording_start = false;
	bool history_needs_recording_end = false;
	size_t history_textureIndex = 0;
	std::vector<std::shared_ptr<wiResource>> history_textures; // we'd like to keep history tetures in GPU memory to avoid GPU readback
	std::shared_ptr<wiResource> GetEditTextureSlot(const wiScene::MaterialComponent& material, int* uvset = nullptr);
	void ReplaceEditTextureSlot(wiScene::MaterialComponent& material, std::shared_ptr<wiResource> resource);
public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wiECS::Entity entity = wiECS::INVALID_ENTITY;
	int subset = -1;

	wiComboBox modeComboBox;
	wiLabel infoLabel;
	wiSlider radiusSlider;
	wiSlider amountSlider;
	wiSlider falloffSlider;
	wiSlider spacingSlider;
	wiCheckBox backfaceCheckBox;
	wiCheckBox wireCheckBox;
	wiColorPicker colorPicker;
	wiComboBox textureSlotComboBox;
	wiButton saveTextureButton;

	void Update(float dt);
	void DrawBrush() const;

	XMFLOAT2 pos = XMFLOAT2(0, 0);

	enum MODE
	{
		MODE_DISABLED,
		MODE_TEXTURE,
		MODE_VERTEXCOLOR,
		MODE_SCULPTING_ADD,
		MODE_SCULPTING_SUBTRACT,
		MODE_SOFTBODY_PINNING,
		MODE_SOFTBODY_PHYSICS,
		MODE_HAIRPARTICLE_ADD_TRIANGLE,
		MODE_HAIRPARTICLE_REMOVE_TRIANGLE,
		MODE_HAIRPARTICLE_LENGTH,
		MODE_WIND,
	};
	MODE GetMode() const;
	void SetEntity(wiECS::Entity value, int subsetindex = -1);

	wiArchive* currentHistory = nullptr;
	void RecordHistory(bool start, wiGraphics::CommandList cmd = ~0);
	void ConsumeHistoryOperation(wiArchive& archive, bool undo);
};
