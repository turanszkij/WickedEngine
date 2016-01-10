#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiSprite.h"

class SpriteAnim_BindLua
{
public:
	wiSprite::Anim anim;

	static const char className[];
	static Luna<SpriteAnim_BindLua>::FunctionType methods[];
	static Luna<SpriteAnim_BindLua>::PropertyType properties[];

	SpriteAnim_BindLua(const wiSprite::Anim& anim);
	SpriteAnim_BindLua(lua_State *L);
	~SpriteAnim_BindLua();

	int SetRot(lua_State *L);
	int SetRotation(lua_State *L);
	int SetOpacity(lua_State *L);
	int SetFade(lua_State *L);
	int SetRepeatable(lua_State *L);
	int SetVelocity(lua_State *L);
	int SetScaleX(lua_State *L);
	int SetScaleY(lua_State *L);
	int SetMovingTexAnim(lua_State *L);
	int SetDrawRecAnim(lua_State *L);

	int GetRot(lua_State *L);
	int GetRotation(lua_State *L);
	int GetOpacity(lua_State *L);
	int GetFade(lua_State *L);
	int GetRepeatable(lua_State *L);
	int GetVelocity(lua_State *L);
	int GetScaleX(lua_State *L);
	int GetScaleY(lua_State *L);
	int GetMovingTexAnim(lua_State *L);
	int GetDrawRecAnim(lua_State *L);


	static void Bind();
};

class MovingTexData_BindLua
{
public:
	wiSprite::Anim::MovingTexData data;

	static const char className[];
	static Luna<MovingTexData_BindLua>::FunctionType methods[];
	static Luna<MovingTexData_BindLua>::PropertyType properties[];

	MovingTexData_BindLua(const wiSprite::Anim::MovingTexData& data);
	MovingTexData_BindLua(lua_State *L);
	~MovingTexData_BindLua();

	int SetSpeedX(lua_State* L);
	int SetSpeedY(lua_State* L);

	int GetSpeedX(lua_State* L);
	int GetSpeedY(lua_State* L);
};

class DrawRecData_BindLua
{
public:
	wiSprite::Anim::DrawRecData data;

	static const char className[];
	static Luna<DrawRecData_BindLua>::FunctionType methods[];
	static Luna<DrawRecData_BindLua>::PropertyType properties[];

	DrawRecData_BindLua(const wiSprite::Anim::DrawRecData& data);
	DrawRecData_BindLua(lua_State *L);
	~DrawRecData_BindLua();

	int SetOnFrameChangeWait(lua_State* L);
	int SetFrameCount(lua_State* L);
	int SetJumpX(lua_State* L);
	int SetJumpY(lua_State* L);
	int SetSizX(lua_State* L);
	int SetSizY(lua_State* L);

	int GetOnFrameChangeWait(lua_State* L);
	int GetFrameCount(lua_State* L);
	int GetJumpX(lua_State* L);
	int GetJumpY(lua_State* L);
	int GetSizX(lua_State* L);
	int GetSizY(lua_State* L);
};

