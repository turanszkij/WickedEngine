#include "LoadingScreenComponent_BindLua.h"

using namespace std;

const char LoadingScreenComponent_BindLua::className[] = "LoadingScreenComponent";

Luna<LoadingScreenComponent_BindLua>::FunctionType LoadingScreenComponent_BindLua::methods[] = {
	lunamethod(Renderable2DComponent_BindLua, AddSprite),
	lunamethod(Renderable2DComponent_BindLua, AddFont),
	lunamethod(Renderable2DComponent_BindLua, RemoveSprite),
	lunamethod(Renderable2DComponent_BindLua, RemoveFont),
	lunamethod(Renderable2DComponent_BindLua, ClearSprites),
	lunamethod(Renderable2DComponent_BindLua, ClearFonts),
	lunamethod(Renderable2DComponent_BindLua, GetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, GetFontOrder),
	lunamethod(Renderable2DComponent_BindLua, AddLayer),
	lunamethod(Renderable2DComponent_BindLua, GetLayers),
	lunamethod(Renderable2DComponent_BindLua, SetLayerOrder),
	lunamethod(Renderable2DComponent_BindLua, SetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, SetFontOrder),

	lunamethod(LoadingScreenComponent_BindLua, Initialize),
	lunamethod(LoadingScreenComponent_BindLua, Load),
	lunamethod(LoadingScreenComponent_BindLua, Unload),
	lunamethod(LoadingScreenComponent_BindLua, Start),
	lunamethod(LoadingScreenComponent_BindLua, Stop),
	lunamethod(LoadingScreenComponent_BindLua, FixedUpdate),
	lunamethod(LoadingScreenComponent_BindLua, Update),
	lunamethod(LoadingScreenComponent_BindLua, Render),
	lunamethod(LoadingScreenComponent_BindLua, Compose),
	lunamethod(RenderableComponent_BindLua, OnStart),
	lunamethod(RenderableComponent_BindLua, OnStop),
	lunamethod(RenderableComponent_BindLua, GetLayerMask),
	lunamethod(RenderableComponent_BindLua, SetLayerMask),

	lunamethod(LoadingScreenComponent_BindLua, AddLoadingTask),
	lunamethod(LoadingScreenComponent_BindLua, OnFinished),
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

int LoadingScreenComponent_BindLua::AddLoadingTask(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string task = wiLua::SGetString(L, 1);
		LoadingScreenComponent* loading = dynamic_cast<LoadingScreenComponent*>(component);
		if (loading != nullptr)
		{
			loading->addLoadingFunction(bind(&wiLua::RunText,wiLua::GetGlobal(),task));
		}
		else
			wiLua::SError(L, "AddLoader(string taskScript) component is not a LoadingScreenComponent!");
	}
	else
		wiLua::SError(L, "AddLoader(string taskScript) not enough arguments!");
	return 0;
}
int LoadingScreenComponent_BindLua::OnFinished(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string task = wiLua::SGetString(L, 1);
		LoadingScreenComponent* loading = dynamic_cast<LoadingScreenComponent*>(component);
		if (loading != nullptr)
		{
			loading->onFinished(bind(&wiLua::RunText, wiLua::GetGlobal(), task));
		}
		else
			wiLua::SError(L, "OnFinished(string taskScript) component is not a LoadingScreenComponent!");
	}
	else
		wiLua::SError(L, "OnFinished(string taskScript) not enough arguments!");
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
