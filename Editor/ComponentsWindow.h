#pragma once
#include "WickedEngine.h"
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
#include "SpringWindow.h"
#include "IKWindow.h"
#include "TransformWindow.h"
#include "LayerWindow.h"
#include "NameWindow.h"

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
	SpringWindow springWnd;
	IKWindow ikWnd;
	TransformWindow transformWnd;
	LayerWindow layerWnd;
	NameWindow nameWnd;
};
