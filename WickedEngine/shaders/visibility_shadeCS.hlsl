#define SURFACE_LOAD_QUAD_DERIVATIVES
#define SHADOW_MASK_ENABLED
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"
#include "objectHF.hlsli"

ConstantBuffer<ShaderTypeBin> bin : register(b10);
StructuredBuffer<uint> binned_pixels : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(64,	1, 1)]
void main(uint DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID)
{
	if (DTid.x >= bin.count)
		return;

	uint2 pixel = unpack_pixel(binned_pixels[bin.offset + DTid.x]);
	uint primitiveID = texture_primitiveID[pixel];
	if (!any(primitiveID))
	{
		return;
	}

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);

	float3 rayDirection_quad_x = CreateCameraRay(uv_to_clipspace(((float2)pixel + float2(1, 0) + 0.5) * GetCamera().internal_resolution_rcp)).Direction;
	float3 rayDirection_quad_y = CreateCameraRay(uv_to_clipspace(((float2)pixel + float2(0, 1) + 0.5) * GetCamera().internal_resolution_rcp)).Direction;

	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();

	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction, rayDirection_quad_x, rayDirection_quad_y))
	{
		return;
	}

	surface.pixel = pixel.xy;
	surface.screenUV = uv;

	if ((surface.flags & SURFACE_FLAG_GI_APPLIED) == 0)
	{
		float3 ambient = GetAmbient(surface.N);
		surface.gi = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));
	}

	Lighting lighting;
	lighting.create(0, 0, surface.gi, 0);

	ApplyEmissive(surface, lighting);


#ifdef PLANARREFLECTION
	lighting.indirect.specular += PlanarReflection(surface, surface.bumpColor.rg) * surface.F;
#endif

	TiledLighting(surface, lighting);

	[branch]
	if (GetCamera().texture_ssr_index >= 0)
	{
		float4 ssr = bindless_textures[GetCamera().texture_ssr_index].SampleLevel(sampler_linear_clamp, surface.screenUV, 0);
		lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
	}
	[branch]
	if (GetCamera().texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_float[GetCamera().texture_ao_index].SampleLevel(sampler_linear_clamp, surface.screenUV, 0).r;
	}

	float4 color = 0;

	ApplyLighting(surface, lighting, color);

#ifdef UNLIT
	color = surface.baseColor;
#endif // UNLIT

	ApplyFog(surface.hit_depth, GetCamera().position, surface.V, color);

	color = max(0, color);

	output[pixel] = float4(color.rgb, 1);

}
