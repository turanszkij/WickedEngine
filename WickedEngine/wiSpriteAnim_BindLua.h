#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSprite.h"

namespace wi::lua
{

	class SpriteAnim_BindLua
	{
	public:
		wi::Sprite::Anim anim;

		inline static constexpr char className[] = "SpriteAnim";
		static Luna<SpriteAnim_BindLua>::FunctionType methods[];
		static Luna<SpriteAnim_BindLua>::PropertyType properties[];

		SpriteAnim_BindLua(const wi::Sprite::Anim& anim);
		SpriteAnim_BindLua(lua_State* L);

		int SetRot(lua_State* L);
		int SetRotation(lua_State* L);
		int SetOpacity(lua_State* L);
		int SetFade(lua_State* L);
		int SetRepeatable(lua_State* L);
		int SetVelocity(lua_State* L);
		int SetScaleX(lua_State* L);
		int SetScaleY(lua_State* L);
		int SetMovingTexAnim(lua_State* L);
		int SetDrawRecAnim(lua_State* L);

		int GetRot(lua_State* L);
		int GetRotation(lua_State* L);
		int GetOpacity(lua_State* L);
		int GetFade(lua_State* L);
		int GetRepeatable(lua_State* L);
		int GetVelocity(lua_State* L);
		int GetScaleX(lua_State* L);
		int GetScaleY(lua_State* L);
		int GetMovingTexAnim(lua_State* L);
		int GetDrawRecAnim(lua_State* L);


		static void Bind();
	};

	class MovingTexAnim_BindLua
	{
	public:
		wi::Sprite::Anim::MovingTexAnim anim;

		inline static constexpr char className[] = "MovingTexAnim";
		static Luna<MovingTexAnim_BindLua>::FunctionType methods[];
		static Luna<MovingTexAnim_BindLua>::PropertyType properties[];

		MovingTexAnim_BindLua(const wi::Sprite::Anim::MovingTexAnim& anim);
		MovingTexAnim_BindLua(lua_State* L);
		~MovingTexAnim_BindLua();

		int SetSpeedX(lua_State* L);
		int SetSpeedY(lua_State* L);

		int GetSpeedX(lua_State* L);
		int GetSpeedY(lua_State* L);
	};

	class DrawRectAnim_BindLua
	{
	public:
		wi::Sprite::Anim::DrawRectAnim anim;

		inline static constexpr char className[] = "DrawRectAnim";
		static Luna<DrawRectAnim_BindLua>::FunctionType methods[];
		static Luna<DrawRectAnim_BindLua>::PropertyType properties[];

		DrawRectAnim_BindLua(const wi::Sprite::Anim::DrawRectAnim& anim);
		DrawRectAnim_BindLua(lua_State* L);
		~DrawRectAnim_BindLua();

		int SetFrameRate(lua_State* L);
		int SetFrameCount(lua_State* L);
		int SetHorizontalFrameCount(lua_State* L);

		int GetFrameRate(lua_State* L);
		int GetFrameCount(lua_State* L);
		int GetHorizontalFrameCount(lua_State* L);
	};

}
