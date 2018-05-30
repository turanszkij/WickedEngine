#include "Renderable2DComponent_BindLua.h"
#include "wiResourceManager_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiFont_BindLua.h"

#include <sstream>

using namespace std;

const char Renderable2DComponent_BindLua::className[] = "Renderable2DComponent";

Luna<Renderable2DComponent_BindLua>::FunctionType Renderable2DComponent_BindLua::methods[] = {
	lunamethod(Renderable2DComponent_BindLua, GetContent),
	lunamethod(Renderable2DComponent_BindLua, AddSprite),
	lunamethod(Renderable2DComponent_BindLua, AddFont),
	lunamethod(Renderable2DComponent_BindLua, RemoveSprite),
	lunamethod(Renderable2DComponent_BindLua, RemoveFont),
	lunamethod(Renderable2DComponent_BindLua, ClearSprites),
	lunamethod(Renderable2DComponent_BindLua, ClearFonts),
	lunamethod(Renderable2DComponent_BindLua, GetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, GetFontOrder),
	lunamethod(Renderable2DComponent_BindLua, AddLayer),
	lunamethod(Renderable2DComponent_BindLua, GetLayers),
	lunamethod(Renderable2DComponent_BindLua, SetLayerOrder),
	lunamethod(Renderable2DComponent_BindLua, SetSpriteOrder),
	lunamethod(Renderable2DComponent_BindLua, SetFontOrder),
	lunamethod(Renderable2DComponent_BindLua, Initialize),
	lunamethod(Renderable2DComponent_BindLua, Load),
	lunamethod(Renderable2DComponent_BindLua, Unload),
	lunamethod(Renderable2DComponent_BindLua, Start),
	lunamethod(Renderable2DComponent_BindLua, Stop),
	lunamethod(Renderable2DComponent_BindLua, FixedUpdate),
	lunamethod(Renderable2DComponent_BindLua, Update),
	lunamethod(Renderable2DComponent_BindLua, Render),
	lunamethod(Renderable2DComponent_BindLua, Compose),
	lunamethod(RenderableComponent_BindLua, OnStart),
	lunamethod(RenderableComponent_BindLua, OnStop),
	lunamethod(RenderableComponent_BindLua, GetLayerMask),
	lunamethod(RenderableComponent_BindLua, SetLayerMask),
	{ NULL, NULL }
};
Luna<Renderable2DComponent_BindLua>::PropertyType Renderable2DComponent_BindLua::properties[] = {
	{ NULL, NULL }
};

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(Renderable2DComponent* component)
{
	this->component = component;
}

Renderable2DComponent_BindLua::Renderable2DComponent_BindLua(lua_State *L)
{
	component = new Renderable2DComponent();
}


Renderable2DComponent_BindLua::~Renderable2DComponent_BindLua()
{
}

int Renderable2DComponent_BindLua::AddSprite(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "AddSprite() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::lightcheck(L, 1);
		if (sprite != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				if(argc>1)
					ccomp->addSprite(sprite->sprite, wiLua::SGetString(L,2));
				else
					ccomp->addSprite(sprite->sprite);
			}
			else
			{
				wiLua::SError(L, "AddSprite(Sprite sprite, opt string layer) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "AddSprite(Sprite sprite, opt string layer) argument is not a Sprite!");
	}
	else
	{
		wiLua::SError(L, "AddSprite(Sprite sprite, opt string layer) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::AddFont(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "AddFont() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiFont_BindLua* font = Luna<wiFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				if (argc > 1)
					ccomp->addFont(font->font, wiLua::SGetString(L, 2));
				else
					ccomp->addFont(font->font);
			}
			else
			{
				wiLua::SError(L, "AddFont(Font font, opt string layer) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "AddFont(Font font, opt string layer) argument is not a Font!");
	}
	else
	{
		wiLua::SError(L, "AddFont(Font font, opt string layer) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::RemoveSprite(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "RemoveSprite() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::lightcheck(L, 1);
		if (sprite != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				ccomp->removeSprite(sprite->sprite);
			}
			else
			{
				wiLua::SError(L, "RemoveSprite(Sprite sprite) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "RemoveSprite(Sprite sprite) argument is not a Sprite!");
	}
	else
	{
		wiLua::SError(L, "RemoveSprite(Sprite sprite) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::RemoveFont(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "RemoveFont() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiFont_BindLua* font = Luna<wiFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				ccomp->removeFont(font->font);
			}
			else
			{
				wiLua::SError(L, "RemoveFont(Font font) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "RemoveFont(Font font) argument is not a Font!");
	}
	else
	{
		wiLua::SError(L, "RemoveFont(Font font) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::ClearSprites(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "ClearSprites() component is empty!");
		return 0;
	}
	Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
	if (ccomp != nullptr)
	{
		ccomp->clearSprites();
	}
	else
	{
		wiLua::SError(L, "ClearSprites() not a Renderable2DComponent!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::ClearFonts(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "ClearFonts() component is empty!");
		return 0;
	}
	Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
	if (ccomp != nullptr)
	{
		ccomp->clearFonts();
	}
	else
	{
		wiLua::SError(L, "ClearFonts() not a Renderable2DComponent!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::GetSpriteOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetSpriteOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::lightcheck(L, 1);
		if (sprite != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				wiLua::SSetInt(L, ccomp->getSpriteOrder(sprite->sprite));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetSpriteOrder(Sprite sprite) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "GetSpriteOrder(Sprite sprite) argument is not a Sprite!");
	}
	else
	{
		wiLua::SError(L, "GetSpriteOrder(Sprite sprite) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::GetFontOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetFontOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiFont_BindLua* font = Luna<wiFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
			if (ccomp != nullptr)
			{
				wiLua::SSetInt(L, ccomp->getFontOrder(font->font));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetFontOrder(Font font) not a Renderable2DComponent!");
			}
		}
		else
			wiLua::SError(L, "GetFontOrder(Font font) argument is not a Sprite!");
	}
	else
	{
		wiLua::SError(L, "GetFontOrder(Font font) not enough arguments!");
	}
	return 0;
}

int Renderable2DComponent_BindLua::AddLayer(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "AddLayer() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
		if (ccomp != nullptr)
		{
			ccomp->addLayer(wiLua::SGetString(L, 1));
		}
		else
		{
			wiLua::SError(L, "AddLayer(string name) not a Renderable2DComponent!");
		}
	}
	else
	{
		wiLua::SError(L, "AddLayer(string name) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::GetLayers(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetLayers() component is empty!");
		return 0;
	}
	
	Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
	if (ccomp != nullptr)
	{
		stringstream ss("");
		for (auto& x : ccomp->layers)
		{
			ss << x.name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	else
	{
		wiLua::SError(L, "GetLayers() not a Renderable2DComponent!");
	}

	return 0;
}
int Renderable2DComponent_BindLua::SetLayerOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLayerOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
		if (ccomp != nullptr)
		{
			ccomp->setLayerOrder(wiLua::SGetString(L, 1),wiLua::SGetInt(L,2));
		}
		else
		{
			wiLua::SError(L, "SetLayerOrder(string name, int order) not a Renderable2DComponent!");
		}
	}
	else
	{
		wiLua::SError(L, "SetLayerOrder(string name, int order) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::SetSpriteOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSpriteOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
		if (ccomp != nullptr)
		{
			wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::lightcheck(L, 1);
			if (sprite != nullptr)
			{
				ccomp->SetSpriteOrder(sprite->sprite, wiLua::SGetInt(L, 2));
			}
			else
			{
				wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) argument is not a Sprite!");
			}
		}
		else
		{
			wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not a Renderable2DComponent!");
		}
	}
	else
	{
		wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not enough arguments!");
	}
	return 0;
}
int Renderable2DComponent_BindLua::SetFontOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFontOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		Renderable2DComponent* ccomp = dynamic_cast<Renderable2DComponent*>(component);
		if (ccomp != nullptr)
		{
			wiFont_BindLua* font = Luna<wiFont_BindLua>::lightcheck(L, 1);
			if (font != nullptr)
			{
				ccomp->SetFontOrder(font->font, wiLua::SGetInt(L, 2));
			}
			else
			{
				wiLua::SError(L, "SetFontOrder(Font font, int order) argument is not a Font!");
			}
		}
		else
		{
			wiLua::SError(L, "SetFontOrder(Font font, int order) not a Renderable2DComponent!");
		}
	}
	else
	{
		wiLua::SError(L, "SetFontOrder(Font font, int order) not enough arguments!");
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