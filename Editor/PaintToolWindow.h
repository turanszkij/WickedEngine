#pragma once
#include "WickedEngine.h"

class EditorComponent;

class PaintToolWindow : public wi::gui::Window
{
	float rot = 0;
	float stroke_dist = 0;
	bool history_needs_recording_start = false;
	bool history_needs_recording_end = false;
	size_t history_textureIndex = 0;
	wi::vector<wi::graphics::Texture> history_textures; // we'd like to keep history textures in GPU memory to avoid GPU readback
	wi::graphics::Texture GetEditTextureSlot(const wi::scene::MaterialComponent& material, int* uvset = nullptr);
	void ReplaceEditTextureSlot(wi::scene::MaterialComponent& material, const wi::graphics::Texture& texture);

	struct SculptingIndex
	{
		size_t ind;
		float affection;
	};
	wi::vector<SculptingIndex> sculpting_indices;
	XMFLOAT3 sculpting_normal = XMFLOAT3(0, 0, 0);

	wi::Resource brushTex;
	wi::Resource revealTex;

public:
	void Create(EditorComponent* editor);

	EditorComponent* editor = nullptr;
	wi::ecs::Entity entity = wi::ecs::INVALID_ENTITY;
	int subset = -1;

	wi::gui::ComboBox modeComboBox;
	wi::gui::Label infoLabel;
	wi::gui::Slider radiusSlider;
	wi::gui::Slider amountSlider;
	wi::gui::Slider falloffSlider;
	wi::gui::Slider spacingSlider;
	wi::gui::Slider rotationSlider;
	wi::gui::CheckBox backfaceCheckBox;
	wi::gui::CheckBox wireCheckBox;
	wi::gui::CheckBox pressureCheckBox;
	wi::gui::ColorPicker colorPicker;
	wi::gui::ComboBox textureSlotComboBox;
	wi::gui::ComboBox brushShapeComboBox;
	wi::gui::Button saveTextureButton;
	wi::gui::Button brushTextureButton;
	wi::gui::Button revealTextureButton;
	wi::gui::ComboBox axisCombo;

	void Update(float dt);
	void DrawBrush(const wi::Canvas& canvas, wi::graphics::CommandList cmd) const;

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
	void SetEntity(wi::ecs::Entity value, int subsetindex = -1);

	enum class AxisLock
	{
		Disabled,
		X,
		Y,
		Z
	};

	wi::Archive* currentHistory = nullptr;
	void RecordHistory(bool start, wi::graphics::CommandList cmd = wi::graphics::CommandList());
	void ConsumeHistoryOperation(wi::Archive& archive, bool undo);

	void ResizeLayout() override;
};
