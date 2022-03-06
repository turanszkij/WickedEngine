#include "globals.hlsli"
#include "brdf.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

RWTexture2D<float2> tile_minmax_roughness_horizontal : register(u0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 tile_upperleft = uint2(DTid.x * SSR_TILESIZE, DTid.y);
	float minRoughness = 1.0;
	float maxRoughness = 0.0;

	uint2 dim;
	texture_depth.GetDimensions(dim.x, dim.y);

	[loop]
	for (uint i = 0; i < SSR_TILESIZE; ++i)
	{
		const uint2 pixel = uint2(tile_upperleft.x + i, tile_upperleft.y);
		if (pixel.x >= 0 && pixel.y >= 0 && pixel.x < dim.x && pixel.y < dim.y)
		{
			float depth = texture_depth[pixel];
			if (depth == 0.0)
			{
				maxRoughness = max(maxRoughness, 1.0);
				minRoughness = min(minRoughness, 1.0);
			}
			else
			{
				float roughness = texture_roughness[pixel];
				maxRoughness = max(maxRoughness, roughness);
				minRoughness = min(minRoughness, roughness);
			}
		}
	}

	tile_minmax_roughness_horizontal[DTid.xy] = float2(minRoughness, maxRoughness);
}
