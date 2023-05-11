#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSpriteFont.h"

namespace wi::lua
{
	class SpriteFont_BindLua
	{
	public:
		wi::SpriteFont font;

		inline static constexpr char className[] = "SpriteFont";
		static Luna<SpriteFont_BindLua>::FunctionType methods[];
		static Luna<SpriteFont_BindLua>::PropertyType properties[];

		SpriteFont_BindLua(const wi::SpriteFont& font);
		SpriteFont_BindLua(lua_State* L);

		int SetStyle(lua_State* L);
		int SetText(lua_State* L);
		int SetSize(lua_State* L);
		int SetPos(lua_State* L);
		int SetSpacing(lua_State* L);
		int SetAlign(lua_State* L);
		int SetColor(lua_State* L);
		int SetShadowColor(lua_State* L);
		int SetBolden(lua_State* L);
		int SetSoftness(lua_State* L);
		int SetShadowBolden(lua_State* L);
		int SetShadowSoftness(lua_State* L);
		int SetShadowOffset(lua_State* L);
		int SetHorizontalWrapping(lua_State* L);
		int SetHidden(lua_State* L);

		int GetText(lua_State* L);
		int GetSize(lua_State* L);
		int GetPos(lua_State* L);
		int GetSpacing(lua_State* L);
		int GetAlign(lua_State* L);
		int GetColor(lua_State* L);
		int GetShadowColor(lua_State* L);
		int GetBolden(lua_State* L);
		int GetSoftness(lua_State* L);
		int GetShadowBolden(lua_State* L);
		int GetShadowSoftness(lua_State* L);
		int GetShadowOffset(lua_State* L);
		int GetHorizontalWrapping(lua_State* L);
		int IsHidden(lua_State* L);

		int TextSize(lua_State* L);

		int SetTypewriterTime(lua_State* L);
		int SetTypewriterLooped(lua_State* L);
		int SetTypewriterCharacterStart(lua_State* L);
		int SetTypewriterSound(lua_State* L);
		int ResetTypewriter(lua_State* L);
		int TypewriterFinish(lua_State* L);
		int IsTypewriterFinished(lua_State* L);

		static void Bind();
	};
}
