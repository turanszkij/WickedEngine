#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output_mip0, float, 0);
RWTEXTURE2D(output_mip1, float, 1);
RWTEXTURE2D(output_mip2, float, 2);
RWTEXTURE2D(output_mip3, float, 3);
RWTEXTURE2D(output_mip4, float, 4);
RWTEXTURE2D(output_mip5, float, 5);

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
	output_mip0[DTid.xy * 2 + uint2(0, 0)] = lineardepths.x;
	output_mip0[DTid.xy * 2 + uint2(1, 0)] = lineardepths.y;
	output_mip0[DTid.xy * 2 + uint2(0, 1)] = lineardepths.z;
	output_mip0[DTid.xy * 2 + uint2(1, 1)] = lineardepths.w;

	float maxdepth = max(lineardepths.x, max(lineardepths.y, max(lineardepths.z, lineardepths.w)));
	tile_max[GTid.x][GTid.y] = maxdepth;
	output_mip1[DTid.xy] = maxdepth;
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 2 == 0 && GTid.y % 2 == 0)
	{
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 1][GTid.y], max(tile_max[GTid.x][GTid.y + 1], tile_max[GTid.x + 1][GTid.y+ 1])));
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_mip2[DTid.xy / 2] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 4 == 0 && GTid.y % 4 == 0)
	{
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 2][GTid.y], max(tile_max[GTid.x][GTid.y + 2], tile_max[GTid.x + 2][GTid.y + 2])));
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_mip3[DTid.xy / 4] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 8 == 0 && GTid.y % 8 == 0)
	{
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 4][GTid.y], max(tile_max[GTid.x][GTid.y + 4], tile_max[GTid.x + 4][GTid.y + 4])));
		tile_max[GTid.x][GTid.y] = maxdepth;
		output_mip4[DTid.xy / 8] = maxdepth;
	}
	GroupMemoryBarrierWithGroupSync();

	if (GTid.x % 16 == 0 && GTid.y % 16 == 0)
	{
		maxdepth = max(tile_max[GTid.x][GTid.y], max(tile_max[GTid.x + 8][GTid.y], max(tile_max[GTid.x][GTid.y + 8], tile_max[GTid.x + 8][GTid.y + 8])));
		output_mip5[DTid.xy / 16] = maxdepth;
	}
}
