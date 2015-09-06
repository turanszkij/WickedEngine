#include "wiFont_BindLua.h"
#include "wiFont.h"
#include "CommonInclude.h"

const char wiFont_BindLua::className[] = "Font";

Luna<wiFont_BindLua>::FunctionType wiFont_BindLua::methods[] = {
	lunamethod(wiFont_BindLua, GetProperties),
	lunamethod(wiFont_BindLua, SetProperties),
	{ NULL, NULL }
};
Luna<wiFont_BindLua>::PropertyType wiFont_BindLua::properties[] = {
	{ NULL, NULL }
};

wiFont_BindLua::wiFont_BindLua(wiFont* font)
{
	if (font != nullptr)
		this->font = new wiFont(*font);
	else
		this->font = nullptr;
}
wiFont_BindLua::wiFont_BindLua(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string text = wiLua::SGetString(L, 1);
		font = new wiFont(text);
	}
	else
		font = nullptr;
}
wiFont_BindLua::~wiFont_BindLua()
{
	SAFE_DELETE(font);
}


int wiFont_BindLua::GetProperties(lua_State* L)
{
	return 0;
}
int wiFont_BindLua::SetProperties(lua_State* L)
{
	return 0;
}

void wiFont_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiFont_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
