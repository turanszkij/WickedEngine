#include "SpriteAnim_BindLua.h"

const char SpriteAnim_BindLua::className[] = "SpriteAnim";

Luna<SpriteAnim_BindLua>::FunctionType SpriteAnim_BindLua::methods[] = {
	lunamethod(SpriteAnim_BindLua, SetRot),
	{ NULL, NULL }
};
Luna<SpriteAnim_BindLua>::PropertyType SpriteAnim_BindLua::properties[] = {
	{ NULL, NULL }
};

SpriteAnim_BindLua::SpriteAnim_BindLua(const wiSprite::Anim& anim) :anim(anim)
{
}

SpriteAnim_BindLua::SpriteAnim_BindLua(lua_State *L)
{
	anim = wiSprite::Anim();
}

SpriteAnim_BindLua::~SpriteAnim_BindLua()
{
}


int SpriteAnim_BindLua::SetRot(lua_State *L)
{
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		anim.rot = wiLua::SGetFloat(L, 1);
	}
	else
	{
		wiLua::SError(L, "SetRot(float rot) not enough arguments!");
	}
	return 0;
}

void SpriteAnim_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<SpriteAnim_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}
