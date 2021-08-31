#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWTEXTURE2D(output_velocity, float2, 0);

RWTEXTURE2D(output_depth_mip0, float, 1);
RWTEXTURE2D(output_depth_mip1, float, 2);
RWTEXTURE2D(output_depth_mip2, float, 3);

RWTEXTURE2D(output_lineardepth_mip0, float, 4);
RWTEXTURE2D(output_lineardepth_mip1, float, 5);
RWTEXTURE2D(output_lineardepth_mip2, float, 6);

#ifdef VISIBILITY_MSAA
TEXTURE2DMS(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2DMS(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
RWTEXTURE2D(output_primitiveID, uint2, 7);
#else
TEXTURE2D(texture_primitiveID, uint2, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_depthbuffer, float, TEXSLOT_ONDEMAND1);
#endif // VISIBILITY_MSAA

groupshared float tile_depths[8][8];
groupshared float tile_lineardepths[8][8];

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex, uint3 Gid : SV_GroupID, uint3 GTid : SV_GroupThreadID)
{
	uint2 pixel = DTid.xy;

	const float2 uv = ((float2)pixel + 0.5) * g_xFrame.InternalResolution_rcp;
	const float depth = texture_depthbuffer[pixel];
	const float3 P = reconstructPosition(uv, depth);

	float3 pre = P;

	if (depth > 0)
	{
		uint2 primitiveID = texture_primitiveID[pixel];

#ifdef VISIBILITY_MSAA
		output_primitiveID[pixel] = primitiveID;
#endif // VISIBILITY_MSAA

		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		if (surface.load(prim, P))
		{
			pre = surface.pre;
		}

	}

	float4 pos2DPrev = mul(g_xCamera.PrevVP, float4(pre, 1));
	pos2DPrev.xy /= pos2DPrev.w;
	float2 pos2D = uv * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((pos2DPrev.xy - g_xCamera.TemporalAAJitterPrev) - (pos2D.xy - g_xCamera.TemporalAAJitter)) * float2(0.5, -0.5);

	output_velocity[pixel] = velocity;



	// Downsample depths:
	output_depth_mip0[pixel] = depth;
	float lineardepth = getLinearDepth(depth) * g_xCamera.ZFarP_rcp;
	output_lineardepth_mip0[pixel] = lineardepth;

#if 0
	// Downsample - MAX

	tile_depths[GTid.x][GTid.y] = depth;
	tile_lineardepths[GTid.x][GTid.y] = lineardepth;
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		float maxdepth = max(tile_depths[GTid.x][GTid.y], max(tile_depths[GTid.x + 1][GTid.y], max(tile_depths[GTid.x][GTid.y + 1], tile_depths[GTid.x + 1][GTid.y + 1])));
		tile_depths[GTid.x][GTid.y] = maxdepth;
		output_depth_mip1[DTid.xy / 2] = maxdepth;

		maxdepth = max(tile_lineardepths[GTid.x][GTid.y], max(tile_lineardepths[GTid.x + 1][GTid.y], max(tile_lineardepths[GTid.x][GTid.y + 1], tile_lineardepths[GTid.x + 1][GTid.y + 1])));
		tile_lineardepths[GTid.x][GTid.y] = maxdepth;
		output_lineardepth_mip1[DTid.xy / 2] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		float maxdepth = max(tile_depths[GTid.x][GTid.y], max(tile_depths[GTid.x + 2][GTid.y], max(tile_depths[GTid.x][GTid.y + 2], tile_depths[GTid.x + 2][GTid.y + 2])));
		tile_depths[GTid.x][GTid.y] = maxdepth;
		output_depth_mip2[DTid.xy / 4] = maxdepth;

		maxdepth = max(tile_lineardepths[GTid.x][GTid.y], max(tile_lineardepths[GTid.x + 2][GTid.y], max(tile_lineardepths[GTid.x][GTid.y + 2], tile_lineardepths[GTid.x + 2][GTid.y + 2])));
		tile_lineardepths[GTid.x][GTid.y] = maxdepth;
		output_lineardepth_mip2[DTid.xy / 4] = maxdepth;
	}

#else

	// Downsample - SIMPLE
	output_depth_mip1[pixel / 2] = depth;
	output_lineardepth_mip1[pixel / 2] = lineardepth;

	output_depth_mip2[pixel / 4] = depth;
	output_lineardepth_mip2[pixel / 4] = lineardepth;

#endif
}
