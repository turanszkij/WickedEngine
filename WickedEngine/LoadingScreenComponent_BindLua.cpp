#include "LoadingScreenComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"

const char LoadingScreenComponent_BindLua::className[] = "LoadingScreenComponent";

Luna<LoadingScreenComponent_BindLua>::FunctionType LoadingScreenComponent_BindLua::methods[] = {
	lunamethod(LoadingScreenComponent_BindLua, GetContent),
	lunamethod(LoadingScreenComponent_BindLua, AddSprite),
	{ NULL, NULL }
};
Luna<LoadingScreenComponent_BindLua>::PropertyType LoadingScreenComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

LoadingScreenComponent_BindLua::LoadingScreenComponent_BindLua(LoadingScreenComponent* component) :component(component)
{
}

LoadingScreenComponent_BindLua::LoadingScreenComponent_BindLua(lua_State *L)
{
	component = new LoadingScreenComponent();
}


LoadingScreenComponent_BindLua::~LoadingScreenComponent_BindLua()
{
}

void LoadingScreenComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<LoadingScreenComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
