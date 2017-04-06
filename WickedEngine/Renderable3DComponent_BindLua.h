#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "Renderable3DComponent.h"
#include "Renderable2DComponent_BindLua.h"

class Renderable3DComponent_BindLua : public Renderable2DComponent_BindLua
{
public:
	static const char className[];
	static Luna<Renderable3DComponent_BindLua>::FunctionType methods[];
	static Luna<Renderable3DComponent_BindLua>::PropertyType properties[];

	Renderable3DComponent_BindLua(Renderable3DComponent* component = nullptr);
	Renderable3DComponent_BindLua(lua_State* L);
	~Renderable3DComponent_BindLua();

	int SetSSAOEnabled(lua_State* L);
	int SetSSREnabled(lua_State* L);
	int SetShadowsEnabled(lua_State* L);
	int SetReflectionsEnabled(lua_State* L);
	int SetFXAAEnabled(lua_State* L);
	int SetBloomEnabled(lua_State* L);
	int SetColorGradingEnabled(lua_State* L);
	int SetEmitterParticlesEnabled(lua_State* L);
	int SetHairParticlesEnabled(lua_State* L);
	int SetHairParticlesReflectionEnabled(lua_State* L);
	int SetVolumeLightsEnabled(lua_State* L);
	int SetLightShaftsEnabled(lua_State* L);
	int SetLensFlareEnabled(lua_State* L);
	int SetMotionBlurEnabled(lua_State* L);
	int SetSSSEnabled(lua_State* L);
	int SetDepthOfFieldEnabled(lua_State* L);
	int SetStereogramEnabled(lua_State* L);
	int SetEyeAdaptionEnabled(lua_State* L);
	int SetTessellationEnabled(lua_State* L);
	int SetMSAASampleCount(lua_State* L);
	int SetHairParticleAlphaCompositionEnabled(lua_State* L);
	int SetSharpenFilterEnabled(lua_State* L);
	int SetSharpenFilterAmount(lua_State* L);

	int SetDepthOfFieldFocus(lua_State* L);
	int SetDepthOfFieldStrength(lua_State* L);

	int SetPreferredThreadingCount(lua_State* L);

	static void Bind();
};

