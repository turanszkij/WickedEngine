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
};
