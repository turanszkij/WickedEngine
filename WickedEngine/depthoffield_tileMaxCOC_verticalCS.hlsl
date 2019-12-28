#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(tilemax_horizontal, float2, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x, DTid.y * DEPTHOFFIELD_TILESIZE);
	float min_depth = 1;
	float max_coc = 0;

	int2 dim;
	tilemax_horizontal.GetDimensions(dim.x, dim.y);

	[loop]
	for (uint i = 0; i < DEPTHOFFIELD_TILESIZE; ++i)
	{
		const int2 pixel = uint2(tile_upperleft.x, tile_upperleft.y + i);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
		{
			const float2 mindepth_maxcoc = tilemax_horizontal[pixel];
			min_depth = min(min_depth, mindepth_maxcoc.x);
			max_coc = max(max_coc, mindepth_maxcoc.y);
		}
	}

	output[DTid.xy] = float2(min_depth, max_coc);
}
