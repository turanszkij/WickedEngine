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
#include "MetadataWindow.h"
#include "ConstraintWindow.h"
#include "SplineWindow.h"

class EditorComponent;

class ComponentsWindow : public wi::gui::Window
{
public:
	void Create(EditorComponent* editor);
	void UpdateData(float dt);

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
	MetadataWindow metadataWnd;
	ConstraintWindow constraintWnd;
	SplineWindow splineWnd;

	enum class Filter : uint64_t
	{
		Transform = 1ull << 0ull,
		Material = 1ull << 1ull,
		Mesh = 1ull << 2ull,
		Object = 1ull << 3ull,
		EnvironmentProbe = 1ull << 4ull,
		Decal = 1ull << 5ull,
		Sound = 1ull << 6ull,
		Weather = 1ull << 7ull,
		Light = 1ull << 8ull,
		Animation = 1ull << 9ull,
		Force = 1ull << 10ull,
		Emitter = 1ull << 11ull,
		Hairparticle = 1ull << 12ull,
		IK = 1ull << 13ull,
		Camera = 1ull << 14ull,
		Armature = 1ull << 15ull,
		Collider = 1ull << 16ull,
		Script = 1ull << 17ull,
		Expression = 1ull << 18ull,
		Terrain = 1ull << 19ull,
		Spring = 1ull << 20ull,
		Humanoid = 1ull << 21ull,
		Video = 1ull << 22ull,
		Sprite = 1ull << 23ull,
		Font = 1ull << 24ull,
		VoxelGrid = 1ull << 25ull,
		RigidBody = 1ull << 26ull,
		SoftBody = 1ull << 27ull,
		Metadata = 1ull << 28ull,
		Vehicle = 1ull << 29ull,
		Constraint = 1ull << 30ull,
		Spline = 1ull << 31ull,

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
