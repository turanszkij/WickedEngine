#include "LoadingScreenComponent_BindLua.h"

const char LoadingScreenComponent_BindLua::className[] = "LoadingScreenComponent";

Luna<LoadingScreenComponent_BindLua>::FunctionType LoadingScreenComponent_BindLua::methods[] = {
	lunamethod(LoadingScreenComponent_BindLua, GetContent),
	lunamethod(LoadingScreenComponent_BindLua, AddSprite),
	lunamethod(LoadingScreenComponent_BindLua, Initialize),
	lunamethod(LoadingScreenComponent_BindLua, Load),
	lunamethod(LoadingScreenComponent_BindLua, Unload),
	lunamethod(LoadingScreenComponent_BindLua, Start),
	lunamethod(LoadingScreenComponent_BindLua, Stop),
	lunamethod(LoadingScreenComponent_BindLua, Update),
	lunamethod(LoadingScreenComponent_BindLua, Render),
	lunamethod(LoadingScreenComponent_BindLua, Compose),

	lunamethod(LoadingScreenComponent_BindLua, AddLoadingComponent),
	{ NULL, NULL }
};
Luna<LoadingScreenComponent_BindLua>::PropertyType LoadingScreenComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

LoadingScreenComponent_BindLua::LoadingScreenComponent_BindLua(LoadingScreenComponent* component)
{
	this->component = component;
}

LoadingScreenComponent_BindLua::LoadingScreenComponent_BindLua(lua_State *L)
{
	component = new LoadingScreenComponent();
}

LoadingScreenComponent_BindLua::~LoadingScreenComponent_BindLua()
{
}

//TODO
int LoadingScreenComponent_BindLua::AddLoadingComponent(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		RenderableComponent_BindLua* comp = Luna<RenderableComponent_BindLua>::lightcheck(L, 1);
		if (comp != nullptr)
		{
			loadingScreen->addLoadingComponent(comp->component);
		}
		else
			wiLua::SError(L, "AddLoadingComponent(RenderableComponent component) argument is not a RenderableComponent!");
	}
	else
		wiLua::SError(L, "AddLoadingComponent(RenderableComponent component) not enough arguments!");
	return 0;
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
