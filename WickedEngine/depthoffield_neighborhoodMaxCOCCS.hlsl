#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(tile_mindepth_maxcoc, float2, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float2, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float min_depth = 1;
	float max_coc = 0;

	int2 dim;
	tile_mindepth_maxcoc.GetDimensions(dim.x, dim.y);

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			const int2 pixel = DTid.xy + int2(x, y);
			if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
			{
				const float2 mindepth_maxcoc = tile_mindepth_maxcoc[pixel];
				min_depth = min(min_depth, mindepth_maxcoc.x);
				max_coc = max(max_coc, mindepth_maxcoc.y);
			}
		}
	}

	output[DTid.xy] = float2(min_depth, max_coc);
}