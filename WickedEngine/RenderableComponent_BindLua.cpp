#include "RenderableComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"

using namespace std;

const char RenderableComponent_BindLua::className[] = "RenderableComponent";

Luna<RenderableComponent_BindLua>::FunctionType RenderableComponent_BindLua::methods[] = {
	lunamethod(RenderableComponent_BindLua, GetContent),
	lunamethod(RenderableComponent_BindLua, Initialize),
	lunamethod(RenderableComponent_BindLua, Load),
	lunamethod(RenderableComponent_BindLua, Unload),
	lunamethod(RenderableComponent_BindLua, Start),
	lunamethod(RenderableComponent_BindLua, Stop),
	lunamethod(RenderableComponent_BindLua, FixedUpdate),
	lunamethod(RenderableComponent_BindLua, Update),
	lunamethod(RenderableComponent_BindLua, Render),
	lunamethod(RenderableComponent_BindLua, Compose),
	lunamethod(RenderableComponent_BindLua, OnStart),
	lunamethod(RenderableComponent_BindLua, OnStop),
	lunamethod(RenderableComponent_BindLua, GetLayerMask),
	lunamethod(RenderableComponent_BindLua, SetLayerMask),
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

int RenderableComponent_BindLua::Load(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Load() component is null!");
		return 0;
	}
	component->Load();
	return 0;
}

int RenderableComponent_BindLua::Unload(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Unload() component is null!");
		return 0;
	}
	component->Unload();
	return 0;
}

int RenderableComponent_BindLua::Start(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Start() component is null!");
		return 0;
	}
	component->Start();
	return 0;
}

int RenderableComponent_BindLua::Stop(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Stop() component is null!");
		return 0;
	}
	component->Stop();
	return 0;
}

int RenderableComponent_BindLua::FixedUpdate(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "FixedUpdate() component is null!");
		return 0;
	}
	component->FixedUpdate();
	return 0;
}

int RenderableComponent_BindLua::Update(lua_State* L)
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

int RenderableComponent_BindLua::Render(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Render() component is null!");
		return 0;
	}
	component->Render();
	return 0;
}

int RenderableComponent_BindLua::Compose(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "Compose() component is null!");
		return 0;
	}
	component->Compose();
	return 0;
}


int RenderableComponent_BindLua::OnStart(lua_State* L)
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
int RenderableComponent_BindLua::OnStop(lua_State* L)
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


int RenderableComponent_BindLua::GetLayerMask(lua_State* L)
{
	uint32_t mask = component->getLayerMask();
	wiLua::SSetInt(L, *reinterpret_cast<int*>(&mask));
	return 1;
}
int RenderableComponent_BindLua::SetLayerMask(lua_State* L)
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

void RenderableComponent_BindLua::Bind()
{

	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<RenderableComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

