#define SURFACE_LOAD_ENABLE_WIND
#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "surfaceHF.hlsli"
#include "raytracingHF.hlsli"

// This shader extracts per pixel velocity based on the primitiveID
//	It can run in uniform and divergent manner based on previous primitiveID classification


StructuredBuffer<PrimitiveVisibilityTile> primitive_binned_tiles : register(t0);

RWTexture2D<float2> output_velocity : register(u0);

[numthreads(VISIBILITY_BLOCKSIZE * VISIBILITY_BLOCKSIZE, 1, 1)]
void main(uint Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const uint2 GTid = remap_lane_8x8(groupIndex);

#ifdef PRIMITIVEID_UNIFORM
	const uint tile_offset = 0;
#else
	const uint tile_offset = GetCamera().visibility_tilecount_flat;
#endif // PRIMITIVEID_UNIFORM

	PrimitiveVisibilityTile primitive_tile = primitive_binned_tiles[tile_offset + Gid];

	const uint2 tileID = unpack_pixel(primitive_tile.visibility_tile_id);
	const uint2 pixel = tileID * VISIBILITY_BLOCKSIZE + GTid;

	const float2 uv = ((float2)pixel + 0.5) * GetCamera().internal_resolution_rcp;

	[branch]
	if (!GetCamera().is_uv_inside_scissor(uv))
	{
		output_velocity[pixel] = 0;
		return;
	}

	const float2 clipspace = uv_to_clipspace(uv);
	RayDesc ray = CreateCameraRay(pixel);

#ifdef PRIMITIVEID_UNIFORM
	const uint primitiveID = primitive_tile.primitiveID;
#else
	const uint primitiveID = texture_primitiveID[pixel];
#endif // PRIMITIVEID_UNIFORM

	float3 pre;
	[branch]
	if (primitiveID != 0)
	{
		PrimitiveID prim;
		prim.init();
		prim.unpack(primitiveID);

		Surface surface;
		surface.init();

		[branch]
		if (surface.load(prim, ray.Origin, ray.Direction))
		{
			pre = surface.pre;
		}
	}
	else
	{
		pre = ray.Origin + ray.Direction * GetCamera().z_far;
	}

	float2 pos2D = clipspace;
	float4 pos2DPrev = mul(GetCamera().previous_view_projection, float4(pre, 1));
	if (pos2DPrev.w > 0)
	{
		pos2DPrev.xy /= pos2DPrev.w;
		float2 velocity = ((pos2DPrev.xy - GetCamera().temporalaa_jitter_prev) - (pos2D.xy - GetCamera().temporalaa_jitter)) * float2(0.5, -0.5);
		output_velocity[pixel] = clamp(velocity, -1, 1);
	}
}
