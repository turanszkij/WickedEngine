#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_PostProcess.h"

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output_fullres, unorm float, 0);
RWTEXTURE2D(output_minmax_mip0, unorm float2, 1);
RWTEXTURE2D(output_minmax_mip1, unorm float2, 2);
RWTEXTURE2D(output_minmax_mip2, unorm float2, 3);
RWTEXTURE2D(output_minmax_mip3, unorm float2, 4);
RWTEXTURE2D(output_minmax_mip4, unorm float2, 5);

groupshared float tile_min[POSTPROCESS_LINEARDEPTH_BLOCKSIZE][POSTPROCESS_LINEARDEPTH_BLOCKSIZE];
groupshared float tile_max[POSTPROCESS_LINEARDEPTH_BLOCKSIZE][POSTPROCESS_LINEARDEPTH_BLOCKSIZE];

[numthreads(POSTPROCESS_LINEARDEPTH_BLOCKSIZE, POSTPROCESS_LINEARDEPTH_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID)
{
	const float4 lineardepths = float4(
		getLinearDepth(input[clamp(DTid.xy * 2 + uint2(0, 0), 0, lineardepth_inputresolution - 1)]),
		getLinearDepth(input[clamp(DTid.xy * 2 + uint2(1, 0), 0, lineardepth_inputresolution - 1)]),
		getLinearDepth(input[clamp(DTid.xy * 2 + uint2(0, 1), 0, lineardepth_inputresolution - 1)]),
		getLinearDepth(input[clamp(DTid.xy * 2 + uint2(1, 1), 0, lineardepth_inputresolution - 1)])
	) * g_xCamera_ZFarP_rcp;
	output_fullres[DTid.xy * 2 + uint2(0, 0)] = lineardepths.x;
	output_fullres[DTid.xy * 2 + uint2(1, 0)] = lineardepths.y;
	output_fullres[DTid.xy * 2 + uint2(0, 1)] = lineardepths.z;
	output_fullres[DTid.xy * 2 + uint2(1, 1)] = lineardepths.w;

	float mindepth = min(lineardepths.x, min(lineardepths.y, min(lineardepths.z, lineardepths.w)));
	float maxdepth = max(lineardepths.x, max(lineardepths.y, max(lineardepths.z, lineardepths.w)));
	tile_min[GTid.x][GTid.y] = mindepth;
	tile_max[GTid.x][GTid.y] = maxdepth;
	output_minmax_mip0[DTid.xy] = float2(mindepth, maxdepth);
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		mindepth = min(tile_min[GTid.x][GTid.y], min(tile_min[GTid.x + 1][GTid.y], min(tile_min[GTid.x][GTid.y + 1], tile_min[GTid.x + 1][GTid.y+ 1])));
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 1][GTid.y], max(tile_max[GTid.x][GTid.y + 1], tile_max[GTid.x + 1][GTid.y+ 1])));
		tile_min[GTid.x][GTid.y] = mindepth;
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_minmax_mip1[DTid.xy / 2] = float2(mindepth, maxdepth);
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		mindepth = min(tile_min[GTid.x][GTid.y], min(tile_min[GTid.x + 2][GTid.y], min(tile_min[GTid.x][GTid.y + 2], tile_min[GTid.x + 2][GTid.y + 2])));
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 2][GTid.y], max(tile_max[GTid.x][GTid.y + 2], tile_max[GTid.x + 2][GTid.y + 2])));
		tile_min[GTid.x][GTid.y] = mindepth;
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_minmax_mip2[DTid.xy / 4] = float2(mindepth, maxdepth);
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		mindepth = min(tile_min[GTid.x][GTid.y], min(tile_min[GTid.x + 4][GTid.y], min(tile_min[GTid.x][GTid.y + 4], tile_min[GTid.x + 4][GTid.y + 4])));
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 4][GTid.y], max(tile_max[GTid.x][GTid.y + 4], tile_max[GTid.x + 4][GTid.y + 4])));
		tile_min[GTid.x][GTid.y] = mindepth;
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_minmax_mip3[DTid.xy / 8] = float2(mindepth, maxdepth);
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		mindepth = min(tile_min[GTid.x][GTid.y], min(tile_min[GTid.x + 8][GTid.y], min(tile_min[GTid.x][GTid.y + 8], tile_min[GTid.x + 8][GTid.y + 8])));
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 8][GTid.y], max(tile_max[GTid.x][GTid.y + 8], tile_max[GTid.x + 8][GTid.y + 8])));
		output_minmax_mip4[DTid.xy / 16] = float2(mindepth, maxdepth);
	}
}
