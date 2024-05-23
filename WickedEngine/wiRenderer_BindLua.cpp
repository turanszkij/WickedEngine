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
#include "wiVoxelGrid_BindLua.h"
#include "wiPathQuery_BindLua.h"
#include "wiTrailRenderer_BindLua.h"

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
		if (argc > 0)
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
		if (argc > 0)
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
	int SetDebugEnvProbesEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugEnvProbes(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetDebugEnvProbesEnabled(bool enabled) not enough arguments!");
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
		else
		{
			wi::lua::SError(L, "SetDebugForceFieldsEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCamerasEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugCameras(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetDebugCamerasEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCollidersEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawDebugColliders(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetDebugCollidersEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetGridHelperEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetToDrawGridHelper(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetGridHelperEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDDGIDebugEnabled(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			wi::renderer::SetDDGIDebugEnabled(wi::lua::SGetBool(L, 1));
		}
		else
		{
			wi::lua::SError(L, "SetDDGIDebugEnabled(bool enabled) not enough arguments!");
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
				bool depth = false;
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
					{
						XMStoreFloat4(&line.color_start, XMLoadFloat4(&c->data));
						XMStoreFloat4(&line.color_end, XMLoadFloat4(&c->data));
					}
					else
						wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");

					if (argc > 3)
					{
						depth = wi::lua::SGetBool(L, 4);
					}
				}
				wi::renderer::DrawLine(line, depth);
			}
			else
				wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");
		}
		else
			wi::lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) not enough arguments!");

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
				bool depth = false;
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

						if (argc > 3)
						{
							depth = wi::lua::SGetBool(L, 4);
						}
					}
				}
				wi::renderer::DrawPoint(point, depth);
			}
			else
				wi::lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) first argument must be a Vector type!");
		}
		else
			wi::lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) not enough arguments!");

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
						bool depth = true;
						if (argc > 2)
						{
							depth = wi::lua::SGetBool(L, 3);
						}
						wi::renderer::DrawBox(m->data, color->data, depth);
						return 0;
					}
				}

				wi::renderer::DrawBox(m->data);
			}
			else
				wi::lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) not enough arguments!");

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
						bool depth = true;
						if (argc > 2)
						{
							depth = wi::lua::SGetBool(L, 3);
						}
						wi::renderer::DrawSphere(sphere->sphere, color->data, depth);
						return 0;
					}
				}

				wi::renderer::DrawSphere(sphere->sphere);
			}
			else
				wi::lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) not enough arguments!");

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
						bool depth = true;
						if (argc > 2)
						{
							depth = wi::lua::SGetBool(L, 3);
						}
						wi::renderer::DrawCapsule(capsule->capsule, color->data, depth);
						return 0;
					}
				}

				wi::renderer::DrawCapsule(capsule->capsule);
			}
			else
				wi::lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			wi::lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) not enough arguments!");

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
	int DrawVoxelGrid(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			VoxelGrid_BindLua* a = Luna<VoxelGrid_BindLua>::lightcheck(L, 1);
			if (a)
			{
				wi::renderer::DrawVoxelGrid(a->voxelgrid);
			}
			else
				wi::lua::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) first argument must be a VoxelGrid type!");
		}
		else
			wi::lua::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) not enough arguments!");

		return 0;
	}
	int DrawPathQuery(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			PathQuery_BindLua* a = Luna<PathQuery_BindLua>::lightcheck(L, 1);
			if (a)
			{
				wi::renderer::DrawPathQuery(&a->pathquery);
			}
			else
				wi::lua::SError(L, "DrawPathQuery(PathQuery pathquery) first argument must be a PathQuery type!");
		}
		else
			wi::lua::SError(L, "DrawPathQuery(PathQuery pathquery) not enough arguments!");

		return 0;
	}
	int DrawTrail(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			TrailRenderer_BindLua* a = Luna<TrailRenderer_BindLua>::lightcheck(L, 1);
			if (a)
			{
				wi::renderer::DrawTrail(&a->trail);
			}
			else
				wi::lua::SError(L, "DrawTrail(TrailRenderer trail) first argument must be a TrailRenderer type!");
		}
		else
			wi::lua::SError(L, "DrawTrail(TrailRenderer trail) not enough arguments!");

		return 0;
	}

	class PaintTextureParams_BindLua
	{
	public:
		wi::renderer::PaintTextureParams params;

		PaintTextureParams_BindLua(const wi::renderer::PaintTextureParams& params) : params(params) {}
		PaintTextureParams_BindLua(lua_State* L) {}

		int SetEditTexture(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetEditTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				wi::lua::SError(L, "SetEditTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.editTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetBrushTexture(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				wi::lua::SError(L, "SetBrushTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.brushTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetRevealTexture(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetRevealTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				wi::lua::SError(L, "SetRevealTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.revealTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetCenterPixel(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetCenterPixel(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetCenterPixel(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushCenter.x = uint32_t(vec->data.x);
			params.push.xPaintBrushCenter.y = uint32_t(vec->data.y);
			return 0;
		}
		int SetBrushColor(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushColor(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				wi::lua::SError(L, "SetBrushColor(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushColor = wi::Color::fromFloat4(vec->data);
			return 0;
		}
		int SetBrushRadius(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushRadius(int value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRadius = wi::lua::SGetInt(L, 1);
			return 0;
		}
		int SetBrushAmount(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushAmount(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushAmount = wi::lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushSmoothness(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushSmoothness(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushSmoothness = wi::lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushRotation(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushRotation(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRotation = wi::lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushShape(lua_State* L)
		{
			int argc = wi::lua::SGetArgCount(L);
			if (argc < 1)
			{
				wi::lua::SError(L, "SetBrushShape(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushShape = wi::lua::SGetInt(L, 1);
			return 0;
		}

		inline static constexpr char className[] = "PaintTextureParams";
		inline static constexpr Luna<PaintTextureParams_BindLua>::FunctionType methods[] = {
			lunamethod(PaintTextureParams_BindLua, SetEditTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushTexture),
			lunamethod(PaintTextureParams_BindLua, SetRevealTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushColor),
			lunamethod(PaintTextureParams_BindLua, SetCenterPixel),
			lunamethod(PaintTextureParams_BindLua, SetBrushRadius),
			lunamethod(PaintTextureParams_BindLua, SetBrushAmount),
			lunamethod(PaintTextureParams_BindLua, SetBrushSmoothness),
			lunamethod(PaintTextureParams_BindLua, SetBrushRotation),
			lunamethod(PaintTextureParams_BindLua, SetBrushShape),
			{ nullptr, nullptr }
		};
		inline static constexpr Luna<PaintTextureParams_BindLua>::PropertyType properties[] = {
			{ nullptr, nullptr }
		};
	};

	int PaintIntoTexture(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 1)
		{
			wi::lua::SError(L, "PaintIntoTexture(PaintTextureParams params): not enough arguments!");
			return 0;
		}
		PaintTextureParams_BindLua* params = Luna<PaintTextureParams_BindLua>::lightcheck(L, 1);
		if (params == nullptr)
		{
			wi::lua::SError(L, "PaintIntoTexture(PaintTextureParams params): argument is not a PaintTextureParams!");
			return 0;
		}
		wi::renderer::PaintIntoTexture(params->params);
		return 0;
	}
	int CreatePaintableTexture(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 2)
		{
			wi::lua::SError(L, "CreatePaintableTexture(int width,height, opt int mips = 0, opt Vector initialColor = Vector()): not enough arguments!");
			return 0;
		}
		uint32_t width = (uint32_t)wi::lua::SGetInt(L, 1);
		uint32_t height = (uint32_t)wi::lua::SGetInt(L, 2);
		uint32_t mips = 0;
		wi::Color color = wi::Color::Transparent();
		if (argc > 2)
		{
			mips = (uint32_t)wi::lua::SGetInt(L, 3);
			if (argc > 3)
			{
				Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (v != nullptr)
				{
					color = wi::Color::fromFloat4(v->data);
				}
			}
		}
		Luna<Texture_BindLua>::push(L, wi::renderer::CreatePaintableTexture(width, height, mips, color));
		return 1;
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

			Luna<PaintTextureParams_BindLua>::Register(wi::lua::GetLuaState());

			wi::lua::RegisterFunc("SetGamma", SetGamma);
			wi::lua::RegisterFunc("SetGameSpeed", SetGameSpeed);
			wi::lua::RegisterFunc("GetGameSpeed", GetGameSpeed);

			wi::lua::RegisterFunc("SetShadowProps2D", SetShadowProps2D);
			wi::lua::RegisterFunc("SetShadowPropsCube", SetShadowPropsCube);
			wi::lua::RegisterFunc("SetDebugBoxesEnabled", SetDebugBoxesEnabled);
			wi::lua::RegisterFunc("SetDebugPartitionTreeEnabled", SetDebugPartitionTreeEnabled);
			wi::lua::RegisterFunc("SetDebugBonesEnabled", SetDebugBonesEnabled);
			wi::lua::RegisterFunc("SetDebugEmittersEnabled", SetDebugEmittersEnabled);
			wi::lua::RegisterFunc("SetDebugEnvProbesEnabled", SetDebugEnvProbesEnabled);
			wi::lua::RegisterFunc("SetDebugForceFieldsEnabled", SetDebugForceFieldsEnabled);
			wi::lua::RegisterFunc("SetDebugCamerasEnabled", SetDebugCamerasEnabled);
			wi::lua::RegisterFunc("SetDebugCollidersEnabled", SetDebugCollidersEnabled);
			wi::lua::RegisterFunc("SetGridHelperEnabled", SetGridHelperEnabled);
			wi::lua::RegisterFunc("SetDDGIDebugEnabled", SetDDGIDebugEnabled);
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
			wi::lua::RegisterFunc("DrawVoxelGrid", DrawVoxelGrid);
			wi::lua::RegisterFunc("DrawPathQuery", DrawPathQuery);
			wi::lua::RegisterFunc("DrawTrail", DrawTrail);

			wi::lua::RegisterFunc("PaintIntoTexture", PaintIntoTexture);
			wi::lua::RegisterFunc("CreatePaintableTexture", CreatePaintableTexture);

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
