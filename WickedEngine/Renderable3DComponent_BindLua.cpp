#include "Renderable3DComponent_BindLua.h"

const char Renderable3DComponent_BindLua::className[] = "Renderable3DComponent";

Luna<Renderable3DComponent_BindLua>::FunctionType Renderable3DComponent_BindLua::methods[] = {
	{ NULL, NULL }
};
Luna<Renderable3DComponent_BindLua>::PropertyType Renderable3DComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

Renderable3DComponent_BindLua::Renderable3DComponent_BindLua(Renderable3DComponent* component) :component(component)
{
}

Renderable3DComponent_BindLua::Renderable3DComponent_BindLua(lua_State* L)
{
	component = nullptr;
}

Renderable3DComponent_BindLua::~Renderable3DComponent_BindLua()
{
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
