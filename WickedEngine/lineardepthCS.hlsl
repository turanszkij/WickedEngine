#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output_lineardepth_mip0, float, 0);
RWTEXTURE2D(output_lineardepth_mip1, float, 1);
RWTEXTURE2D(output_lineardepth_mip2, float, 2);
RWTEXTURE2D(output_lineardepth_mip3, float, 3);
RWTEXTURE2D(output_lineardepth_mip4, float, 4);
RWTEXTURE2D(output_lineardepth_mip5, float, 5);

RWTEXTURE2D(output_depth_mip1, float, 6);
RWTEXTURE2D(output_depth_mip2, float, 7);

groupshared float tile[POSTPROCESS_LINEARDEPTH_BLOCKSIZE][POSTPROCESS_LINEARDEPTH_BLOCKSIZE];

[numthreads(POSTPROCESS_LINEARDEPTH_BLOCKSIZE, POSTPROCESS_LINEARDEPTH_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	// Native depth MAX chain:
	//	Native depth MAX = closest pixel
	const float4 depths = float4(
		texture_depth[clamp(DTid.xy * 2 + uint2(0, 0), 0, lineardepth_inputresolution - 1)],
		texture_depth[clamp(DTid.xy * 2 + uint2(1, 0), 0, lineardepth_inputresolution - 1)],
		texture_depth[clamp(DTid.xy * 2 + uint2(0, 1), 0, lineardepth_inputresolution - 1)],
		texture_depth[clamp(DTid.xy * 2 + uint2(1, 1), 0, lineardepth_inputresolution - 1)]
	);

	float maxdepth = max(depths.x, max(depths.y, max(depths.z, depths.w)));
	tile[GTid.x][GTid.y] = maxdepth;
	output_depth_mip1[DTid.xy] = maxdepth;
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		maxdepth = max(tile[GTid.x][GTid.y], max(tile[GTid.x + 1][GTid.y], max(tile[GTid.x][GTid.y + 1], tile[GTid.x + 1][GTid.y + 1])));
		tile[GTid.x][GTid.y] = maxdepth;
		output_depth_mip2[DTid.xy / 2] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	// Linear depth MAX chain:
	//	Linear depth MAX = farthest pixel
	const float4 lineardepths = float4(
		getLinearDepth(depths.x),
		getLinearDepth(depths.y),
		getLinearDepth(depths.z),
		getLinearDepth(depths.w)
	) * g_xCamera_ZFarP_rcp;
	output_lineardepth_mip0[DTid.xy * 2 + uint2(0, 0)] = lineardepths.x;
	output_lineardepth_mip0[DTid.xy * 2 + uint2(1, 0)] = lineardepths.y;
	output_lineardepth_mip0[DTid.xy * 2 + uint2(0, 1)] = lineardepths.z;
	output_lineardepth_mip0[DTid.xy * 2 + uint2(1, 1)] = lineardepths.w;

	maxdepth = max(lineardepths.x, max(lineardepths.y, max(lineardepths.z, lineardepths.w)));
	tile[GTid.x][GTid.y] = maxdepth;
	output_lineardepth_mip1[DTid.xy] = maxdepth;
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		maxdepth = max(tile[GTid.x][GTid.y], max(tile[GTid.x + 1][GTid.y], max(tile[GTid.x][GTid.y + 1], tile[GTid.x + 1][GTid.y+ 1])));
		tile[GTid.x][GTid.y] = maxdepth;
		output_lineardepth_mip2[DTid.xy / 2] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		maxdepth = max(tile[GTid.x][GTid.y], max(tile[GTid.x + 2][GTid.y], max(tile[GTid.x][GTid.y + 2], tile[GTid.x + 2][GTid.y + 2])));
		tile[GTid.x][GTid.y] = maxdepth;
		output_lineardepth_mip3[DTid.xy / 4] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		maxdepth = max(tile[GTid.x][GTid.y], max(tile[GTid.x + 4][GTid.y], max(tile[GTid.x][GTid.y + 4], tile[GTid.x + 4][GTid.y + 4])));
		tile[GTid.x][GTid.y] = maxdepth;
		output_lineardepth_mip4[DTid.xy / 8] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		maxdepth = max(tile[GTid.x][GTid.y], max(tile[GTid.x + 8][GTid.y], max(tile[GTid.x][GTid.y + 8], tile[GTid.x + 8][GTid.y + 8])));
		output_lineardepth_mip5[DTid.xy / 16] = maxdepth;
	}
}
