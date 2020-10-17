#include "wiRenderer_BindLua.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiScene.h"
#include "wiScene_BindLua.h"
#include "Vector_BindLua.h"
#include "Matrix_BindLua.h"
#include "Texture_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiIntersect_BindLua.h"

using namespace std;
using namespace wiECS;
using namespace wiGraphics;
using namespace wiScene;
using namespace wiScene_BindLua;
using namespace wiIntersect_BindLua;

namespace wiRenderer_BindLua
{
	int SetResolutionScale(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetResolutionScale(wiLua::SGetFloat(L, 1));
		}
		else
		{
			wiLua::SError(L, "SetResolutionScale(float) not enough arguments!");
		}
		return 0;
	}
	int SetGamma(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetGamma(wiLua::SGetFloat(L, 1));
		}
		else
		{
			wiLua::SError(L, "SetGamma(float) not enough arguments!");
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
	int GetGameSpeed(lua_State* L)
	{
		wiLua::SSetFloat(L, wiRenderer::GetGameSpeed());
		return 1;
	}

	int GetScreenWidth(lua_State* L)
	{
		wiLua::SSetFloat(L, wiRenderer::GetDevice()->GetScreenWidth());
		return 1;
	}
	int GetScreenHeight(lua_State* L)
	{
		wiLua::SSetFloat(L, wiRenderer::GetDevice()->GetScreenHeight());
		return 1;
	}

	int GetCamera(lua_State* L)
	{
		Luna<CameraComponent_BindLua>::push(L, new CameraComponent_BindLua(&wiRenderer::GetCamera()));
		return 1;
	}
	int AttachCamera(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Entity entity = (Entity)wiLua::SGetLongLong(L, 1);
			wiRenderer::AttachCamera(entity);
		}
		else
		{
			wiLua::SError(L, "AttachCamera(Entity entity) not enough arguments!");
		}
		return 0;
	}

	int SetShadowProps2D(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			wiRenderer::SetShadowProps2D(wiLua::SGetInt(L, 1), wiLua::SGetInt(L, 2), wiLua::SGetInt(L, 3));
		}
		else
			wiLua::SError(L, "SetShadowProps2D(int resolution, int count, int softShadowQuality) not enough arguments!");
		return 0;
	}
	int SetShadowPropsCube(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			wiRenderer::SetShadowPropsCube(wiLua::SGetInt(L, 1), wiLua::SGetInt(L, 2));
		}
		else
			wiLua::SError(L, "SetShadowPropsCube(int resolution, int count) not enough arguments!");
		return 0;
	}
	int SetDebugPartitionTreeEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetToDrawDebugPartitionTree(wiLua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugBoxesEnabled(lua_State* L)
	{
		wiLua::SError(L, "SetDebugBoxesEnabled is obsolete! Use SetDebugPartitionTreeEnabled(bool value) instead to draw a partition tree!");
		return 0;
	}
	int SetDebugBonesEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetToDrawDebugBoneLines(wiLua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEmittersEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetToDrawDebugEmitters(wiLua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugForceFieldsEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetToDrawDebugForceFields(wiLua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetVSyncEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::GetDevice()->SetVSyncEnabled(wiLua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetResolution(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			wiRenderer::GetDevice()->SetResolution(wiLua::SGetInt(L, 1), wiLua::SGetInt(L, 2));
		}
		else
		{
			wiLua::SError(L, "SetResolution(int width,height) not enough arguments!");
		}
		return 0;
	}
	int SetDebugLightCulling(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetDebugLightCulling(wiLua::SGetBool(L, 1));
		}
		else
		{
			wiLua::SError(L, "SetDebugLightCulling(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetOcclusionCullingEnabled(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			wiRenderer::SetOcclusionCullingEnabled(wiLua::SGetBool(L, 1));
		}
		else
		{
			wiLua::SError(L, "SetOcclusionCullingEnabled(bool enabled) not enough arguments!");
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
				wiRenderer::RenderableLine line;
				XMStoreFloat3(&line.start, a->vector);
				XMStoreFloat3(&line.end, b->vector);
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
					{
						XMStoreFloat4(&line.color_start, c->vector);
						XMStoreFloat4(&line.color_end, c->vector);
					}
					else
						wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
				}
				wiRenderer::DrawLine(line);
			}
			else
				wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
		}
		else
			wiLua::SError(L, "DrawLine(Vector origin,end, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawPoint(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (a)
			{
				wiRenderer::RenderablePoint point;
				XMStoreFloat3(&point.position, a->vector);
				if (argc > 1)
				{
					point.size = wiLua::SGetFloat(L, 2);

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						XMStoreFloat4(&point.color, color->vector);
					}
				}
				wiRenderer::DrawPoint(point);
			}
			else
				wiLua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color) first argument must be a Vector type!");
		}
		else
			wiLua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawBox(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m)
			{
				XMFLOAT4X4 mat;
				XMStoreFloat4x4(&mat, m->matrix);

				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						XMFLOAT4 col;
						XMStoreFloat4(&col, color->vector);
						wiRenderer::DrawBox(mat, col);
						return 0;
					}
				}

				wiRenderer::DrawBox(mat);
			}
			else
				wiLua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wiLua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawSphere(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						XMFLOAT4 col;
						XMStoreFloat4(&col, color->vector);
						wiRenderer::DrawSphere(sphere->sphere, col);
						return 0;
					}
				}

				wiRenderer::DrawSphere(sphere->sphere);
			}
			else
				wiLua::SError(L, "DrawSphere(Sphere sphere, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wiLua::SError(L, "DrawSphere(Sphere sphere, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawCapsule(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 0)
		{
			Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (capsule)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						XMFLOAT4 col;
						XMStoreFloat4(&col, color->vector);
						wiRenderer::DrawCapsule(capsule->capsule, col);
						return 0;
					}
				}

				wiRenderer::DrawCapsule(capsule->capsule);
			}
			else
				wiLua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wiLua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color) not enough arguments!");

		return 0;
	}
	int PutWaterRipple(lua_State* L)
	{
		int argc = wiLua::SGetArgCount(L);
		if (argc > 1)
		{
			string name = wiLua::SGetString(L, 1);
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v)
			{
				XMFLOAT3 pos;
				XMStoreFloat3(&pos, v->vector);
				wiRenderer::PutWaterRipple(name, pos);
			}
			else
				wiLua::SError(L, "PutWaterRipple(String imagename, Vector position) argument is not a Vector!");
		}
		else
			wiLua::SError(L, "PutWaterRipple(String imagename, Vector position) not enough arguments!");
		return 0;
	}

	int ClearWorld(lua_State* L)
	{
		wiRenderer::ClearWorld();
		return 0;
	}
	int ReloadShaders(lua_State* L)
	{
		wiRenderer::ReloadShaders();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			wiLua::RegisterFunc("SetResolutionScale", SetResolutionScale);
			wiLua::RegisterFunc("SetGamma", SetGamma);
			wiLua::RegisterFunc("SetGameSpeed", SetGameSpeed);
			wiLua::RegisterFunc("GetGameSpeed", GetGameSpeed);

			wiLua::RegisterFunc("GetScreenWidth", GetScreenWidth);
			wiLua::RegisterFunc("GetScreenHeight", GetScreenHeight);

			wiLua::RegisterFunc("GetCamera", GetCamera);
			wiLua::RegisterFunc("AttachCamera", AttachCamera);

			wiLua::RegisterFunc("SetShadowProps2D", SetShadowProps2D);
			wiLua::RegisterFunc("SetShadowPropsCube", SetShadowPropsCube);
			wiLua::RegisterFunc("SetDebugBoxesEnabled", SetDebugBoxesEnabled);
			wiLua::RegisterFunc("SetDebugPartitionTreeEnabled", SetDebugPartitionTreeEnabled);
			wiLua::RegisterFunc("SetDebugBonesEnabled", SetDebugBonesEnabled);
			wiLua::RegisterFunc("SetDebugEmittersEnabled", SetDebugEmittersEnabled);
			wiLua::RegisterFunc("SetDebugForceFieldsEnabled", SetDebugForceFieldsEnabled);
			wiLua::RegisterFunc("SetVSyncEnabled", SetVSyncEnabled);
			wiLua::RegisterFunc("SetResolution", SetResolution);
			wiLua::RegisterFunc("SetDebugLightCulling", SetDebugLightCulling);
			wiLua::RegisterFunc("SetOcclusionCullingEnabled", SetOcclusionCullingEnabled);

			wiLua::RegisterFunc("DrawLine", DrawLine);
			wiLua::RegisterFunc("DrawPoint", DrawPoint);
			wiLua::RegisterFunc("DrawBox", DrawBox);
			wiLua::RegisterFunc("DrawSphere", DrawSphere);
			wiLua::RegisterFunc("DrawCapsule", DrawCapsule);
			wiLua::RegisterFunc("PutWaterRipple", PutWaterRipple);


			wiLua::RunText("PICK_VOID = 0");
			wiLua::RunText("PICK_OPAQUE = 1");
			wiLua::RunText("PICK_TRANSPARENT = 2");
			wiLua::RunText("PICK_WATER = 4");


			wiLua::RegisterFunc("ClearWorld", ClearWorld);
			wiLua::RegisterFunc("ReloadShaders", ReloadShaders);
		}
	}
};
