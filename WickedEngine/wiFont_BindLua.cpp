#include "wiFont_BindLua.h"
#include "wiFont.h"
#include "CommonInclude.h"
#include "Vector_BindLua.h"

using namespace std;

const char wiFont_BindLua::className[] = "Font";

Luna<wiFont_BindLua>::FunctionType wiFont_BindLua::methods[] = {
	lunamethod(wiFont_BindLua, GetText),
	lunamethod(wiFont_BindLua, SetSize),
	lunamethod(wiFont_BindLua, SetPos),
	lunamethod(wiFont_BindLua, SetSpacing),
	lunamethod(wiFont_BindLua, SetAlign),

	lunamethod(wiFont_BindLua, SetStyle),
	lunamethod(wiFont_BindLua, SetText),
	lunamethod(wiFont_BindLua, GetSize),
	lunamethod(wiFont_BindLua, GetPos),
	lunamethod(wiFont_BindLua, GetSpacing),
	lunamethod(wiFont_BindLua, GetAlign),

	lunamethod(wiFont_BindLua, Destroy),
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
		this->font = new wiFont();
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
		font = new wiFont();
}
wiFont_BindLua::~wiFont_BindLua()
{
}


int wiFont_BindLua::SetStyle(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string name = wiLua::SGetString(L, 1);
		font->style = wiFont::AddFontStyle(name);
	}
	else
	{
		wiLua::SError(L, "SetStyle(string style, opt int size = 16) not enough arguments!");
	}
	return 0;
}
int wiFont_BindLua::SetText(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
		font->SetText(wiLua::SGetString(L, 1));
	else
		font->SetText("");
	return 0;
}
int wiFont_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		font->params.size = wiLua::SGetInt(L, 1);
	}
	else
		wiLua::SError(L, "SetSize(int size) not enough arguments!");
	return 0;
}
int wiFont_BindLua::SetPos(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			font->params.posX = (int)XMVectorGetX(param->vector);
			font->params.posY = (int)XMVectorGetY(param->vector);
		}
		else
			wiLua::SError(L, "SetPos(Vector pos) argument is not a vector!");
	}
	else
		wiLua::SError(L, "SetPos(Vector pos) not enough arguments!");
	return 0;
}
int wiFont_BindLua::SetSpacing(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			font->params.spacingX = (int)XMVectorGetX(param->vector);
			font->params.spacingY = (int)XMVectorGetY(param->vector);
		}
		else
			wiLua::SError(L, "SetSpacing(Vector spacing) argument is not a vector!");
	}
	else
		wiLua::SError(L, "SetSpacing(Vector spacing) not enough arguments!");
	return 0;
}
int wiFont_BindLua::SetAlign(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		font->params.h_align = (wiFontAlign)wiLua::SGetInt(L, 1);
		if (argc > 1)
		{
			font->params.v_align = (wiFontAlign)wiLua::SGetInt(L, 2);
		}
	}
	else
		wiLua::SError(L, "SetAlign(WIFALIGN Halign, opt WIFALIGN Valign) not enough arguments!");
	return 0;
}

int wiFont_BindLua::GetText(lua_State* L)
{
	wiLua::SSetString(L, font->GetTextA());
	return 1;
}
int wiFont_BindLua::GetSize(lua_State* L)
{
	wiLua::SSetInt(L, font->params.size);
	return 1;
}
int wiFont_BindLua::GetPos(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font->params.posX, (float)font->params.posY, 0, 0)));
	return 1;
}
int wiFont_BindLua::GetSpacing(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font->params.spacingX, (float)font->params.spacingY, 0, 0)));
	return 1;
}
int wiFont_BindLua::GetAlign(lua_State* L)
{
	wiLua::SSetInt(L, font->params.h_align);
	wiLua::SSetInt(L, font->params.v_align);
	return 2;
}

int wiFont_BindLua::Destroy(lua_State* L)
{
	if (font != nullptr)
	{
		font->CleanUp();
		delete font;
		font = nullptr;
	}
	return 0;
}

void wiFont_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiFont_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());


		wiLua::GetGlobal()->RunText("WIFALIGN_LEFT = 0");
		wiLua::GetGlobal()->RunText("WIFALIGN_CENTER = 1");
		wiLua::GetGlobal()->RunText("WIFALIGN_RIGHT = 2");
		wiLua::GetGlobal()->RunText("WIFALIGN_TOP = 3");
		wiLua::GetGlobal()->RunText("WIFALIGN_BOTTOM = 4");
	}
}
