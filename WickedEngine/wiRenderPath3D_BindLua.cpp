#include "wiRenderPath3D_BindLua.h"
#include "wiTexture_BindLua.h"

namespace wi::lua
{

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

		lunamethod(RenderPath3D_BindLua, SetResolutionScale),
		lunamethod(RenderPath3D_BindLua, SetAO),
		lunamethod(RenderPath3D_BindLua, SetAOPower),
		lunamethod(RenderPath3D_BindLua, SetSSREnabled),
		lunamethod(RenderPath3D_BindLua, SetSSGIEnabled),
		lunamethod(RenderPath3D_BindLua, SetRaytracedDiffuseEnabled),
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
		lunamethod(RenderPath3D_BindLua, SetDitherEnabled),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldEnabled),
		lunamethod(RenderPath3D_BindLua, SetEyeAdaptionEnabled),
		lunamethod(RenderPath3D_BindLua, SetMSAASampleCount),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterEnabled),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterAmount),
		lunamethod(RenderPath3D_BindLua, SetExposure),
		lunamethod(RenderPath3D_BindLua, SetMotionBlurStrength),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldStrength),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsStrength),
		lunamethod(RenderPath3D_BindLua, SetOutlineEnabled),
		lunamethod(RenderPath3D_BindLua, SetOutlineThickness),
		lunamethod(RenderPath3D_BindLua, SetOutlineThreshold),
		lunamethod(RenderPath3D_BindLua, SetOutlineColor),
		lunamethod(RenderPath3D_BindLua, SetFSREnabled),
		lunamethod(RenderPath3D_BindLua, SetFSRSharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Enabled),
		lunamethod(RenderPath3D_BindLua, SetFSR2Sharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Preset),
		lunamethod(RenderPath3D_BindLua, SetTonemap),

		lunamethod(RenderPath3D_BindLua, SetCropLeft),
		lunamethod(RenderPath3D_BindLua, SetCropTop),
		lunamethod(RenderPath3D_BindLua, SetCropRight),
		lunamethod(RenderPath3D_BindLua, SetCropBottom),

		lunamethod(RenderPath3D_BindLua, GetLastPostProcessRT),

		lunamethod(RenderPath2D_BindLua, CopyFrom),
		{ NULL, NULL }
	};
	Luna<RenderPath3D_BindLua>::PropertyType RenderPath3D_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int RenderPath3D_BindLua::SetResolutionScale(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetResolutionScale(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			((RenderPath3D*)component)->resolutionScale = value;
		}
		else
			wi::lua::SError(L, "SetResolutionScale(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAO(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetAO(AO value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			int value = wi::lua::SGetInt(L, 1);
			RenderPath3D::AO ao = (RenderPath3D::AO)value;
			((RenderPath3D*)component)->setAO(ao);
		}
		else
			wi::lua::SError(L, "SetAO(AO value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAOPower(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetAOPower(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			((RenderPath3D*)component)->setAOPower(value);
		}
		else
			wi::lua::SError(L, "SetAOPower(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetSSREnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSREnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetSSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSGIEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetSSGIEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSGIEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetSSGIEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedDiffuseEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetRaytracedDiffuseEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedDiffuseEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetRaytracedDiffuseEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetRaytracedReflectionsEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedReflectionsEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetRaytracedReflectionsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetShadowsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetShadowsEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setShadowsEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetShadowsEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setReflectionsEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFXAAEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFXAAEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFXAAEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetFXAAEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetBloomEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setBloomEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetBloomEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetBloomThreshold(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setBloomThreshold(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetBloomThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetColorGradingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetColorGradingEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setColorGradingEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetColorGradingEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetVolumeLightsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetVolumeLightsEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setVolumeLightsEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetVolumeLightsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetLightShaftsEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLightShaftsEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetLightShaftsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLensFlareEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetLensFlareEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLensFlareEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetLensFlareEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetMotionBlurEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setMotionBlurEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetMotionBlurEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDitherEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetDitherEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setDitherEnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetDitherEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetDepthOfFieldEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetDepthOfFieldEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetEyeAdaptionEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetEyeAdaptionEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setEyeAdaptionEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetEyeAdaptionEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMSAASampleCount(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetMSAASampleCount(int value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMSAASampleCount((uint32_t)wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetMSAASampleCount(int value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetSharpenFilterEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetSharpenFilterEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterAmount(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetSharpenFilterAmount(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterAmount(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetSharpenFilterAmount(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetExposure(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetExposure(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setExposure(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetExposure(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetMotionBlurStrength(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMotionBlurStrength(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetMotionBlurStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetDepthOfFieldStrength(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldStrength(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetDepthOfFieldStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetLightShaftsStrength(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setLightShaftsStrength(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetLightShaftsStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetOutlineEnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineEnabled(wi::lua::SGetBool(L, 1));
		}
		else
			wi::lua::SError(L, "SetOutlineEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThickness(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetOutlineThickness(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThickness(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetOutlineThickness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetOutlineThreshold(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThreshold(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetOutlineThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineColor(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetOutlineColor(float r,g,b,a) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 3)
		{
			((RenderPath3D*)component)->setOutlineColor(wi::lua::SGetFloat4(L, 1));
		}
		else
			wi::lua::SError(L, "SetOutlineColor(float r,g,b,a) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFSREnabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSREnabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetFSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSRSharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFSRSharpness(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSRSharpness(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetFSRSharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Enabled(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFSR2Enabled(bool value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSR2Enabled(wi::lua::SGetBool(L, 1));
		else
			wi::lua::SError(L, "SetFSR2Enabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Sharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFSR2Sharpness(float value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Sharpness(wi::lua::SGetFloat(L, 1));
		}
		else
			wi::lua::SError(L, "SetFSR2Sharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Preset(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetFSR2Preset(FSR2_Preset value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Preset((wi::RenderPath3D::FSR2_Preset)wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetFSR2Preset(FSR2_Preset value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetTonemap(lua_State* L)
	{
		if (component == nullptr)
		{
			wi::lua::SError(L, "SetTonemap(Tonemap value) component is null!");
			return 0;
		}
		if (wi::lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setTonemap((wi::renderer::Tonemap)wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetTonemap(Tonemap value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetCropLeft(lua_State* L)
	{
		((RenderPath3D*)component)->crop_left = wi::lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropTop(lua_State* L)
	{
		((RenderPath3D*)component)->crop_top = wi::lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropRight(lua_State* L)
	{
		((RenderPath3D*)component)->crop_right = wi::lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropBottom(lua_State* L)
	{
		((RenderPath3D*)component)->crop_bottom = wi::lua::SGetFloat(L, 1);
		return 0;
	}

	int RenderPath3D_BindLua::GetLastPostProcessRT(lua_State* L)
	{
		const wi::graphics::Texture* tex = ((RenderPath3D*)component)->GetLastPostprocessRT();
		if (tex == nullptr)
			return 0;
		Luna<Texture_BindLua>::push(L, *tex);
		return 1;
	}

static const std::string value_bindings = R"(
AO_DISABLED = 0
AO_SSAO = 1
AO_HBAO = 2
AO_MSAO = 3
AO_RTAO = 4

FSR2_Preset = {
	Quality = 0,
	Balanced = 1,
	Performance = 2,
	Ultra_Performance = 3,
}

Tonemap = {
	Reinhard = 0,
	ACES = 1,
}
)";

	void RenderPath3D_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<RenderPath3D_BindLua>::Register(wi::lua::GetLuaState());

			wi::lua::RunText(value_bindings);
		}
	}

}
