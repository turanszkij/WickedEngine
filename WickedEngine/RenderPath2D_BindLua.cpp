#include "RenderPath2D_BindLua.h"
#include "wiSprite_BindLua.h"
#include "wiSpriteFont_BindLua.h"

#include <sstream>

using namespace std;

const char RenderPath2D_BindLua::className[] = "RenderPath2D";

Luna<RenderPath2D_BindLua>::FunctionType RenderPath2D_BindLua::methods[] = {
	lunamethod(RenderPath2D_BindLua, AddSprite),
	lunamethod(RenderPath2D_BindLua, AddFont),
	lunamethod(RenderPath2D_BindLua, RemoveSprite),
	lunamethod(RenderPath2D_BindLua, RemoveFont),
	lunamethod(RenderPath2D_BindLua, ClearSprites),
	lunamethod(RenderPath2D_BindLua, ClearFonts),
	lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
	lunamethod(RenderPath2D_BindLua, GetFontOrder),
	lunamethod(RenderPath2D_BindLua, AddLayer),
	lunamethod(RenderPath2D_BindLua, GetLayers),
	lunamethod(RenderPath2D_BindLua, SetLayerOrder),
	lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
	lunamethod(RenderPath2D_BindLua, SetFontOrder),
	lunamethod(RenderPath_BindLua, GetLayerMask),
	lunamethod(RenderPath_BindLua, SetLayerMask),
	{ NULL, NULL }
};
Luna<RenderPath2D_BindLua>::PropertyType RenderPath2D_BindLua::properties[] = {
	{ NULL, NULL }
};

int RenderPath2D_BindLua::AddSprite(lua_State *L)
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
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				if(argc>1)
					ccomp->AddSprite(&sprite->sprite, wiLua::SGetString(L,2));
				else
					ccomp->AddSprite(&sprite->sprite);
			}
			else
			{
				wiLua::SError(L, "AddSprite(Sprite sprite, opt string layer) not a RenderPath2D!");
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
int RenderPath2D_BindLua::AddFont(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "AddFont() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSpriteFont_BindLua* font = Luna<wiSpriteFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				if (argc > 1)
					ccomp->AddFont(&font->font, wiLua::SGetString(L, 2));
				else
					ccomp->AddFont(&font->font);
			}
			else
			{
				wiLua::SError(L, "AddFont(Font font, opt string layer) not a RenderPath2D!");
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
int RenderPath2D_BindLua::RemoveSprite(lua_State *L)
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
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				ccomp->RemoveSprite(&sprite->sprite);
			}
			else
			{
				wiLua::SError(L, "RemoveSprite(Sprite sprite) not a RenderPath2D!");
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
int RenderPath2D_BindLua::RemoveFont(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "RemoveFont() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSpriteFont_BindLua* font = Luna<wiSpriteFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				ccomp->RemoveFont(&font->font);
			}
			else
			{
				wiLua::SError(L, "RemoveFont(Font font) not a RenderPath2D!");
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
int RenderPath2D_BindLua::ClearSprites(lua_State *L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "ClearSprites() component is empty!");
		return 0;
	}
	RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
	if (ccomp != nullptr)
	{
		ccomp->ClearSprites();
	}
	else
	{
		wiLua::SError(L, "ClearSprites() not a RenderPath2D!");
	}
	return 0;
}
int RenderPath2D_BindLua::ClearFonts(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "ClearFonts() component is empty!");
		return 0;
	}
	RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
	if (ccomp != nullptr)
	{
		ccomp->ClearFonts();
	}
	else
	{
		wiLua::SError(L, "ClearFonts() not a RenderPath2D!");
	}
	return 0;
}
int RenderPath2D_BindLua::GetSpriteOrder(lua_State* L)
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
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				wiLua::SSetInt(L, ccomp->GetSpriteOrder(&sprite->sprite));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetSpriteOrder(Sprite sprite) not a RenderPath2D!");
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
int RenderPath2D_BindLua::GetFontOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetFontOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		wiSpriteFont_BindLua* font = Luna<wiSpriteFont_BindLua>::lightcheck(L, 1);
		if (font != nullptr)
		{
			RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
			if (ccomp != nullptr)
			{
				wiLua::SSetInt(L, ccomp->GetFontOrder(&font->font));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetFontOrder(Font font) not a RenderPath2D!");
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

int RenderPath2D_BindLua::AddLayer(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "AddLayer() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 0)
	{
		RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			ccomp->AddLayer(wiLua::SGetString(L, 1));
		}
		else
		{
			wiLua::SError(L, "AddLayer(string name) not a RenderPath2D!");
		}
	}
	else
	{
		wiLua::SError(L, "AddLayer(string name) not enough arguments!");
	}
	return 0;
}
int RenderPath2D_BindLua::GetLayers(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "GetLayers() component is empty!");
		return 0;
	}
	
	RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
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
		wiLua::SError(L, "GetLayers() not a RenderPath2D!");
	}

	return 0;
}
int RenderPath2D_BindLua::SetLayerOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetLayerOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			ccomp->SetLayerOrder(wiLua::SGetString(L, 1),wiLua::SGetInt(L,2));
		}
		else
		{
			wiLua::SError(L, "SetLayerOrder(string name, int order) not a RenderPath2D!");
		}
	}
	else
	{
		wiLua::SError(L, "SetLayerOrder(string name, int order) not enough arguments!");
	}
	return 0;
}
int RenderPath2D_BindLua::SetSpriteOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetSpriteOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			wiSprite_BindLua* sprite = Luna<wiSprite_BindLua>::lightcheck(L, 1);
			if (sprite != nullptr)
			{
				ccomp->SetSpriteOrder(&sprite->sprite, wiLua::SGetInt(L, 2));
			}
			else
			{
				wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) argument is not a Sprite!");
			}
		}
		else
		{
			wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not a RenderPath2D!");
		}
	}
	else
	{
		wiLua::SError(L, "SetSpriteOrder(Sprite sprite, int order) not enough arguments!");
	}
	return 0;
}
int RenderPath2D_BindLua::SetFontOrder(lua_State* L)
{
	if (component == nullptr)
	{
		wiLua::SError(L, "SetFontOrder() component is empty!");
		return 0;
	}
	int argc = wiLua::SGetArgCount(L);
	if (argc > 1)
	{
		RenderPath2D* ccomp = dynamic_cast<RenderPath2D*>(component);
		if (ccomp != nullptr)
		{
			wiSpriteFont_BindLua* font = Luna<wiSpriteFont_BindLua>::lightcheck(L, 1);
			if (font != nullptr)
			{
				ccomp->SetFontOrder(&font->font, wiLua::SGetInt(L, 2));
			}
			else
			{
				wiLua::SError(L, "SetFontOrder(Font font, int order) argument is not a Font!");
			}
		}
		else
		{
			wiLua::SError(L, "SetFontOrder(Font font, int order) not a RenderPath2D!");
		}
	}
	else
	{
		wiLua::SError(L, "SetFontOrder(Font font, int order) not enough arguments!");
	}
	return 0;
}

void RenderPath2D_BindLua::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<RenderPath2D_BindLua>::Register(wiLua::GetLuaState());
	}
}