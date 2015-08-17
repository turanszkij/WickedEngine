#include "RenderableComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"

const char RenderableComponent_BindLua::className[] = "RenderableComponent";

Luna<RenderableComponent_BindLua>::FunctionType RenderableComponent_BindLua::methods[] = {
	lunamethod(RenderableComponent_BindLua, GetContent),
	lunamethod(RenderableComponent_BindLua, Initialize),
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
	component = new RenderableComponent();
}


RenderableComponent_BindLua::~RenderableComponent_BindLua()
{
}

int RenderableComponent_BindLua::GetContent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	Luna<wiResourceManager_BindLua>::push(L, new wiResourceManager_BindLua(&component->Content));
	return 1;
}

int RenderableComponent_BindLua::Initialize(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Initialize() component is null!");
		return 0;
	}
	component->Initialize();
	return 0;
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

