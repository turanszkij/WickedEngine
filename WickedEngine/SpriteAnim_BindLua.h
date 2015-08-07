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

	static void Bind();
};

