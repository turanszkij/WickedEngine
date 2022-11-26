#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "depthOfFieldHF.hlsli"

RWTexture2D<float2> tile_mindepth_maxcoc : register(u0);
RWTexture2D<float> tile_mincoc : register(u1);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x * DEPTHOFFIELD_TILESIZE, DTid.y);
	float min_depth = 1;
	float max_coc = 0;
	float min_coc = 1000000;

	[loop]
	for (uint i = 0; i < DEPTHOFFIELD_TILESIZE; ++i)
	{
		const int2 pixel = uint2(tile_upperleft.x + i, tile_upperleft.y);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < postprocess.resolution.x && pixel.y < postprocess.resolution.y)
		{
			const float2 uv = (pixel + 0.5) * postprocess.resolution_rcp;
			const float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, uv, 0);
			const float coc = get_coc(depth);
			min_depth = min(min_depth, depth);
			max_coc = max(max_coc, coc);
			min_coc = min(min_coc, coc);
		}
	}

	tile_mindepth_maxcoc[DTid.xy] = float2(min_depth, max_coc);
	tile_mincoc[DTid.xy] = min_coc;
}
