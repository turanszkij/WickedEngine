#include "wiRenderer_BindLua.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiScene.h"
#include "wiScene_BindLua.h"
#include "wiMath_BindLua.h"
#include "wiTexture_BindLua.h"
#include "wiEmittedParticle.h"
#include "wiHairParticle.h"
#include "wiPrimitive_BindLua.h"
#include "wiEventHandler.h"

using namespace wi::ecs;
using namespace wi::graphics;
using namespace wi::scene;
using namespace wi::lua::scene;
using namespace wi::lua::primitive;

namespace wi::lua::renderer
{
	int SetGamma(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::lua::SSetString(L, "SetGamma() no longer supported!");
		}
		else
		{
			wi::lua::SError(L, "SetGamma(float) not enough arguments!");
		}
		return 0;
	}
	int SetGameSpeed(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetGameSpeed(wi::lua::SGetFloat(L, 1));
		}
		else
		{
			wi::lua::SError(L,"SetGameSpeed(float) not enough arguments!");
		}
		return 0;
	}
	int GetGameSpeed(lua_State* L)
	{
		wi::lua::SSetFloat(L, wi::renderer::GetGameSpeed());
		return 1;
	}

	int SetShadowProps2D(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			wi::renderer::SetShadowProps2D(wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetShadowProps2D(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetShadowPropsCube(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			wi::renderer::SetShadowPropsCube(wi::lua::SGetInt(L, 1));
		}
		else
			wi::lua::SError(L, "SetShadowPropsCube(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetDebugPartitionTreeEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugPartitionTree(wi::lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugBoxesEnabled(lua_State* L)
	{
		wi::lua::SError(L, "SetDebugBoxesEnabled is obsolete! Use SetDebugPartitionTreeEnabled(bool value) instead to draw a partition tree!");
		return 0;
	}
	int SetDebugBonesEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugBoneLines(wi::lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEmittersEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugEmitters(wi::lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugForceFieldsEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugForceFields(wi::lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetVSyncEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::eventhandler::SetVSync(wi::lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetResolution(lua_State* L)
	{
		wi::lua::SError(L, "SetResolution() is deprecated, now it's handled by window events!");
		return 0;
	}
	int SetDebugLightCulling(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetDebugLightCulling(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetDebugLightCulling(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetOcclusionCullingEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetOcclusionCullingEnabled(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetOcclusionCullingEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}

	int DrawLine(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (a && b)
			{
				wi::renderer::RenderableLine line;
				XMStoreFloat3(&line.start, XMLoadFloat4(&a->data));
				XMStoreFloat3(&line.end, XMLoadFloat4(&b->data));
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
					{
						XMStoreFloat4(&line.color_start, XMLoadFloat4(&c->data));
						XMStoreFloat4(&line.color_end, XMLoadFloat4(&c->data));
					}
					else
						wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
				}
				wi::renderer::DrawLine(line);
			}
			else
				wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color) one or more arguments are not vectors!");
		}
		else
			wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawPoint(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (a)
			{
				wi::renderer::RenderablePoint point;
				XMStoreFloat3(&point.position, XMLoadFloat4(&a->data));
				if (argc > 1)
				{
					point.size = wi::lua::SGetFloat(L, 2);

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color)
						{
							point.color = color->data;
						}
					}
				}
				wi::renderer::DrawPoint(point);
			}
			else
				wi::lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color) first argument must be a Vector type!");
		}
		else
			wi::lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawBox(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						wi::renderer::DrawBox(m->data, color->data);
						return 0;
					}
				}

				wi::renderer::DrawBox(m->data);
			}
			else
				wi::lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawSphere(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
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
						wi::renderer::DrawSphere(sphere->sphere, color->data);
						return 0;
					}
				}

				wi::renderer::DrawSphere(sphere->sphere);
			}
			else
				wi::lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawCapsule(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
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
						wi::renderer::DrawCapsule(capsule->capsule, color->data);
						return 0;
					}
				}

				wi::renderer::DrawCapsule(capsule->capsule);
			}
			else
				wi::lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color) not enough arguments!");

		return 0;
	}
	int DrawDebugText(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string text = wi::lua::SGetString(L, 1);
			wi::renderer::DebugTextParams params;
			if (argc > 1)
			{
				Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 2);
				if (position != nullptr)
				{
					params.position.x = position->data.x;
					params.position.y = position->data.y;
					params.position.z = position->data.z;

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color != nullptr)
						{
							params.color = color->data;

							if (argc > 3)
							{
								params.scaling = wi::lua::SGetFloat(L, 4);

								if (argc > 4)
								{
									params.flags = wi::lua::SGetInt(L, 5);
								}
							}
						}
						else
							wi::lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) third argument was not a Vector!");
					}
				}
				else
					wi::lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) second argument was not a Vector!");

			}
			wi::renderer::DrawDebugText(text.c_str(), params);
		}
		else
			wi::lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) not enough arguments!");

		return 0;
	}
	int PutWaterRipple(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 1)
		{
			std::string name = wi::lua::SGetString(L, 1);
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(name, v->GetFloat3());
			}
			else
				wi::lua::SError(L, "PutWaterRipple(string imagename, Vector position) argument is not a Vector!");
		}
		else if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(v->GetFloat3());
			}
			else
				wi::lua::SError(L, "PutWaterRipple(Vector position) argument is not a Vector!");
		}
		else
		{
			wi::lua::SError(L, "PutWaterRipple(Vector position) not enough arguments!");
			wi::lua::SError(L, "PutWaterRipple(string imagename, Vector position) not enough arguments!");
		}
		return 0;
	}

	int ClearWorld(lua_State* L)
	{
		Scene_BindLua* scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (scene == nullptr)
		{
			wi::renderer::ClearWorld(*GetGlobalScene());
		}
		else
		{
			wi::renderer::ClearWorld(*scene->scene);
		}
		return 0;
	}
	int ReloadShaders(lua_State* L)
	{
		wi::renderer::ReloadShaders();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			wi::lua::RegisterFunc("SetGamma", SetGamma);
			wi::lua::RegisterFunc("SetGameSpeed", SetGameSpeed);
			wi::lua::RegisterFunc("GetGameSpeed", GetGameSpeed);

			wi::lua::RegisterFunc("SetShadowProps2D", SetShadowProps2D);
			wi::lua::RegisterFunc("SetShadowPropsCube", SetShadowPropsCube);
			wi::lua::RegisterFunc("SetDebugBoxesEnabled", SetDebugBoxesEnabled);
			wi::lua::RegisterFunc("SetDebugPartitionTreeEnabled", SetDebugPartitionTreeEnabled);
			wi::lua::RegisterFunc("SetDebugBonesEnabled", SetDebugBonesEnabled);
			wi::lua::RegisterFunc("SetDebugEmittersEnabled", SetDebugEmittersEnabled);
			wi::lua::RegisterFunc("SetDebugForceFieldsEnabled", SetDebugForceFieldsEnabled);
			wi::lua::RegisterFunc("SetVSyncEnabled", SetVSyncEnabled);
			wi::lua::RegisterFunc("SetResolution", SetResolution);
			wi::lua::RegisterFunc("SetDebugLightCulling", SetDebugLightCulling);
			wi::lua::RegisterFunc("SetOcclusionCullingEnabled", SetOcclusionCullingEnabled);

			wi::lua::RegisterFunc("DrawLine", DrawLine);
			wi::lua::RegisterFunc("DrawPoint", DrawPoint);
			wi::lua::RegisterFunc("DrawBox", DrawBox);
			wi::lua::RegisterFunc("DrawSphere", DrawSphere);
			wi::lua::RegisterFunc("DrawCapsule", DrawCapsule);
			wi::lua::RegisterFunc("DrawDebugText", DrawDebugText);
			wi::lua::RegisterFunc("PutWaterRipple", PutWaterRipple);

			wi::lua::RegisterFunc("ClearWorld", ClearWorld);
			wi::lua::RegisterFunc("ReloadShaders", ReloadShaders);

			wi::lua::RunText(R"(
GetScreenWidth = function() return main.GetCanvas().GetLogicalWidth() end
GetScreenHeight = function() return main.GetCanvas().GetLogicalHeight() end

DEBUG_TEXT_DEPTH_TEST = 1
DEBUG_TEXT_CAMERA_FACING = 2
DEBUG_TEXT_CAMERA_SCALING = 4
)");

		}
	}
};
