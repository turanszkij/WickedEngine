#include "RenderPath3D_TiledDeferred_BindLua.h"

const char RenderPath3D_TiledDeferred_BindLua::className[] = "RenderPath3D_TiledDeferred";

Luna<RenderPath3D_TiledDeferred_BindLua>::FunctionType RenderPath3D_TiledDeferred_BindLua::methods[] = {
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

	lunamethod(RenderPath_BindLua, GetContent),
	lunamethod(RenderPath_BindLua, Initialize),
	lunamethod(RenderPath_BindLua, Load),
	lunamethod(RenderPath_BindLua, Unload),
	lunamethod(RenderPath_BindLua, Start),
	lunamethod(RenderPath_BindLua, Stop),
	lunamethod(RenderPath_BindLua, FixedUpdate),
	lunamethod(RenderPath_BindLua, Update),
	lunamethod(RenderPath_BindLua, Render),
	lunamethod(RenderPath_BindLua, Compose),
	lunamethod(RenderPath_BindLua, OnStart),
	lunamethod(RenderPath_BindLua, OnStop),
	lunamethod(RenderPath_BindLua, GetLayerMask),
	lunamethod(RenderPath_BindLua, SetLayerMask),

	lunamethod(RenderPath3D_BindLua, SetSSAOEnabled),
	lunamethod(RenderPath3D_BindLua, SetSSREnabled),
	lunamethod(RenderPath3D_BindLua, SetShadowsEnabled),
	lunamethod(RenderPath3D_BindLua, SetReflectionsEnabled),
	lunamethod(RenderPath3D_BindLua, SetFXAAEnabled),
	lunamethod(RenderPath3D_BindLua, SetBloomEnabled),
	lunamethod(RenderPath3D_BindLua, SetColorGradingEnabled),
	lunamethod(RenderPath3D_BindLua, SetColorGradingTexture),
	lunamethod(RenderPath3D_BindLua, SetEmitterParticlesEnabled),
	lunamethod(RenderPath3D_BindLua, SetHairParticlesEnabled),
	lunamethod(RenderPath3D_BindLua, SetHairParticlesReflectionEnabled),
	lunamethod(RenderPath3D_BindLua, SetVolumeLightsEnabled),
	lunamethod(RenderPath3D_BindLua, SetLightShaftsEnabled),
	lunamethod(RenderPath3D_BindLua, SetLensFlareEnabled),
	lunamethod(RenderPath3D_BindLua, SetMotionBlurEnabled),
	lunamethod(RenderPath3D_BindLua, SetSSSEnabled),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldEnabled),
	lunamethod(RenderPath3D_BindLua, SetStereogramEnabled),
	lunamethod(RenderPath3D_BindLua, SetEyeAdaptionEnabled),
	lunamethod(RenderPath3D_BindLua, SetTessellationEnabled),
	lunamethod(RenderPath3D_BindLua, SetSharpenFilterEnabled),
	lunamethod(RenderPath3D_BindLua, SetSharpenFilterAmount),

	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldFocus),
	lunamethod(RenderPath3D_BindLua, SetDepthOfFieldStrength),

	{ NULL, NULL }
};
Luna<RenderPath3D_TiledDeferred_BindLua>::PropertyType RenderPath3D_TiledDeferred_BindLua::properties[] = {
	{ NULL, NULL }
};

RenderPath3D_TiledDeferred_BindLua::RenderPath3D_TiledDeferred_BindLua(RenderPath3D_TiledDeferred* component)
{
	this->component = component;
}

RenderPath3D_TiledDeferred_BindLua::RenderPath3D_TiledDeferred_BindLua(lua_State *L)
{
	component = new RenderPath3D_TiledDeferred();
}


RenderPath3D_TiledDeferred_BindLua::~RenderPath3D_TiledDeferred_BindLua()
{
}

void RenderPath3D_TiledDeferred_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		//Luna<RenderPath3D_TiledDeferred_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
