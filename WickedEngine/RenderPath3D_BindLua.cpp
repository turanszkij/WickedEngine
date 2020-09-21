#include "RenderPath3D_BindLua.h"
#include "Texture_BindLua.h"

const char RenderPath3D_BindLua::className[] = "RenderPath3D";

Luna<RenderPath3D_BindLua>::FunctionType RenderPath3D_BindLua::methods[] = {
	lunamethod(RenderPath2D_BindLua, AddSprite),
	lunamethod(RenderPath2D_BindLua, AddFont),
	lunamethod(RenderPath2D_BindLua, RemoveSprite),
	lunamethod(RenderPath2D_BindLua, RemoveFont),
	lunamethod(RenderPath2D_BindLua, ClearSprites),
	lunamethod(RenderPath2D_BindLua, ClearFonts),
	lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
	lunamethod(RenderPath2D_BindLua, GetFontOrder),

	lunamethod(RenderPath2D_BindLua, AddLayer),
	lunamethod(RenderPath2D_BindLua, GetLayers),
	lunamethod(RenderPath2D_BindLua, SetLayerOrder),
	lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
	lunamethod(RenderPath2D_BindLua, SetFontOrder),

	lunamethod(RenderPath_BindLua, GetLayerMask),
	lunamethod(RenderPath_BindLua, SetLayerMask),

	lunamethod(RenderPath3D_BindLua, SetAO),
	lunamethod(RenderPath3D_BindLua, SetSSREnabled),
	lunamethod(RenderPath3D_BindLua, SetRaytracedReflectionsEnabled),
	lunamethod(RenderPath3D_BindLua, SetShadowsEnabled),
	lunamethod(RenderPath3D_BindLua, SetReflectionsEnabled),
	lunamethod(RenderPath3D_BindLua, SetFXAAEnabled),
	lunamethod(RenderPath3D_BindLua, SetBloomEnabled),
	lunamethod(RenderPath3D_BindLua, SetBloomThreshold),
	lunamethod(RenderPath3D_BindLua, SetColorGradingEnabled),
	lunamethod(RenderPath3D_BindLua, SetVolumeLightsEnabled),
	lunamethod(RenderPath3D_BindLua, SetLightShaftsEnabled),
	lunamethod(RenderPath3D_BindLua, SetLensFlareEnabled),
	lunamethod(RenderPath3D_BindLua, SetMotionBlurEnabled),
	lunamethod(RenderPath3D_BindLua, SetSSSEnabled),
	lunamethod(RenderPath3D_BindLua, SetDitherEnabled),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldEnabled),
	lunamethod(RenderPath3D_BindLua, SetEyeAdaptionEnabled),
	lunamethod(RenderPath3D_BindLua, SetMSAASampleCount),
	lunamethod(RenderPath3D_BindLua, SetSharpenFilterEnabled),
	lunamethod(RenderPath3D_BindLua, SetSharpenFilterAmount),
	lunamethod(RenderPath3D_BindLua, SetExposure),
	lunamethod(RenderPath3D_BindLua, SetMotionBlurStrength),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldFocus),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldStrength),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldAspect),
	{ NULL, NULL }
};
Luna<RenderPath3D_BindLua>::PropertyType RenderPath3D_BindLua::properties[] = {
	{ NULL, NULL }
};


int RenderPath3D_BindLua::SetAO(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetAO(AO value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		int value = wiLua::SGetInt(L, 1);
		RenderPath3D::AO ao = (RenderPath3D::AO)value;
		((RenderPath3D*)component)->setAO(ao);
	}
	else
		wiLua::SError(L, "SetAO(AO value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetSSREnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSSREnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setSSREnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetSSREnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetRaytracedReflectionsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetRaytracedReflectionsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setRaytracedReflectionsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetRaytracedReflectionsEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetShadowsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetShadowsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setShadowsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetReflectionsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetShadowsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setReflectionsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetFXAAEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFXAAEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setFXAAEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetFXAAEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetBloomEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetBloomEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setBloomEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetBloomEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetBloomThreshold(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetBloomThreshold(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setBloomThreshold(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetBloomThreshold(float value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetColorGradingEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetColorGradingEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setColorGradingEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetColorGradingEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetVolumeLightsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetVolumeLightsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setVolumeLightsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetVolumeLightsEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetLightShaftsEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLightShaftsEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setLightShaftsEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetLightShaftsEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetLensFlareEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLensFlareEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setLensFlareEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetLensFlareEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetMotionBlurEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetMotionBlurEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setMotionBlurEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetMotionBlurEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetSSSEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSSSEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setSSSEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetSSSEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetDitherEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDitherEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
		((RenderPath3D*)component)->setDitherEnabled(wiLua::SGetBool(L, 1));
	else
		wiLua::SError(L, "SetDitherEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetDepthOfFieldEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setDepthOfFieldEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetEyeAdaptionEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetEyeAdaptionEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setEyeAdaptionEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetEyeAdaptionEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetMSAASampleCount(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetMSAASampleCount(int value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setMSAASampleCount((uint32_t)wiLua::SGetInt(L, 1));
	}
	else
		wiLua::SError(L, "SetMSAASampleCount(int value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetSharpenFilterEnabled(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSharpenFilterEnabled(bool value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setSharpenFilterEnabled(wiLua::SGetBool(L, 1));
	}
	else
		wiLua::SError(L, "SetSharpenFilterEnabled(bool value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetSharpenFilterAmount(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSharpenFilterAmount(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setSharpenFilterAmount(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetSharpenFilterAmount(float value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetExposure(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetExposure(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setExposure(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetExposure(float value) not enough arguments!");
	return 0;
}


int RenderPath3D_BindLua::SetMotionBlurStrength(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetMotionBlurStrength(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setMotionBlurStrength(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetMotionBlurStrength(float value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetDepthOfFieldFocus(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldFocus(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setDepthOfFieldFocus(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldFocus(float value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetDepthOfFieldStrength(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldStrength(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setDepthOfFieldStrength(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldStrength(float value) not enough arguments!");
	return 0;
}
int RenderPath3D_BindLua::SetDepthOfFieldAspect(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetDepthOfFieldAspect(float value) component is null!");
		return 0;
	}
	if (wiLua::SGetArgCount(L) > 0)
	{
		((RenderPath3D*)component)->setDepthOfFieldAspect(wiLua::SGetFloat(L, 1));
	}
	else
		wiLua::SError(L, "SetDepthOfFieldAspect(float value) not enough arguments!");
	return 0;
}

void RenderPath3D_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<RenderPath3D_BindLua>::Register(wiLua::GetLuaState());

		wiLua::RunText("AO_DISABLED = 0");
		wiLua::RunText("AO_SSAO = 1");
		wiLua::RunText("AO_HBAO = 2");
		wiLua::RunText("AO_MSAO = 3");
		wiLua::RunText("AO_RTAO = 4");
	}
}
