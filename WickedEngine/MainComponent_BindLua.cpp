#include "MainComponent_BindLua.h"
#include "Renderable3DComponent_BindLua.h"

const char MainComponent_BindLua::className[] = "MainComponent";

Luna<MainComponent_BindLua>::FunctionType MainComponent_BindLua::methods[] = {
	lunamethod(MainComponent_BindLua, GetActiveComponent),
	lunamethod(MainComponent_BindLua, SetActiveComponent),
	lunamethod(MainComponent_BindLua, SetFrameSkip),
	{ NULL, NULL }
};
Luna<MainComponent_BindLua>::PropertyType MainComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

MainComponent_BindLua::MainComponent_BindLua(MainComponent* component) :component(component)
{
}

MainComponent_BindLua::MainComponent_BindLua(lua_State *L)
{
	component = nullptr;
}

MainComponent_BindLua::~MainComponent_BindLua()
{
}

int MainComponent_BindLua::GetActiveComponent(lua_State *L)
{
	Renderable3DComponent* comp = dynamic_cast<Renderable3DComponent*>(component->getActiveComponent());
	if (comp != nullptr)
	{
		Luna<Renderable3DComponent_BindLua>::push(L, new Renderable3DComponent_BindLua(comp));
		return 1;
	}

	wiLua::SError(L, "GetActiveComponent() Warning: type of active component not registered with lua!");
	return 0;
}
int MainComponent_BindLua::SetActiveComponent(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		//TODO
	}
	else
	{
		wiLua::SError(L, "SetActiveComponent(Renderable3DComponent component) not enought arguments!");
		return 0;
	}
	return 0;
}
int MainComponent_BindLua::SetFrameSkip(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		component->setFrameSkip(wiLua::SGetBool(L, 2));
	}
	else
		wiLua::SError(L, "SetFrameSkip(bool enabled) not enought arguments!");
	return 0;
}

void MainComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<MainComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
