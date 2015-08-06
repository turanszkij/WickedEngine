#include "wiRenderer_BindLua.h"
#include "wiRenderer.h"
#include "wiLoader.h"
#include "wiHelper.h"

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
			wiLua::GetGlobal()->RegisterFunc("GetObjectProp", GetObjectProp);
			wiLua::GetGlobal()->RegisterFunc("GetMeshes", GetMeshes);
			wiLua::GetGlobal()->RegisterFunc("GetLights", GetLights);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);
			wiLua::GetGlobal()->RegisterFunc("GetGameSpeed", GetGameSpeed);
			wiLua::GetGlobal()->RegisterFunc("GetMaterials", GetMaterials);

			wiLua::GetGlobal()->RegisterFunc("SetObjectProp", SetObjectProp);
			wiLua::GetGlobal()->RegisterFunc("SetGameSpeed", SetGameSpeed);

			wiLua::GetGlobal()->RegisterFunc("LoadModel", LoadModel);
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
	int GetObjectProp(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			//name,property
			string name = wiLua::SGetString(L, 1);
			string prop = wiHelper::toUpper( wiLua::SGetString(L, 2) );
			Object* found = nullptr;
			for (auto& x : wiRenderer::objects)
			{
				if (!x->name.compare(name))
				{
					found = x;
					break;
				}
			}
			if (found == nullptr)
			{
				for (auto& x : wiRenderer::objects_trans)
				{
					if (!x->name.compare(name))
					{
						found = x;
						break;
					}
				}
				if (found == nullptr)
				{
					for (auto& x : wiRenderer::objects_water)
					{
						if (!x->name.compare(name))
						{
							found = x;
							break;
						}
					}
				}
			}
			if (found != nullptr)
			{
				if (prop.find("TRANSP")!=string::npos)
				{
					wiLua::SSetFloat(L, found->transparency);
					return 1;
				}
				else if (prop.find("COLOR")!=string::npos)
				{
					wiLua::SSetFloat3(L, found->color);
					return 3;
				}
				else
				{
					wiLua::SError(L, "GetObjectProp(string name,string property) no such property!");
					return 0;
				}
			}
			else
			{
				wiLua::SError(L, "GetObjectProp(string name,string property) no such object!");
				return 0;
			}
		}
		else
		{
			wiLua::SError(L, "GetObjectProp(string name,string property) not enough arguments!");
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


	int SetObjectProp(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			//name,property
			string name = wiLua::SGetString(L, 1);
			string prop = wiHelper::toUpper(wiLua::SGetString(L, 2));
			Object* found = nullptr;
			for (auto& x : wiRenderer::objects)
			{
				if (!x->name.compare(name))
				{
					found = x;
					break;
				}
			}
			if (found == nullptr)
			{
				for (auto& x : wiRenderer::objects_trans)
				{
					if (!x->name.compare(name))
					{
						found = x;
						break;
					}
				}
				if (found == nullptr)
				{
					for (auto& x : wiRenderer::objects_water)
					{
						if (!x->name.compare(name))
						{
							found = x;
							break;
						}
					}
				}
			}
			if (found != nullptr)
			{
				if (prop.find("TRANSP") != string::npos)
				{
					found->transparency = wiLua::SGetFloat(L, 3);
				}
				else if (prop.find("COLOR") != string::npos)
				{
					found->color = wiLua::SGetFloat3(L, 3);
				}
				else
				{
					wiLua::SError(L, "GetObjectProp(string name,string property) no such property!");
				}
			}
			else
			{
				wiLua::SError(L, "GetObjectProp(string name,string property) no such object!");
				return 0;
			}
		}
		else
		{
			wiLua::SError(L, "GetObjectProp(string name,string property) not enough arguments!");
		}
		return 0;
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
};
