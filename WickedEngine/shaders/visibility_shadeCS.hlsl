#define SURFACE_LOAD_QUAD_DERIVATIVES
#define SHADOW_MASK_ENABLED
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"
#include "objectHF.hlsli"

RWTexture2D<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	//uint2 pixel = DTid.xy;
	uint2 pixel = Gid.xy * 8 + remap_lane_8x8(groupIndex);

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	uint primitiveID = texture_primitiveID[pixel];
	if (any(primitiveID))
	{
		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		surface.init();
		surface.raycone = pixel_ray_cone_from_image_height(GetCamera().internal_resolution.y);

		[branch]
		if (surface.load(prim, ray.Origin, ray.Direction))
		{
			surface.pixel = pixel.xy;
			surface.screenUV = uv;

			float3 ambient = GetAmbient(surface.N);
			ambient = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));

			Lighting lighting;
			lighting.create(0, 0, ambient, 0);

			//LightMapping(surface.inst.lightmap, input.atl, lighting, surface);

			ApplyEmissive(surface, lighting);


#ifdef PLANARREFLECTION
			lighting.indirect.specular += PlanarReflection(surface, bumpColor.rg) * surface.F;
#endif

			TiledLighting(surface, lighting);

			[branch]
			if (GetCamera().texture_ssr_index >= 0)
			{
				float4 ssr = bindless_textures[GetCamera().texture_ssr_index].SampleLevel(sampler_linear_clamp, surface.screenUV, 0);
				lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
			}

#ifdef UNLIT
			lighting.direct.diffuse = 1;
			lighting.indirect.diffuse = 0;
			lighting.direct.specular = 0;
			lighting.indirect.specular = 0;
#endif // UNLIT

			float4 color = 0;

			ApplyLighting(surface, lighting, color);

			ApplyFog(surface.hit_depth, GetCamera().position, surface.V, color);

			output[pixel] = float4(color.rgb, 1);
			//output[pixel] = float4(surface.albedo, 1);
		}
	}
	else
	{
		// sky
	}
}
