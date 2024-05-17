#pragma once
#include "MaterialWindow.h"
#include "WeatherWindow.h"
#include "ObjectWindow.h"
#include "MeshWindow.h"
#include "EnvProbeWindow.h"
#include "DecalWindow.h"
#include "LightWindow.h"
#include "AnimationWindow.h"
#include "EmitterWindow.h"
#include "HairParticleWindow.h"
#include "ForceFieldWindow.h"
#include "SoundWindow.h"
#include "VideoWindow.h"
#include "SpringWindow.h"
#include "IKWindow.h"
#include "TransformWindow.h"
#include "LayerWindow.h"
#include "NameWindow.h"
#include "ScriptWindow.h"
#include "RigidBodyWindow.h"
#include "SoftBodyWindow.h"
#include "ColliderWindow.h"
#include "HierarchyWindow.h"
#include "CameraComponentWindow.h"
#include "ExpressionWindow.h"
#include "ArmatureWindow.h"
#include "HumanoidWindow.h"
#include "TerrainWindow.h"
#include "SpriteWindow.h"
#include "FontWindow.h"
#include "VoxelGridWindow.h"

class EditorComponent;

class ComponentsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);
	void Update(float dt);

	void ResizeLayout() override;

	EditorComponent* editor = nullptr;
	wi::gui::ComboBox newComponentCombo;
	MaterialWindow materialWnd;
	WeatherWindow weatherWnd;
	ObjectWindow objectWnd;
	MeshWindow meshWnd;
	EnvProbeWindow envProbeWnd;
	DecalWindow decalWnd;
	LightWindow lightWnd;
	AnimationWindow animWnd;
	EmitterWindow emitterWnd;
	HairParticleWindow hairWnd;
	ForceFieldWindow forceFieldWnd;
	SoundWindow soundWnd;
	VideoWindow videoWnd;
	SpringWindow springWnd;
	IKWindow ikWnd;
	TransformWindow transformWnd;
	LayerWindow layerWnd;
	NameWindow nameWnd;
	ScriptWindow scriptWnd;
	RigidBodyWindow rigidWnd;
	SoftBodyWindow softWnd;
	ColliderWindow colliderWnd;
	HierarchyWindow hierarchyWnd;
	CameraComponentWindow cameraComponentWnd;
	ExpressionWindow expressionWnd;
	ArmatureWindow armatureWnd;
	HumanoidWindow humanoidWnd;
	TerrainWindow terrainWnd;
	SpriteWindow spriteWnd;
	FontWindow fontWnd;
	VoxelGridWindow voxelGridWnd;

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
		VoxelGrid = 1 << 25,
		RigidBody = 1 << 26,
		SoftBody = 1 << 27,

		All = ~0ull,
	} filter = Filter::All;
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
struct enable_bitmask_operators<ComponentsWindow::Filter> {
	static const bool enable = true;
};
