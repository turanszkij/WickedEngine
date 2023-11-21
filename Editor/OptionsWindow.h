#pragma once
#include "GraphicsWindow.h"
#include "CameraWindow.h"
#include "MaterialPickerWindow.h"
#include "PaintToolWindow.h"
#include "GeneralWindow.h"

class EditorComponent;

class OptionsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);
	void Update(float dt);

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	GeneralWindow generalWnd;
	GraphicsWindow graphicsWnd;
	CameraWindow cameraWnd;
	MaterialPickerWindow materialPickerWnd;
	PaintToolWindow paintToolWnd;

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
		Collider = 1 << 16,
		Script = 1 << 17,
		Expression = 1 << 18,
		Terrain = 1 << 19,
		Spring = 1 << 20,
		Humanoid = 1 << 21,
		Video = 1 << 22,
		Sprite = 1 << 23,
		Font = 1 << 24,

		All = ~0ull,
	} filter = Filter::All;
	wi::gui::ComboBox newCombo;
	wi::gui::ComboBox filterCombo;
	wi::gui::TextInputField filterInput;
	wi::gui::CheckBox filterCaseCheckBox;
	wi::gui::TreeList entityTree;
	wi::unordered_set<wi::ecs::Entity> entitytree_temp_items;
	wi::unordered_set<wi::ecs::Entity> entitytree_added_items;
	wi::unordered_set<wi::ecs::Entity> entitytree_opened_items;
	void PushToEntityTree(wi::ecs::Entity entity, int level);
	void RefreshEntityTree();

	bool CheckEntityFilter(wi::ecs::Entity entity);
};

template<>
struct enable_bitmask_operators<OptionsWindow::Filter> {
	static const bool enable = true;
};
