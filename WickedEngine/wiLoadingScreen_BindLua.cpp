#include "wiLoadingScreen_BindLua.h"

namespace wi::lua
{

	Luna<LoadingScreen_BindLua>::FunctionType LoadingScreen_BindLua::methods[] = {
		lunamethod(RenderPath2D_BindLua, AddSprite),
		lunamethod(RenderPath2D_BindLua, AddFont),
		lunamethod(RenderPath2D_BindLua, RemoveSprite),
		lunamethod(RenderPath2D_BindLua, RemoveFont),
		lunamethod(RenderPath2D_BindLua, ClearSprites),
		lunamethod(RenderPath2D_BindLua, ClearFonts),
		lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, GetFontOrder),
		lunamethod(RenderPath2D_BindLua, AddLayer),
		lunamethod(RenderPath2D_BindLua, GetLayers),
		lunamethod(RenderPath2D_BindLua, SetLayerOrder),
		lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, SetFontOrder),

		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),

		lunamethod(LoadingScreen_BindLua, AddLoadingTask),
		lunamethod(LoadingScreen_BindLua, OnFinished),
		{ NULL, NULL }
	};
	Luna<LoadingScreen_BindLua>::PropertyType LoadingScreen_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int LoadingScreen_BindLua::AddLoadingTask(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string task = wi::lua::SGetString(L, 1);
			LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
			if (loading != nullptr)
			{
				loading->addLoadingFunction([=](wi::jobsystem::JobArgs args) {
					wi::lua::RunText(task);
					});
			}
			else
				wi::lua::SError(L, "AddLoader(string taskScript) component is not a LoadingScreen!");
		}
		else
			wi::lua::SError(L, "AddLoader(string taskScript) not enough arguments!");
		return 0;
	}
	int LoadingScreen_BindLua::OnFinished(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string task = wi::lua::SGetString(L, 1);
			LoadingScreen* loading = dynamic_cast<LoadingScreen*>(component);
			if (loading != nullptr)
			{
				loading->onFinished([=] {
					wi::lua::RunText(task);
					});
			}
			else
				wi::lua::SError(L, "OnFinished(string taskScript) component is not a LoadingScreen!");
		}
		else
			wi::lua::SError(L, "OnFinished(string taskScript) not enough arguments!");
		return 0;
	}

	void LoadingScreen_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<LoadingScreen_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

}
