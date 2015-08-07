#include "wiSprite_BindLua.h"
#include "wiImageEffects_BindLua.h"
#include "SpriteAnim_BindLua.h"

const char wiSprite_BindLua::className[] = "Sprite";

Luna<wiSprite_BindLua>::FunctionType wiSprite_BindLua::methods[] = {
	lunamethod(wiSprite_BindLua, SetEffects),
	lunamethod(wiSprite_BindLua, GetEffects),
	lunamethod(wiSprite_BindLua, SetAnim),
	lunamethod(wiSprite_BindLua, GetAnim),
	{ NULL, NULL }
};
Luna<wiSprite_BindLua>::PropertyType wiSprite_BindLua::properties[] = {
	{ NULL, NULL }
};

wiSprite_BindLua::wiSprite_BindLua(wiSprite* sprite) :sprite(sprite)
{
}

wiSprite_BindLua::wiSprite_BindLua(lua_State *L)
{
	string name = "", mask = "", normal = "";
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		name = wiLua::SGetString(L, 1);
		if (argc > 1)
		{
			mask = wiLua::SGetString(L, 2);
			if (argc > 2)
			{
				normal = wiLua::SGetString(L, 3);
			}
		}
	}
	sprite = new wiSprite(name, mask, normal);
}


wiSprite_BindLua::~wiSprite_BindLua()
{
}

int wiSprite_BindLua::SetEffects(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "GetEffects() sprite is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		wiImageEffects_BindLua* effects = Luna<wiImageEffects_BindLua>::check(L, 2);
		if (effects != nullptr)
		{
			sprite->effects = effects->effects;
		}
	}
	else
	{
		wiLua::SError(L, "SetEffects(ImageEffects effects) not enough arguments!");
	}
	return 0;
}
int wiSprite_BindLua::GetEffects(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "GetEffects() sprite is null!");
		return 0;
	}
	Luna<wiImageEffects_BindLua>::push(L, new wiImageEffects_BindLua(sprite->effects));
	return 1;
}
int wiSprite_BindLua::SetAnim(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "SetAnim() sprite is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		SpriteAnim_BindLua* anim = Luna<SpriteAnim_BindLua>::check(L, 2);
		if (anim != nullptr)
		{
			sprite->anim = anim->anim;
		}
	}
	else
	{
		wiLua::SError(L, "SetAnim(SpriteAnim anim) not enough arguments!");
	}
	return 0;
}
int wiSprite_BindLua::GetAnim(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "GetAnim() sprite is null!");
		return 0;
	}
	Luna<SpriteAnim_BindLua>::push(L, new SpriteAnim_BindLua(sprite->anim));
	return 1;
}

void wiSprite_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<wiSprite_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}

