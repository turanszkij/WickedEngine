#include "wiSprite_BindLua.h"
#include "wiImageParams_BindLua.h"
#include "SpriteAnim_BindLua.h"

using namespace std;

const char wiSprite_BindLua::className[] = "Sprite";

Luna<wiSprite_BindLua>::FunctionType wiSprite_BindLua::methods[] = {
	lunamethod(wiSprite_BindLua, SetParams),
	lunamethod(wiSprite_BindLua, GetParams),
	lunamethod(wiSprite_BindLua, SetAnim),
	lunamethod(wiSprite_BindLua, GetAnim),

	lunamethod(wiSprite_BindLua, Destroy),
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

int wiSprite_BindLua::SetParams(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "GetParams() sprite is null!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiImageParams_BindLua* params = Luna<wiImageParams_BindLua>::check(L, 1);
		if (params != nullptr)
		{
			sprite->params = params->params;
		}
	}
	else
	{
		wiLua::SError(L, "SetParams(ImageEffects effects) not enough arguments!");
	}
	return 0;
}
int wiSprite_BindLua::GetParams(lua_State *L)
{
	if (sprite == nullptr)
	{
		wiLua::SError(L, "GetParams() sprite is null!");
		return 0;
	}
	Luna<wiImageParams_BindLua>::push(L, new wiImageParams_BindLua(sprite->params));
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
	if (argc > 0)
	{
		SpriteAnim_BindLua* anim = Luna<SpriteAnim_BindLua>::check(L, 1);
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

int wiSprite_BindLua::Destroy(lua_State* L)
{
	if (sprite != nullptr)
	{
		sprite->CleanUp();
		delete sprite;
		sprite = nullptr;
	}
	return 0;
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

