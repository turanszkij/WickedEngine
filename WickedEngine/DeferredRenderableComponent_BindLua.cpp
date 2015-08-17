#include "DeferredRenderableComponent_BindLua.h"

const char DeferredRenderableComponent_BindLua::className[] = "DeferredRenderableComponent";

Luna<DeferredRenderableComponent_BindLua>::FunctionType DeferredRenderableComponent_BindLua::methods[] = {
	lunamethod(DeferredRenderableComponent_BindLua, GetContent),
	lunamethod(DeferredRenderableComponent_BindLua, Initialize),
	lunamethod(DeferredRenderableComponent_BindLua, Load),
	lunamethod(DeferredRenderableComponent_BindLua, Unload),
	lunamethod(DeferredRenderableComponent_BindLua, Start),
	lunamethod(DeferredRenderableComponent_BindLua, Stop),
	lunamethod(DeferredRenderableComponent_BindLua, Update),
	lunamethod(DeferredRenderableComponent_BindLua, Render),
	lunamethod(DeferredRenderableComponent_BindLua, Compose),
	{ NULL, NULL }
};
Luna<DeferredRenderableComponent_BindLua>::PropertyType DeferredRenderableComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

DeferredRenderableComponent_BindLua::DeferredRenderableComponent_BindLua(DeferredRenderableComponent* component)
{
	this->component = component;
}

DeferredRenderableComponent_BindLua::DeferredRenderableComponent_BindLua(lua_State *L)
{
	component = new DeferredRenderableComponent();
}


DeferredRenderableComponent_BindLua::~DeferredRenderableComponent_BindLua()
{
}

void DeferredRenderableComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<DeferredRenderableComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
