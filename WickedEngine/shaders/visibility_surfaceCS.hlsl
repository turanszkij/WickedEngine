#define ENTITY_TILE_UNIFORM
#define SURFACE_LOAD_QUAD_DERIVATIVES
#ifndef REDUCED
#define SURFACE_LOAD_ENABLE_WIND
#endif // REDUCED
#define SVT_FEEDBACK
#define TEXTURE_SLOT_NONUNIFORM
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "raytracingHF.hlsli"
#include "surfaceHF.hlsli"
#include "shadingHF.hlsli"

struct VisibilityPushConstants
{
	uint global_tile_offset;
};
PUSHCONSTANT(push, VisibilityPushConstants);

StructuredBuffer<VisibilityTile> binned_tiles : register(t0);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float2> output_normal : register(u1);
RWTexture2D<unorm float> output_roughness : register(u2);
RWTexture2D<uint4> output_payload_0 : register(u3);
RWTexture2D<uint4> output_payload_1 : register(u4);

[numthreads(VISIBILITY_BLOCKSIZE, VISIBILITY_BLOCKSIZE, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint tile_offset = push.global_tile_offset + Gid.x;
	VisibilityTile tile = binned_tiles[tile_offset];
	const uint2 GTid = remap_lane_8x8(groupIndex);
	const uint2 pixel = unpack_pixel(tile.visibility_tile_id) * VISIBILITY_BLOCKSIZE + GTid;

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;
	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(clipspace);
	float3 rayDirection_quad_x = QuadReadAcrossX(ray.Direction);
	float3 rayDirection_quad_y = QuadReadAcrossY(ray.Direction);

	[branch] if (!tile.check_thread_valid(groupIndex)) return; // only return after QuadRead operations!

	uint primitiveID = texture_primitiveID[pixel];
	PrimitiveID prim;
	prim.unpack(primitiveID);

	Surface surface;
	surface.init();
	surface.pixel = pixel.xy;
	surface.screenUV = uv;

	[branch]
	if (!surface.load(prim, ray.Origin, ray.Direction, rayDirection_quad_x, rayDirection_quad_y, tile.entity_flat_tile_index))
	{
		return;
	}

#ifdef UNLIT
	float4 color = float4(surface.albedo, 1);
	ApplyFog(surface.hit_depth, surface.V, color);
	output[pixel] = color;
	return;
#endif // UNLIT

	// Write out sampleable attributes for post processing into textures:
#ifdef CLEARCOAT
	// Clearcoat must write out the top layer's normal and roughness to match the specs:
	output_normal[pixel] = encode_oct(surface.clearcoat.N);
	output_roughness[pixel] = surface.clearcoat.roughness;
#else
	output_normal[pixel] = encode_oct(surface.N);
	output_roughness[pixel] = surface.roughness;
#endif // CLEARCOAT


#ifndef REDUCED
	// Pack primary payload for shading:
	uint4 payload_0;
	payload_0.x = pack_rgba(float4(ApplySRGBCurve_Fast(surface.albedo), surface.occlusion));
	payload_0.y = pack_rgba(float4(ApplySRGBCurve_Fast(surface.f0), surface.roughness));
	payload_0.z = pack_half2(encode_oct(surface.N));
	payload_0.w = Pack_R11G11B10_FLOAT(surface.emissiveColor);
	output_payload_0[pixel] = payload_0;


	// Pack secondary payload (surface parameters that are varying per shader type):

#ifdef ANISOTROPIC
	output_payload_1[pixel].xy = pack_half4(surface.T);
#endif // ANISOTROPIC

#ifdef PLANARREFLECTION
	output_payload_1[pixel].x = pack_half2(surface.bumpColor.rg);
#endif

#ifdef SHEEN
	output_payload_1[pixel].x = pack_rgba(float4(surface.sheen.color, surface.sheen.roughness));
#endif // SHEEN

#ifdef CLEARCOAT
	output_payload_1[pixel].y = pack_half2(encode_oct(surface.clearcoat.N));
	output_payload_1[pixel].z = pack_rgba(float4(surface.clearcoat.roughness, 0, 0, 0));
#endif // CLEARCOAT

#endif // REDUCED
}
