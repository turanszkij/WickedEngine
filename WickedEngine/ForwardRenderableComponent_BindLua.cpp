#include "ForwardRenderableComponent_BindLua.h"

const char ForwardRenderableComponent_BindLua::className[] = "ForwardRenderableComponent";

Luna<ForwardRenderableComponent_BindLua>::FunctionType ForwardRenderableComponent_BindLua::methods[] = {
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

	lunamethod(RenderableComponent_BindLua, GetContent),
	lunamethod(RenderableComponent_BindLua, Initialize),
	lunamethod(RenderableComponent_BindLua, Load),
	lunamethod(RenderableComponent_BindLua, Unload),
	lunamethod(RenderableComponent_BindLua, Start),
	lunamethod(RenderableComponent_BindLua, Stop),
	lunamethod(RenderableComponent_BindLua, FixedUpdate),
	lunamethod(RenderableComponent_BindLua, Update),
	lunamethod(RenderableComponent_BindLua, Render),
	lunamethod(RenderableComponent_BindLua, Compose),
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
Luna<ForwardRenderableComponent_BindLua>::PropertyType ForwardRenderableComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

ForwardRenderableComponent_BindLua::ForwardRenderableComponent_BindLua(ForwardRenderableComponent* component)
{
	this->component = component;
}

ForwardRenderableComponent_BindLua::ForwardRenderableComponent_BindLua(lua_State *L)
{
	component = new ForwardRenderableComponent();
}


ForwardRenderableComponent_BindLua::~ForwardRenderableComponent_BindLua()
{
}

void ForwardRenderableComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		//Luna<ForwardRenderableComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
