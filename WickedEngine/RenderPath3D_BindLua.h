#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "RenderPath3D.h"
#include "RenderPath2D_BindLua.h"

class RenderPath3D_BindLua : public RenderPath2D_BindLua
{
public:
	static const char className[];
	static Luna<RenderPath3D_BindLua>::FunctionType methods[];
	static Luna<RenderPath3D_BindLua>::PropertyType properties[];

	RenderPath3D_BindLua() = default;
	RenderPath3D_BindLua(RenderPath3D* component)
	{
		this->component = component;
	}
	RenderPath3D_BindLua(lua_State* L) {}

	int SetAO(lua_State* L);
	int SetSSREnabled(lua_State* L);
	int SetRaytracedReflectionsEnabled(lua_State* L);
	int SetShadowsEnabled(lua_State* L);
	int SetReflectionsEnabled(lua_State* L);
	int SetFXAAEnabled(lua_State* L);
	int SetBloomEnabled(lua_State* L);
	int SetBloomThreshold(lua_State* L);
	int SetColorGradingEnabled(lua_State* L);
	int SetVolumeLightsEnabled(lua_State* L);
	int SetLightShaftsEnabled(lua_State* L);
	int SetLensFlareEnabled(lua_State* L);
	int SetMotionBlurEnabled(lua_State* L);
	int SetSSSEnabled(lua_State* L);
	int SetDitherEnabled(lua_State* L);
	int SetDepthOfFieldEnabled(lua_State* L);
	int SetEyeAdaptionEnabled(lua_State* L);
	int SetMSAASampleCount(lua_State* L);
	int SetSharpenFilterEnabled(lua_State* L);
	int SetSharpenFilterAmount(lua_State* L);
	int SetExposure(lua_State* L);
	int SetMotionBlurStrength(lua_State* L);
	int SetDepthOfFieldFocus(lua_State* L);
	int SetDepthOfFieldStrength(lua_State* L);
	int SetDepthOfFieldAspect(lua_State* L);

	static void Bind();
};

