#include "wiRenderer_BindLua.h"
#include "wiRenderer.h"
#include "wiLoader.h"
#include "wiHelper.h"
#include "wiLoader_BindLua.h"

namespace wiRenderer_BindLua
{

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			wiLua::GetGlobal()->RegisterFunc("GetArmatures", GetArmatures);
			wiLua::GetGlobal()->RegisterFunc("GetObjects", GetObjects);
			wiLua::GetGlobal()->RegisterFunc("GetObject", GetObjectLua);
			wiLua::GetGlobal()->RegisterFunc("GetMeshes", GetMeshes);
			wiLua::GetGlobal()->RegisterFunc("GetLights", GetLights);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);
			wiLua::GetGlobal()->RegisterFunc("GetGameSpeed", GetGameSpeed);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);

			wiLua::GetGlobal()->RegisterFunc("SetGameSpeed", SetGameSpeed);

			wiLua::GetGlobal()->RegisterFunc("LoadModel", LoadModel);
			wiLua::GetGlobal()->RegisterFunc("FinishLoading", FinishLoading);
		}
	}

	int GetArmatures(lua_State* L)
	{
		stringstream ss("");
		ss << "Armature list:" << endl;
		for (auto& x : wiRenderer::armatures)
		{
			ss << x->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetObjects(lua_State* L)
	{
		stringstream ss("");
		ss << "Object list:" << endl;
		for (auto& x : wiRenderer::objects)
		{
			ss << x->name << endl;
		}
		for (auto& x : wiRenderer::objects_trans)
		{
			ss << x->name << endl;
		}
		for (auto& x : wiRenderer::objects_water)
		{
			ss << x->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetObjectLua(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			string name = wiLua::SGetString(L, 1);
			Object* object = wiRenderer::getObjectByName(name);
			if (object != nullptr)
			{
				Luna<Object_BindLua>::push(L, new Object_BindLua(object));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetObject(String name) object not found!");
				return 0;
			}
		}
		return 0;
	}
	int GetMeshes(lua_State* L)
	{
		stringstream ss("");
		ss << "Mesh list:" << endl;
		for (auto& x : wiRenderer::meshes)
		{
			ss << x.second->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetLights(lua_State* L)
	{
		stringstream ss("");
		ss << "Light list:" << endl;
		for (auto& x : wiRenderer::lights)
		{
			ss << x->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetMaterials(lua_State* L)
	{
		stringstream ss("");
		ss << "Material list:" << endl;
		for (auto& x : wiRenderer::materials)
		{
			ss << x.second->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetGameSpeed(lua_State* L)
	{
		wiLua::SSetFloat(L, wiRenderer::GetGameSpeed());
		return 1;
	}


	int SetGameSpeed(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetGameSpeed(wiLua::SGetFloat(L, 1));
		}
		else
		{
			wiLua::SError(L,"SetGameSpeed(float) not enough arguments!");
		}
		return 0;
	}

	int LoadModel(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			string dir = wiLua::SGetString(L, 1);
			string name = wiLua::SGetString(L, 2);
			string identifier = "common";
			if (argc > 2)
			{
				identifier = wiLua::SGetString(L, 3);
			}
			wiRenderer::LoadModel(dir, name, XMMatrixIdentity(), identifier, wiRenderer::physicsEngine);
		}
		else
		{
			wiLua::SError(L, "LoadModel(string directory, string name, opt string identifier) not enough arguments!");
		}
		return 0;
	}
	int FinishLoading(lua_State* L)
	{
		wiRenderer::FinishLoading();
		return 0;
	}
};
