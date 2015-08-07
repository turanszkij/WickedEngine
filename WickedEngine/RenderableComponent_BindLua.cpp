#include "RenderableComponent_BindLua.h"

const char RenderableComponent_BindLua::className[] = "RenderableComponent";

Luna<RenderableComponent_BindLua>::FunctionType RenderableComponent_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<RenderableComponent_BindLua>::PropertyType RenderableComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

RenderableComponent_BindLua::RenderableComponent_BindLua(RenderableComponent* component) :component(component)
{
}

RenderableComponent_BindLua::RenderableComponent_BindLua(lua_State *L)
{
	component = nullptr;
}


RenderableComponent_BindLua::~RenderableComponent_BindLua()
{
}

void RenderableComponent_BindLua::Bind()
{

	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<RenderableComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

