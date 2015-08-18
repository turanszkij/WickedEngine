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
			wiLua::GetGlobal()->RegisterFunc("GetTransforms", GetTransforms);
			wiLua::GetGlobal()->RegisterFunc("GetTransform", GetTransform);
			wiLua::GetGlobal()->RegisterFunc("GetArmatures", GetArmatures);
			wiLua::GetGlobal()->RegisterFunc("GetArmature", GetArmature);
			wiLua::GetGlobal()->RegisterFunc("GetObjects", GetObjects);
			wiLua::GetGlobal()->RegisterFunc("GetObject", GetObjectLua);
			wiLua::GetGlobal()->RegisterFunc("GetMeshes", GetMeshes);
			wiLua::GetGlobal()->RegisterFunc("GetLights", GetLights);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);
			wiLua::GetGlobal()->RegisterFunc("GetGameSpeed", GetGameSpeed);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);
			wiLua::GetGlobal()->RegisterFunc("GetScreenWidth", GetScreenWidth);
			wiLua::GetGlobal()->RegisterFunc("GetScreenHeight", GetScreenHeight);
			wiLua::GetGlobal()->RegisterFunc("GetRenderWidth", GetRenderWidth);
			wiLua::GetGlobal()->RegisterFunc("GetRenderHeight", GetRenderHeight);

			wiLua::GetGlobal()->RegisterFunc("SetGameSpeed", SetGameSpeed);

			wiLua::GetGlobal()->RegisterFunc("LoadModel", LoadModel);
			wiLua::GetGlobal()->RegisterFunc("FinishLoading", FinishLoading);
		}
	}

	int GetTransforms(lua_State* L)
	{
		stringstream ss("");
		for (auto& x : wiRenderer::transforms)
		{
			ss << x.first << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetTransform(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			string name = wiLua::SGetString(L, 1);
			Transform* transform = wiRenderer::getTransformByName(name);
			if (transform != nullptr)
			{
				Object* object = dynamic_cast<Object*>(transform);
				if (object != nullptr)
				{
					Luna<Object_BindLua>::push(L, new Object_BindLua(object));
					return 1;
				}
				Armature* armature = dynamic_cast<Armature*>(transform);
				if (armature != nullptr)
				{
					Luna<Armature_BindLua>::push(L, new Armature_BindLua(armature));
					return 1;
				}

				Luna<Transform_BindLua>::push(L, new Transform_BindLua(transform));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetTransform(String name) transform not found!");
				return 0;
			}
		}
		return 0;
	}
	int GetArmatures(lua_State* L)
	{
		stringstream ss("");
		for (auto& x : wiRenderer::armatures)
		{
			ss << x->name << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetArmature(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			string name = wiLua::SGetString(L, 1);
			Armature* armature = wiRenderer::getArmatureByName(name);
			if (armature != nullptr)
			{
				Luna<Armature_BindLua>::push(L, new Armature_BindLua(armature));
				return 1;
			}
			else
			{
				wiLua::SError(L, "GetArmature(String name) armature not found!");
				return 0;
			}
		}
		return 0;
	}
	int GetObjects(lua_State* L)
	{
		stringstream ss("");
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
		for (auto& x : wiRenderer::meshes)
		{
			ss << x.first << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetLights(lua_State* L)
	{
		for (auto& x : wiRenderer::lights)
		{
			wiLua::SSetString(L, x->name);
		}
		return wiRenderer::lights.size();
	}
	int GetMaterials(lua_State* L)
	{
		stringstream ss("");
		for (auto& x : wiRenderer::materials)
		{
			ss << x.first << endl;
		}
		wiLua::SSetString(L, ss.str());
		return 1;
	}
	int GetGameSpeed(lua_State* L)
	{
		wiLua::SSetFloat(L, wiRenderer::GetGameSpeed());
		return 1;
	}
	int GetScreenWidth(lua_State* L)
	{
		wiLua::SSetInt(L, wiRenderer::SCREENWIDTH);
		return 1;
	}
	int GetScreenHeight(lua_State* L)
	{
		wiLua::SSetInt(L, wiRenderer::SCREENHEIGHT);
		return 1;
	}
	int GetRenderWidth(lua_State* L)
	{
		wiLua::SSetInt(L, wiRenderer::RENDERWIDTH);
		return 1;
	}
	int GetRenderHeight(lua_State* L)
	{
		wiLua::SSetInt(L, wiRenderer::RENDERHEIGHT);
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
