#include "ForwardRenderableComponent_BindLua.h"

const char ForwardRenderableComponent_BindLua::className[] = "ForwardRenderableComponent";

Luna<ForwardRenderableComponent_BindLua>::FunctionType ForwardRenderableComponent_BindLua::methods[] = {
	lunamethod(ForwardRenderableComponent_BindLua, GetContent),
	lunamethod(ForwardRenderableComponent_BindLua, Initialize),
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
		Luna<ForwardRenderableComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
