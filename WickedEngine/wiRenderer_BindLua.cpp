#include "wiRenderer_BindLua.h"
#include "wiRenderer.h"
#include "wiLines.h"
#include "wiLoader.h"
#include "wiHelper.h"
#include "wiLoader_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"

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
			wiLua::GetGlobal()->RegisterFunc("Pick", Pick);
			wiLua::GetGlobal()->RegisterFunc("DrawLine", DrawLine);
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
			XMMATRIX transform = XMMatrixIdentity();
			if (argc > 2)
			{
				identifier = wiLua::SGetString(L, 3);
				if (argc > 3)
				{
					Matrix_BindLua* matrix = Luna<Matrix_BindLua>::lightcheck(L, 4);
					if (matrix != nullptr)
					{
						transform = matrix->matrix;
					}
					else
					{
						wiLua::SError(L, "LoadModel(string directory, string name, opt string identifier, opt Matrix transform) argument is not a matrix!");
					}
				}
			}
			wiRenderer::LoadModel(dir, name, XMMatrixIdentity(), identifier, wiRenderer::physicsEngine);
		}
		else
		{
			wiLua::SError(L, "LoadModel(string directory, string name, opt string identifier, opt Matrix transform) not enough arguments!");
		}
		return 0;
	}
	int FinishLoading(lua_State* L)
	{
		wiRenderer::FinishLoading();
		return 0;
	}

	int Pick(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Ray_BindLua* ray = Luna<Ray_BindLua>::lightcheck(L, 1);
			if (ray != nullptr)
			{
				wiRenderer::Picked pick = wiRenderer::Pick(ray->ray, wiRenderer::PICKTYPE::PICK_OPAQUE);
				Luna<Object_BindLua>::push(L, new Object_BindLua(pick.object));
				Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.position)));
				Luna<Vector_BindLua>::push(L, new Vector_BindLua(XMLoadFloat3(&pick.normal)));
				return 3;
			}
		}
		return 0;
	}

	int DrawLine(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (a && b)
			{
				XMFLOAT3 xa, xb;
				XMFLOAT4 xc = XMFLOAT4(1, 1, 1, 1);
				XMStoreFloat3(&xa, a->vector);
				XMStoreFloat3(&xb, b->vector);
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
						XMStoreFloat4(&xc, c->vector);
					else
						wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
				}
				wiRenderer::linesTemp.push_back(new Lines(xa, xb, xc));
			}
			else
				wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
		}
		else
			wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) not enough arguments!");
		return 0;
	}
};
