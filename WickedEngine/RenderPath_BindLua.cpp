#include "RenderPath_BindLua.h"

using namespace std;

const char RenderPath_BindLua::className[] = "RenderPath";

Luna<RenderPath_BindLua>::FunctionType RenderPath_BindLua::methods[] = {
	lunamethod(RenderPath_BindLua, Initialize),
	lunamethod(RenderPath_BindLua, OnStart),
	lunamethod(RenderPath_BindLua, OnStop),
	lunamethod(RenderPath_BindLua, GetLayerMask),
	lunamethod(RenderPath_BindLua, SetLayerMask),
	{ NULL, NULL }
};
Luna<RenderPath_BindLua>::PropertyType RenderPath_BindLua::properties[] = {
	{ NULL, NULL }
};


int RenderPath_BindLua::Initialize(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Initialize() component is null!");
		return 0;
	}
	component->Initialize();
	return 0;
}


int RenderPath_BindLua::OnStart(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string task = wiLua::SGetString(L, 1);
		component->onStart = bind(&wiLua::RunText, wiLua::GetGlobal(), task);
	}
	else
		wiLua::SError(L, "OnStart(string taskScript) not enough arguments!");
	return 0;
}
int RenderPath_BindLua::OnStop(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string task = wiLua::SGetString(L, 1);
		component->onStop = bind(&wiLua::RunText, wiLua::GetGlobal(), task);
	}
	else
		wiLua::SError(L, "OnStop(string taskScript) not enough arguments!");
	return 0;
}


int RenderPath_BindLua::GetLayerMask(lua_State* L)
{
	uint32_t mask = component->getLayerMask();
	wiLua::SSetInt(L, *reinterpret_cast<int*>(&mask));
	return 1;
}
int RenderPath_BindLua::SetLayerMask(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		int mask = wiLua::SGetInt(L, 1);
		component->setlayerMask(*reinterpret_cast<uint32_t*>(&mask));
	}
	else
		wiLua::SError(L, "SetLayerMask(uint mask) not enough arguments!");
	return 0;
}

void RenderPath_BindLua::Bind()
{

	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<RenderPath_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

