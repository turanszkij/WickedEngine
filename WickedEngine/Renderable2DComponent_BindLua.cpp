#include "Renderable2DComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiSprite.h"

const char Renderable2DComponent_BindLua::className[] = "Renderable2DComponent";

Luna<Renderable2DComponent_BindLua>::FunctionType Renderable2DComponent_BindLua::methods[] = {
	lunamethod(Renderable2DComponent_BindLua, GetContent),
	lunamethod(Renderable2DComponent_BindLua, AddSprite),
	{ NULL, NULL }
};
Luna<Renderable2DComponent_BindLua>::PropertyType Renderable2DComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(Renderable2DComponent* component) :component(component)
{
}

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(lua_State *L)
{
	component = new Renderable2DComponent();
}


Renderable2DComponent_BindLua::~Renderable2DComponent_BindLua()
{
}

int Renderable2DComponent_BindLua::GetContent(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	Luna<wiResourceManager_BindLua>::push(L, new wiResourceManager_BindLua(&component->Content));
	return 1;
}

int Renderable2DComponent_BindLua::AddSprite(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetContent() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		string tex = wiLua::SGetString(L, 2);
		string mask = "";
		string nor = "";
		if (argc > 2)
		{
			mask = wiLua::SGetString(L, 3);
			if (argc > 3)
			{
				nor = wiLua::SGetString(L, 4);
			}
		}
		wiSprite* sprite = new wiSprite(tex,mask,nor,&component->Content);
		sprite->effects = wiImageEffects(100, 100);
		component->addSprite(sprite);
	}
	else
	{
		wiLua::SError(L, "AddSprite(Sprite sprite) not enough arguments!");
	}
	return 0;
}

void Renderable2DComponent_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<Renderable2DComponent_BindLua>::Register(wiLua::GetGlobal()->GetLuaState());
	}
}