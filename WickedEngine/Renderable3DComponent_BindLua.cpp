#include "Renderable3DComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"

const char Renderable3DComponent_BindLua::className[] = "Renderable3DComponent";

Luna<Renderable3DComponent_BindLua>::FunctionType Renderable3DComponent_BindLua::methods[] = {
	lunamethod(Renderable2DComponent_BindLua, AddSprite),
	lunamethod(Renderable2DComponent_BindLua, AddFont),
	lunamethod(Renderable2DComponent_BindLua, RemoveSprite),
	lunamethod(Renderable2DComponent_BindLua, RemoveFont),
	lunamethod(Renderable2DComponent_BindLua, ClearSprites),
	lunamethod(Renderable2DComponent_BindLua, ClearFonts),
	lunamethod(Renderable2DComponent_BindLua, GetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, GetFontOrder),

	lunamethod(Renderable2DComponent_BindLua, AddLayer),
	lunamethod(Renderable2DComponent_BindLua, GetLayers),
	lunamethod(Renderable2DComponent_BindLua, SetLayerOrder),
	lunamethod(Renderable2DComponent_BindLua, SetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, SetFontOrder),

	lunamethod(Renderable3DComponent_BindLua, GetContent),
	lunamethod(Renderable3DComponent_BindLua, Initialize),
	lunamethod(Renderable3DComponent_BindLua, Load),
	lunamethod(Renderable3DComponent_BindLua, Unload),
	lunamethod(Renderable3DComponent_BindLua, Start),
	lunamethod(Renderable3DComponent_BindLua, Stop),
	lunamethod(Renderable3DComponent_BindLua, FixedUpdate),
	lunamethod(Renderable3DComponent_BindLua, Update),
	lunamethod(Renderable3DComponent_BindLua, Render),
	lunamethod(Renderable3DComponent_BindLua, Compose),
	lunamethod(RenderableComponent_BindLua, OnStart),
	lunamethod(RenderableComponent_BindLua, OnStop),
	lunamethod(RenderableComponent_BindLua, GetLayerMask),
	lunamethod(RenderableComponent_BindLua, SetLayerMask),

	lunamethod(Renderable3DComponent_BindLua, SetSSAOEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetSSREnabled),
	lunamethod(Renderable3DComponent_BindLua, SetShadowsEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetReflectionsEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetFXAAEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetBloomEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetColorGradingEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetEmitterParticlesEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetHairParticlesEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetHairParticlesReflectionEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetVolumeLightsEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetLightShaftsEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetLensFlareEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetMotionBlurEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetSSSEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetDepthOfFieldEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetStereogramEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetEyeAdaptionEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetTessellationEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetMSAASampleCount),
	lunamethod(Renderable3DComponent_BindLua, SetSharpenFilterEnabled),
	lunamethod(Renderable3DComponent_BindLua, SetSharpenFilterAmount),

	lunamethod(Renderable3DComponent_BindLua, SetDepthOfFieldFocus),
	lunamethod(Renderable3DComponent_BindLua, SetDepthOfFieldStrength),

	lunamethod(Renderable3DComponent_BindLua, SetPreferredThreadingCount),
	{ NULL, NULL }
};
Luna<Renderable3DComponent_BindLua>::PropertyType Renderable3DComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

Renderable3DComponent_BindLua::Renderable3DComponent_BindLua(Renderable3DComponent* component)
{
	this->component = component;
}

Renderable3DComponent_BindLua::Renderable3DComponent_BindLua(lua_State* L)
{
	component = nullptr;
}

Renderable3DComponent_BindLua::~Renderable3DComponent_BindLua()
{
}


int Renderable3DComponent_BindLua::SetSSAOEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSSAOEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setSSAOEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetSSAOEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetSSREnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSSREnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setSSREnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetSSREnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetShadowsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetShadowsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setShadowsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetReflectionsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetShadowsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setReflectionsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetFXAAEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFXAAEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setFXAAEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetFXAAEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetBloomEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetBloomEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setBloomEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetBloomEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetColorGradingEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetColorGradingEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setColorGradingEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetColorGradingEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetEmitterParticlesEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetEmitterParticlesEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setEmitterParticlesEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetEmitterParticlesEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetHairParticlesEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetHairParticlesEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setHairParticlesEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetHairParticlesEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetHairParticlesReflectionEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetHairParticlesReflectionEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setHairParticlesReflectionEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetHairParticlesReflectionEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetVolumeLightsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetVolumeLightsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setVolumeLightsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetVolumeLightsEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetLightShaftsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLightShaftsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setLightShaftsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetLightShaftsEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetLensFlareEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLensFlareEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setLensFlareEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetLensFlareEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetMotionBlurEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetMotionBlurEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setMotionBlurEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetMotionBlurEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetSSSEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSSSEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((Renderable3DComponent*)component)->setSSSEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetSSSEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetDepthOfFieldEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setDepthOfFieldEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetStereogramEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetStereogramEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setStereogramEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetStereogramEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetEyeAdaptionEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetEyeAdaptionEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setEyeAdaptionEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetEyeAdaptionEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetTessellationEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetTessellationEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setTessellationEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetTessellationEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetMSAASampleCount(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetMSAASampleCount(int value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setMSAASampleCount((UINT)wiLua::SGetInt(L, 1));
	}
	else
		wiLua::SError(L, "SetMSAASampleCount(int value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetSharpenFilterEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSharpenFilterEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setSharpenFilterEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetSharpenFilterEnabled(bool value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetSharpenFilterAmount(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSharpenFilterAmount(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setSharpenFilterAmount(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetSharpenFilterAmount(bool value) not enough arguments!");
	return 0;
}


int Renderable3DComponent_BindLua::SetDepthOfFieldFocus(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldFocus(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setDepthOfFieldFocus(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldFocus(float value) not enough arguments!");
	return 0;
}
int Renderable3DComponent_BindLua::SetDepthOfFieldStrength(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldStrength(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setDepthOfFieldStrength(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldStrength(float value) not enough arguments!");
	return 0;
}

int Renderable3DComponent_BindLua::SetPreferredThreadingCount(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetPreferredThreadingCount(int value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((Renderable3DComponent*)component)->setPreferredThreadingCount((unsigned short)wiLua::SGetInt(L, 1));
	}
	else
		wiLua::SError(L, "SetPreferredThreadingCount(int value) not enough arguments!");
	return 0;
}

void Renderable3DComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Renderable3DComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
