#define SURFACE_LOAD_QUAD_DERIVATIVES
#define SURFACE_LOAD_ENABLE_WIND
#define SVT_FEEDBACK
#define TEXTURE_SLOT_NONUNIFORM
#define SHADOW_MASK_ENABLED
#define PRIMITIVEID_FROM_MESHLET_OPTIMIZED
//#define DISABLE_VOXELGI
//#define DISABLE_ENVMAPS
//#define DISABLE_SOFT_SHADOWMAP
//#define DISABLE_TRANSPARENT_SHADOWMAP

#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#define DISABLE_VOXELGI
#endif // PLANARREFLECTION

#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "brdf.hlsli"
#include "shadingHF.hlsli"

// This shader computes per-pixel lighting based on primitiveID

struct VisibilityPushConstants
{
	uint global_tile_offset;
};
PUSHCONSTANT(push, VisibilityPushConstants);

StructuredBuffer<VisibilityTile> binned_tiles : register(t0);
StructuredBuffer<PrimitiveVisibilityTile> primitive_binned_tiles : register(t1);

RWTexture2D<float4> output : register(u0);

[numthreads(VISIBILITY_BLOCKSIZE * VISIBILITY_BLOCKSIZE, 1, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint tile_offset = push.global_tile_offset + Gid.x;
	VisibilityTile tile = binned_tiles[tile_offset];
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 tileID = unpack_pixel(tile.visibility_tile_id);
	const uint2 pixel = tileID * VISIBILITY_BLOCKSIZE + GTid;

	ShaderCamera camera = GetCamera();
	const uint entity_flat_tile_index = flatten2D(tileID / VISIBILITY_TILED_CULLING_GRANULARITY, camera.entity_culling_tilecount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;

	const float2 uv = ((float2)pixel + 0.5) * camera.internal_resolution_rcp;
	RayDesc ray = CreateCameraRay(pixel);
	float3 rayDirection_quad_x = QuadReadAcrossX(ray.Direction);
	float3 rayDirection_quad_y = QuadReadAcrossY(ray.Direction);

#ifdef PRIMITIVEID_UNIFORM
	const uint primitiveID = tile.shaderType_or_primitiveID;
#else
	const uint primitiveID = texture_primitiveID[pixel];
#endif // PRIMITIVEID_UNIFORM

	PrimitiveID prim;
	prim.init();
	prim.unpack(primitiveID);

#ifndef PRIMITIVEID_UNIFORM
	[branch] if (prim.shaderType != tile.shaderType_or_primitiveID) return;
#endif // PRIMITIVEID_UNIFORM

	Surface surface;
	surface.init();
	surface.pixel = pixel.xy;
	surface.screenUV = uv;

	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction, rayDirection_quad_x, rayDirection_quad_y, entity_flat_tile_index))
	{
		return;
	}

	surface.update();

	if (!surface.IsGIApplied())
	{
		half3 ambient = GetAmbient(surface.N);
		surface.gi = lerp(ambient, ambient * surface.sss.rgb, saturate(surface.sss.a));
	}

	Lighting lighting;
	lighting.create(0, 0, surface.gi, 0);

	TiledLighting(surface, lighting, entity_flat_tile_index, camera);

#ifndef CARTOON
	[branch]
	if (camera.texture_ssr_index >= 0)
	{
		half4 ssr = bindless_textures_half4[descriptor_index(camera.texture_ssr_index)].SampleLevel(sampler_linear_clamp, surface.screenUV, 0);
		lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb * surface.F, ssr.a);
	}
	[branch]
	if (camera.texture_ssgi_index >= 0)
	{
		surface.ssgi = bindless_textures_half4[descriptor_index(camera.texture_ssgi_index)].SampleLevel(sampler_linear_clamp, surface.screenUV, 0).rgb;
	}
	[branch]
	if (camera.texture_ao_index >= 0)
	{
		surface.occlusion *= bindless_textures_half4[descriptor_index(camera.texture_ao_index)].SampleLevel(sampler_linear_clamp, surface.screenUV, 0).r;
	}
#endif // CARTOON

	half4 color = 0;

	ApplyLighting(surface, lighting, color);

	half4 rimHighlight = surface.inst.GetRimHighlight();
	color.rgb += rimHighlight.rgb * pow(1 - surface.NdotV, rimHighlight.w);

#ifdef INTERIORMAPPING
	surface.baseColor.rgb += surface.emissiveColor;
	surface.baseColor *= InteriorMapping(surface.P, surface.N, surface.V, surface.material, surface.inst);
#endif // INTERIORMAPPING

#if defined(UNLIT) || defined(INTERIORMAPPING)
	color = surface.baseColor;
#endif // UNLIT

	ApplyFog(surface.hit_depth, surface.V, color);

	color = saturateMediump(color);

	output[pixel] = half4(color.rgb, 1);

}
