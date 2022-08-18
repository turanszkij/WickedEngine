#pragma once
#include "WickedEngine.h"
#include "GraphicsWindow.h"
#include "CameraWindow.h"
#include "MaterialPickerWindow.h"
#include "PaintToolWindow.h"
#include "TerrainGenerator.h"

class EditorComponent;

class OptionsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);
	void Update(float dt);

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	wi::gui::CheckBox isScalatorCheckBox;
	wi::gui::CheckBox isRotatorCheckBox;
	wi::gui::CheckBox isTranslatorCheckBox;
	wi::gui::CheckBox profilerEnabledCheckBox;
	wi::gui::CheckBox physicsEnabledCheckBox;
	wi::gui::CheckBox cinemaModeCheckBox;
	wi::gui::CheckBox versionCheckBox;
	wi::gui::CheckBox fpsCheckBox;
	wi::gui::CheckBox otherinfoCheckBox;
	wi::gui::ComboBox themeCombo;
	wi::gui::ComboBox saveModeComboBox;
	wi::gui::ComboBox sceneComboBox;
	wi::gui::Slider pathTraceTargetSlider;
	wi::gui::Label pathTraceStatisticsLabel;
	GraphicsWindow graphicsWnd;
	CameraWindow cameraWnd;
	MaterialPickerWindow materialPickerWnd;
	PaintToolWindow paintToolWnd;
	TerrainGenerator terragen;

	enum class Filter : uint64_t
	{
		Transform = 1 << 0,
		Material = 1 << 1,
		Mesh = 1 << 2,
		Object = 1 << 3,
		EnvironmentProbe = 1 << 4,
		Decal = 1 << 5,
		Sound = 1 << 6,
		Weather = 1 << 7,
		Light = 1 << 8,
		Animation = 1 << 9,
		Force = 1 << 10,
		Emitter = 1 << 11,
		Hairparticle = 1 << 12,
		IK = 1 << 13,
		Camera = 1 << 14,
		Armature = 1 << 15,

		All = ~0ull,
	} filter = Filter::All;
	wi::gui::ComboBox newCombo;
	wi::gui::ComboBox filterCombo;
	wi::gui::TreeList entityTree;
	wi::unordered_set<wi::ecs::Entity> entitytree_added_items;
	wi::unordered_set<wi::ecs::Entity> entitytree_opened_items;
	void PushToEntityTree(wi::ecs::Entity entity, int level);
	void RefreshEntityTree();
};

template<>
struct enable_bitmask_operators<OptionsWindow::Filter> {
	static const bool enable = true;
};
