#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x * DEPTHOFFIELD_TILESIZE, DTid.y);
	float min_depth = 1;
	float max_coc = 0;

	int2 dim;
	texture_lineardepth.GetDimensions(dim.x, dim.y);

	[loop]
	for (uint i = 0; i < DEPTHOFFIELD_TILESIZE; ++i)
	{
		const int2 pixel = uint2(tile_upperleft.x + i, tile_upperleft.y);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
		{
			const float depth = texture_lineardepth[pixel];
			const float coc = dof_scale * abs(1 - dof_focus / (depth * g_xCamera_ZFarP));
			min_depth = min(min_depth, depth);
			max_coc = max(max_coc, coc);
		}
	}

	output[DTid.xy] = float2(min_depth, max_coc);
}
