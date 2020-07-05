#include "wiSpriteFont_BindLua.h"
#include "wiFont.h"
#include "CommonInclude.h"
#include "Vector_BindLua.h"

using namespace std;

const char wiSpriteFont_BindLua::className[] = "SpriteFont";

Luna<wiSpriteFont_BindLua>::FunctionType wiSpriteFont_BindLua::methods[] = {
	lunamethod(wiSpriteFont_BindLua, GetText),
	lunamethod(wiSpriteFont_BindLua, SetSize),
	lunamethod(wiSpriteFont_BindLua, SetPos),
	lunamethod(wiSpriteFont_BindLua, SetSpacing),
	lunamethod(wiSpriteFont_BindLua, SetAlign),
	lunamethod(wiSpriteFont_BindLua, SetColor),
	lunamethod(wiSpriteFont_BindLua, SetShadowColor),

	lunamethod(wiSpriteFont_BindLua, SetStyle),
	lunamethod(wiSpriteFont_BindLua, SetText),
	lunamethod(wiSpriteFont_BindLua, GetSize),
	lunamethod(wiSpriteFont_BindLua, GetPos),
	lunamethod(wiSpriteFont_BindLua, GetSpacing),
	lunamethod(wiSpriteFont_BindLua, GetAlign),
	lunamethod(wiSpriteFont_BindLua, GetColor),
	lunamethod(wiSpriteFont_BindLua, GetShadowColor),

	{ NULL, NULL }
};
Luna<wiSpriteFont_BindLua>::PropertyType wiSpriteFont_BindLua::properties[] = {
	{ NULL, NULL }
};

wiSpriteFont_BindLua::wiSpriteFont_BindLua(const wiSpriteFont& font) : font(font)
{
}
wiSpriteFont_BindLua::wiSpriteFont_BindLua(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string text = wiLua::SGetString(L, 1);
		font.SetText(text);
	}
}


int wiSpriteFont_BindLua::SetStyle(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		string name = wiLua::SGetString(L, 1);
		font.params.style = wiFont::AddFontStyle(name.c_str());
	}
	else
	{
		wiLua::SError(L, "SetStyle(string style, opt int size = 16) not enough arguments!");
	}
	return 0;
}
int wiSpriteFont_BindLua::SetText(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
		font.SetText(wiLua::SGetString(L, 1));
	else
		font.SetText("");
	return 0;
}
int wiSpriteFont_BindLua::SetSize(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		font.params.size = wiLua::SGetInt(L, 1);
	}
	else
		wiLua::SError(L, "SetSize(int size) not enough arguments!");
	return 0;
}
int wiSpriteFont_BindLua::SetPos(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			font.params.posX = XMVectorGetX(param->vector);
			font.params.posY = XMVectorGetY(param->vector);
		}
		else
			wiLua::SError(L, "SetPos(Vector pos) argument is not a vector!");
	}
	else
		wiLua::SError(L, "SetPos(Vector pos) not enough arguments!");
	return 0;
}
int wiSpriteFont_BindLua::SetSpacing(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			font.params.spacingX = XMVectorGetX(param->vector);
			font.params.spacingY = XMVectorGetY(param->vector);
		}
		else
			wiLua::SError(L, "SetSpacing(Vector spacing) argument is not a vector!");
	}
	else
		wiLua::SError(L, "SetSpacing(Vector spacing) not enough arguments!");
	return 0;
}
int wiSpriteFont_BindLua::SetAlign(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		font.params.h_align = (wiFontAlign)wiLua::SGetInt(L, 1);
		if (argc > 1)
		{
			font.params.v_align = (wiFontAlign)wiLua::SGetInt(L, 2);
		}
	}
	else
		wiLua::SError(L, "SetAlign(WIFALIGN Halign, opt WIFALIGN Valign) not enough arguments!");
	return 0;
}
int wiSpriteFont_BindLua::SetColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			XMFLOAT4 unpacked;
			XMStoreFloat4(&unpacked, param->vector);
			font.params.color = wiColor::fromFloat4(unpacked);
		}
		else
		{
			int code = wiLua::SGetInt(L, 1);
			font.params.color.rgba = (uint32_t)code;
		}
	}
	else
		wiLua::SError(L, "SetColor(Vector value) not enough arguments!");
	return 0;
}
int wiSpriteFont_BindLua::SetShadowColor(lua_State* L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
		if (param != nullptr)
		{
			XMFLOAT4 unpacked;
			XMStoreFloat4(&unpacked, param->vector);
			font.params.shadowColor = wiColor::fromFloat4(unpacked);
		}
		else
		{
			int code = wiLua::SGetInt(L, 1);
			font.params.shadowColor.rgba = (uint32_t)code;
		}
	}
	else
		wiLua::SError(L, "SetShadowColor(Vector value) not enough arguments!");
	return 0;
}

int wiSpriteFont_BindLua::GetText(lua_State* L)
{
	wiLua::SSetString(L, font.GetTextA());
	return 1;
}
int wiSpriteFont_BindLua::GetSize(lua_State* L)
{
	wiLua::SSetInt(L, font.params.size);
	return 1;
}
int wiSpriteFont_BindLua::GetPos(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font.params.posX, (float)font.params.posY, 0, 0)));
	return 1;
}
int wiSpriteFont_BindLua::GetSpacing(lua_State* L)
{
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font.params.spacingX, (float)font.params.spacingY, 0, 0)));
	return 1;
}
int wiSpriteFont_BindLua::GetAlign(lua_State* L)
{
	wiLua::SSetInt(L, font.params.h_align);
	wiLua::SSetInt(L, font.params.v_align);
	return 2;
}
int wiSpriteFont_BindLua::GetColor(lua_State* L)
{
	XMFLOAT4 C = font.params.color.toFloat4();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&C)));
	return 1;
}
int wiSpriteFont_BindLua::GetShadowColor(lua_State* L)
{
	XMFLOAT4 C = font.params.color.toFloat4();
	Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&C)));
	return 1;
}

void wiSpriteFont_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiSpriteFont_BindLua>::Register(wiLua::GetLuaState());


		wiLua::RunText("WIFALIGN_LEFT = 0");
		wiLua::RunText("WIFALIGN_CENTER = 1");
		wiLua::RunText("WIFALIGN_RIGHT = 2");
		wiLua::RunText("WIFALIGN_TOP = 3");
		wiLua::RunText("WIFALIGN_BOTTOM = 4");
	}
}
