#include "RenderPath_BindLua.h"
#include "wiResourceManager_BindLua.h"

using namespace std;

const char RenderPath_BindLua::className[] = "RenderPath";

Luna<RenderPath_BindLua>::FunctionType RenderPath_BindLua::methods[] = {
	lunamethod(RenderPath_BindLua, GetContent),
	lunamethod(RenderPath_BindLua, Initialize),
	lunamethod(RenderPath_BindLua, Load),
	lunamethod(RenderPath_BindLua, Unload),
	lunamethod(RenderPath_BindLua, Start),
	lunamethod(RenderPath_BindLua, Stop),
	lunamethod(RenderPath_BindLua, FixedUpdate),
	lunamethod(RenderPath_BindLua, Update),
	lunamethod(RenderPath_BindLua, Render),
	lunamethod(RenderPath_BindLua, Compose),
	lunamethod(RenderPath_BindLua, OnStart),
	lunamethod(RenderPath_BindLua, OnStop),
	lunamethod(RenderPath_BindLua, GetLayerMask),
	lunamethod(RenderPath_BindLua, SetLayerMask),
	{ NULL, NULL }
};
Luna<RenderPath_BindLua>::PropertyType RenderPath_BindLua::properties[] = {
	{ NULL, NULL }
};

RenderPath_BindLua::RenderPath_BindLua(RenderPath* component) :component(component)
{
}

RenderPath_BindLua::RenderPath_BindLua(lua_State *L)
{
	component = new RenderPath();
}


RenderPath_BindLua::~RenderPath_BindLua()
{
}

int RenderPath_BindLua::GetContent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	Luna<wiResourceManager_BindLua>::push(L, new wiResourceManager_BindLua(&component->Content));
	return 1;
}

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

int RenderPath_BindLua::Load(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Load() component is null!");
		return 0;
	}
	component->Load();
	return 0;
}

int RenderPath_BindLua::Unload(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Unload() component is null!");
		return 0;
	}
	component->Unload();
	return 0;
}

int RenderPath_BindLua::Start(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Start() component is null!");
		return 0;
	}
	component->Start();
	return 0;
}

int RenderPath_BindLua::Stop(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Stop() component is null!");
		return 0;
	}
	component->Stop();
	return 0;
}

int RenderPath_BindLua::FixedUpdate(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "FixedUpdate() component is null!");
		return 0;
	}
	component->FixedUpdate();
	return 0;
}

int RenderPath_BindLua::Update(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Update(opt float dt = 0) component is null!");
		return 0;
	}
	float dt = 0.f;
	if (wiLua::SGetArgCount(L) > 0)
	{
		dt = wiLua::SGetFloat(L, 1);
	}
	component->Update(dt);
	return 0;
}

int RenderPath_BindLua::Render(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Render() component is null!");
		return 0;
	}
	component->Render();
	return 0;
}

int RenderPath_BindLua::Compose(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Compose() component is null!");
		return 0;
	}
	component->Compose();
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

