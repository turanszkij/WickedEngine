#define SURFACE_LOAD_DISABLE_WIND
#define SHADOW_MASK_ENABLED
#define DISABLE_DECALS
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"
#include "objectHF.hlsli"

ConstantBuffer<ShaderTypeBin> bin : register(b10);
StructuredBuffer<uint> binned_pixels : register(t0);
StructuredBuffer<uint2> input_pixel_data_common : register(t1);
StructuredBuffer<uint4> input_pixel_data_varying : register(t2);

RWTexture2D<float4> output : register(u0);

[numthreads(64,	1, 1)]
void main(uint DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID)
{
	if (DTid.x >= bin.count)
		return;

	const uint bin_data_index = bin.offset + DTid.x;
	uint2 pixel = unpack_pixel(binned_pixels[bin_data_index]);
	uint primitiveID = texture_primitiveID[pixel];
	if (!any(primitiveID))
	{
		return;
	}

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);

	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();

#if 0
	RayDesc ray = CreateCameraRay(clipspace);
	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction))
	{
		return;
	}
#else
	[branch]
	if (!surface.preload_internal(prim))
	{
		return;
	}
#endif

	const float depth = texture_depth[pixel];
	surface.P = reconstruct_position(uv, depth);
	surface.V = GetCamera().position - surface.P;
	surface.hit_depth = length(surface.V);
	surface.V /= surface.hit_depth;

	surface.pixel = pixel.xy;
	surface.screenUV = uv;
	surface.albedo = output[pixel].rgb;
	surface.opacity = 1;
	surface.baseColor = float4(surface.albedo, surface.opacity);

	float4 data0 = unpack_rgba(input_pixel_data_common[bin_data_index].x);
	surface.f0 = data0.rgb;
	surface.occlusion = data0.a;
	surface.emissiveColor = Unpack_R11G11B10_FLOAT(input_pixel_data_common[bin_data_index].y);

#ifdef ANISOTROPIC
	surface.T = unpack_half4(input_pixel_data_varying[bin_data_index].xy);
#endif // ANISOTROPIC

#ifdef SHEEN
	float4 data_sheen = unpack_rgba(input_pixel_data_varying[bin_data_index].x);
	surface.sheen.color = data_sheen.rgb;
	surface.sheen.roughness = data_sheen.a;
#endif // SHEEN

#ifdef CLEARCOAT
	surface.N = decode_oct(unpack_half2(input_pixel_data_varying[bin_data_index].y));
	surface.roughness = unpack_rgba(input_pixel_data_varying[bin_data_index].z).r;
	surface.clearcoat.N = decode_oct(texture_normal[pixel].rg);
	surface.clearcoat.roughness = texture_roughness[pixel].r;
#else
	surface.N = decode_oct(texture_normal[pixel].rg);
	surface.roughness = texture_roughness[pixel].r;
#endif // CLEARCOAT

	surface.update();

	if ((surface.flags & SURFACE_FLAG_GI_APPLIED) == 0)
	{
		float3 ambient = GetAmbient(surface.N);
		surface.gi = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));
	}

	Lighting lighting;
	lighting.create(0, 0, surface.gi, 0);

	ApplyEmissive(surface, lighting);


#ifdef PLANARREFLECTION
	float2 bumpColor = unpack_half2(input_pixel_data_varying[bin_data_index].x);
	lighting.indirect.specular += PlanarReflection(surface, bumpColor) * surface.F;
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
	color = float4(surface.albedo, 1);
#endif // UNLIT

	ApplyFog(surface.hit_depth, GetCamera().position, surface.V, color);

	color = max(0, color);

	output[pixel] = float4(color.rgb, 1);

}
