#include "wiSpriteFont_BindLua.h"
#include "wiFont.h"
#include "wiMath_BindLua.h"

namespace wi::lua
{

	const char SpriteFont_BindLua::className[] = "SpriteFont";

	Luna<SpriteFont_BindLua>::FunctionType SpriteFont_BindLua::methods[] = {
		lunamethod(SpriteFont_BindLua, SetText),
		lunamethod(SpriteFont_BindLua, SetSize),
		lunamethod(SpriteFont_BindLua, SetPos),
		lunamethod(SpriteFont_BindLua, SetSpacing),
		lunamethod(SpriteFont_BindLua, SetAlign),
		lunamethod(SpriteFont_BindLua, SetColor),
		lunamethod(SpriteFont_BindLua, SetShadowColor),
		lunamethod(SpriteFont_BindLua, SetBolden),
		lunamethod(SpriteFont_BindLua, SetSoftness),
		lunamethod(SpriteFont_BindLua, SetShadowBolden),
		lunamethod(SpriteFont_BindLua, SetShadowSoftness),
		lunamethod(SpriteFont_BindLua, SetShadowOffset),

		lunamethod(SpriteFont_BindLua, SetStyle),
		lunamethod(SpriteFont_BindLua, GetText),
		lunamethod(SpriteFont_BindLua, GetSize),
		lunamethod(SpriteFont_BindLua, GetPos),
		lunamethod(SpriteFont_BindLua, GetSpacing),
		lunamethod(SpriteFont_BindLua, GetAlign),
		lunamethod(SpriteFont_BindLua, GetColor),
		lunamethod(SpriteFont_BindLua, GetShadowColor),
		lunamethod(SpriteFont_BindLua, GetBolden),
		lunamethod(SpriteFont_BindLua, GetSoftness),
		lunamethod(SpriteFont_BindLua, GetShadowBolden),
		lunamethod(SpriteFont_BindLua, GetShadowSoftness),
		lunamethod(SpriteFont_BindLua, GetShadowOffset),

		{ NULL, NULL }
	};
	Luna<SpriteFont_BindLua>::PropertyType SpriteFont_BindLua::properties[] = {
		lunaproperty(SpriteFont_BindLua, Text),
		lunaproperty(SpriteFont_BindLua, Size),
		lunaproperty(SpriteFont_BindLua, Pos),
		lunaproperty(SpriteFont_BindLua, Spacing),
		lunaproperty(SpriteFont_BindLua, Align),
		lunaproperty(SpriteFont_BindLua, Color),
		lunaproperty(SpriteFont_BindLua, ShadowColor),
		lunaproperty(SpriteFont_BindLua, Bolden),
		lunaproperty(SpriteFont_BindLua, Softness),
		lunaproperty(SpriteFont_BindLua, ShadowBolden),
		lunaproperty(SpriteFont_BindLua, ShadowSoftness),
		lunaproperty(SpriteFont_BindLua, ShadowOffset),
		{ NULL, NULL }
	};

	SpriteFont_BindLua::SpriteFont_BindLua(const wi::SpriteFont& font) : font(font)
	{
	}
	SpriteFont_BindLua::SpriteFont_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string text = wi::lua::SGetString(L, 1);
			font.SetText(text);
		}
	}


	int SpriteFont_BindLua::SetStyle(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string name = wi::lua::SGetString(L, 1);
			font.params.style = wi::font::AddFontStyle(name.c_str());
		}
		else
		{
			wi::lua::SError(L, "SetStyle(string style, opt int size = 16) not enough arguments!");
		}
		return 0;
	}
	int SpriteFont_BindLua::SetText(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
			font.SetText(wi::lua::SGetString(L, 1));
		else
			font.SetText("");
		return 0;
	}
	int SpriteFont_BindLua::SetSize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			font.params.size = wi::lua::SGetInt(L, 1);
		}
		else
			wi::lua::SError(L, "SetSize(int size) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetPos(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				font.params.posX = param->x;
				font.params.posY = param->y;
			}
			else
				wi::lua::SError(L, "SetPos(Vector pos) argument is not a vector!");
		}
		else
			wi::lua::SError(L, "SetPos(Vector pos) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetSpacing(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				font.params.spacingX = param->x;
				font.params.spacingY = param->y;
			}
			else
				wi::lua::SError(L, "SetSpacing(Vector spacing) argument is not a vector!");
		}
		else
			wi::lua::SError(L, "SetSpacing(Vector spacing) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetAlign(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			font.params.h_align = (wi::font::Alignment)wi::lua::SGetInt(L, 1);
			if (argc > 1)
			{
				font.params.v_align = (wi::font::Alignment)wi::lua::SGetInt(L, 2);
			}
		}
		else
			wi::lua::SError(L, "SetAlign(WIFALIGN Halign, opt WIFALIGN Valign) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetColor(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				font.params.color = wi::Color::fromFloat4(*param);
			}
			else
			{
				int code = wi::lua::SGetInt(L, 1);
				font.params.color.rgba = (uint32_t)code;
			}
		}
		else
			wi::lua::SError(L, "SetColor(Vector value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetShadowColor(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				font.params.shadowColor = wi::Color::fromFloat4(*param);
			}
			else
			{
				int code = wi::lua::SGetInt(L, 1);
				font.params.shadowColor.rgba = (uint32_t)code;
			}
		}
		else
			wi::lua::SError(L, "SetShadowColor(Vector value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetBolden(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			font.params.bolden = value;
		}
		else
			wi::lua::SError(L, "SetBolden(float value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetSoftness(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			font.params.softness = value;
		}
		else
			wi::lua::SError(L, "SetSoftness(float value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetShadowBolden(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			font.params.shadow_bolden = value;
		}
		else
			wi::lua::SError(L, "SetShadowBolden(float value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetShadowSoftness(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			float value = wi::lua::SGetFloat(L, 1);
			font.params.shadow_softness = value;
		}
		else
			wi::lua::SError(L, "SetShadowSoftness(float value) not enough arguments!");
		return 0;
	}
	int SpriteFont_BindLua::SetShadowOffset(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* param = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (param != nullptr)
			{
				font.params.shadow_offset_x = param->x;
				font.params.shadow_offset_y = param->y;
			}
			else
				wi::lua::SError(L, "SetShadowOffset(Vector pos) argument is not a vector!");
		}
		else
			wi::lua::SError(L, "SetShadowOffset(Vector pos) not enough arguments!");
		return 0;
	}

	int SpriteFont_BindLua::GetText(lua_State* L)
	{
		wi::lua::SSetString(L, font.GetTextA());
		return 1;
	}
	int SpriteFont_BindLua::GetSize(lua_State* L)
	{
		wi::lua::SSetInt(L, font.params.size);
		return 1;
	}
	int SpriteFont_BindLua::GetPos(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font.params.posX, (float)font.params.posY, 0, 0)));
		return 1;
	}
	int SpriteFont_BindLua::GetSpacing(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMVectorSet((float)font.params.spacingX, (float)font.params.spacingY, 0, 0)));
		return 1;
	}
	int SpriteFont_BindLua::GetAlign(lua_State* L)
	{
		wi::lua::SSetInt(L, font.params.h_align);
		wi::lua::SSetInt(L, font.params.v_align);
		return 2;
	}
	int SpriteFont_BindLua::GetColor(lua_State* L)
	{
		XMFLOAT4 C = font.params.color.toFloat4();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&C)));
		return 1;
	}
	int SpriteFont_BindLua::GetShadowColor(lua_State* L)
	{
		XMFLOAT4 C = font.params.color.toFloat4();
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&C)));
		return 1;
	}
	int SpriteFont_BindLua::GetBolden(lua_State* L)
	{
		wi::lua::SSetFloat(L, font.params.bolden);
		return 1;
	}
	int SpriteFont_BindLua::GetSoftness(lua_State* L)
	{
		wi::lua::SSetFloat(L, font.params.softness);
		return 1;
	}
	int SpriteFont_BindLua::GetShadowBolden(lua_State* L)
	{
		wi::lua::SSetFloat(L, font.params.shadow_bolden);
		return 1;
	}
	int SpriteFont_BindLua::GetShadowSoftness(lua_State* L)
	{
		wi::lua::SSetFloat(L, font.params.shadow_softness);
		return 1;
	}
	int SpriteFont_BindLua::GetShadowOffset(lua_State* L)
	{
		XMFLOAT4 C = XMFLOAT4(font.params.shadow_offset_x, font.params.shadow_offset_y, 0, 0);
		Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat4(&C)));
		return 1;
	}

	void SpriteFont_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<SpriteFont_BindLua>::Register(wi::lua::GetLuaState());


			wi::lua::RunText("WIFALIGN_LEFT = 0");
			wi::lua::RunText("WIFALIGN_CENTER = 1");
			wi::lua::RunText("WIFALIGN_RIGHT = 2");
			wi::lua::RunText("WIFALIGN_TOP = 3");
			wi::lua::RunText("WIFALIGN_BOTTOM = 4");
		}
	}

}
