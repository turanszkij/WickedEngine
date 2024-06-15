#include "wiTexture_BindLua.h"
#include "wiTextureHelper.h"
#include "wiMath_BindLua.h"
#include "wiHelper.h"

using namespace wi::graphics;

namespace wi::lua
{

	Luna<Texture_BindLua>::FunctionType Texture_BindLua::methods[] = {
		lunamethod(Texture_BindLua, GetLogo),
		lunamethod(Texture_BindLua, CreateGradientTexture),
		lunamethod(Texture_BindLua, CreateLensDistortionNormalMap),
		lunamethod(Texture_BindLua, Save),
		{ NULL, NULL }
	};
	Luna<Texture_BindLua>::PropertyType Texture_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Texture_BindLua::Texture_BindLua(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string name = wi::lua::SGetString(L, 1);
			resource = wi::resourcemanager::Load(name);
		}
	}

	int Texture_BindLua::GetLogo(lua_State* L)
	{
		Luna<Texture_BindLua>::push(L, *wi::texturehelper::getLogo());
		return 1;
	}
	int Texture_BindLua::CreateGradientTexture(lua_State* L)
	{
		wi::texturehelper::GradientType gradient_type = wi::texturehelper::GradientType::Linear;
		uint32_t width = 256;
		uint32_t height = 256;
		XMFLOAT2 uv_start = XMFLOAT2(0, 0);
		XMFLOAT2 uv_end = XMFLOAT2(0, 0);
		wi::texturehelper::GradientFlags gradient_flags = wi::texturehelper::GradientFlags::None;
		std::string swizzle_string;
		float perlin_scale = 1;
		uint32_t perlin_seed = 1234u;
		int perlin_octaves = 8;
		float perlin_persistence = 0.5f;
		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			gradient_type = (wi::texturehelper::GradientType)wi::lua::SGetInt(L, 1);
			if (argc > 1)
			{
				width = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					height = (uint32_t)wi::lua::SGetInt(L, 3);
					if (argc > 3)
					{
						Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 4);
						if (v1)
						{
							uv_start = v1->GetFloat2();
						}
						else
						{
							wi::lua::SError(L, "CreateGradientTexture() uv_start argument is not a vector!");
						}
						if (argc > 4)
						{
							Vector_BindLua* v2 = Luna<Vector_BindLua>::lightcheck(L, 5);
							if (v2)
							{
								uv_end = v2->GetFloat2();
							}
							else
							{
								wi::lua::SError(L, "CreateGradientTexture() uv_end argument is not a vector!");
							}
							if (argc > 5)
							{
								gradient_flags = (wi::texturehelper::GradientFlags)wi::lua::SGetInt(L, 6);
								if (argc > 6)
								{
									swizzle_string = wi::lua::SGetString(L, 7);
									if (argc > 7)
									{
										perlin_scale = wi::lua::SGetFloat(L, 8);
										if (argc > 8)
										{
											perlin_seed = (uint32_t)wi::lua::SGetInt(L, 9);
											if (argc > 9)
											{
												perlin_octaves = wi::lua::SGetInt(L, 10);
												if (argc > 10)
												{
													perlin_persistence = wi::lua::SGetFloat(L, 11);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		wi::graphics::Texture texture = wi::texturehelper::CreateGradientTexture(
			gradient_type,
			width, height,
			uv_start, uv_end,
			gradient_flags,
			wi::graphics::SwizzleFromString(swizzle_string.c_str()),
			perlin_scale,
			perlin_seed,
			perlin_octaves,
			perlin_persistence
		);
		Luna<Texture_BindLua>::push(L, texture);
		return 1;
	}
	int Texture_BindLua::CreateLensDistortionNormalMap(lua_State* L)
	{
		uint32_t width = 256;
		uint32_t height = 256;
		XMFLOAT2 uv_start = XMFLOAT2(0.5f, 0.5f);
		float radius = 0.5f;
		float squish = 1;
		float blend = 1;
		float edge_smoothness = 0.04f;

		int argc = wi::lua::SGetArgCount(L);
		if (argc > 0)
		{
			width = (uint32_t)wi::lua::SGetInt(L, 1);
			if (argc > 1)
			{
				height = (uint32_t)wi::lua::SGetInt(L, 2);
				if (argc > 2)
				{
					Vector_BindLua* v1 = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (v1)
					{
						uv_start = v1->GetFloat2();
					}
					else
					{
						wi::lua::SError(L, "CreateLensDistortionNormalMap() uv_start argument is not a vector!");
					}

					if (argc > 3)
					{
						radius = wi::lua::SGetFloat(L, 4);
						if (argc > 4)
						{
							squish = wi::lua::SGetFloat(L, 5);
							if (argc > 5)
							{
								blend = wi::lua::SGetFloat(L, 6);
								if (argc > 6)
								{
									edge_smoothness = wi::lua::SGetFloat(L, 7);
								}
							}
						}
					}
				}
			}
		}

		wi::graphics::Texture texture = wi::texturehelper::CreateLensDistortionNormalMap(
			width, height,
			uv_start,
			radius,
			squish,
			blend,
			edge_smoothness
		);
		Luna<Texture_BindLua>::push(L, texture);
		return 1;
	}
	int Texture_BindLua::Save(lua_State* L)
	{
		if (wi::lua::SGetArgCount(L) == 0)
		{
			wi::lua::SError(L, "Texture::Save(string filename) filename parameter is not provided!");
			return 0;
		}
		if (!resource.IsValid() || !resource.GetTexture().IsValid())
		{
			wi::lua::SError(L, "Texture::Save(string filename) texture is invalid!");
			return 0;
		}
		wi::helper::saveTextureToFile(resource.GetTexture(), wi::lua::SGetString(L, 1));
		return 0;
	}

	void Texture_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Texture_BindLua>::Register(wi::lua::GetLuaState());
			Luna<Texture_BindLua>::push_global(wi::lua::GetLuaState(), "texturehelper");
			wi::lua::RunText(R"(
GradientType = {
	Linear = 0,
	Circular = 1,
	Angular = 2,
}
GradientFlags = {
	None = 0,
	Inverse = 1 << 0,	
	Smoothstep = 1 << 1,
	PerlinNoise = 1 << 2,
	R16Unorm = 1 << 3,
}
)");
		}
	}

}
