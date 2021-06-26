#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_closestEdge, uint2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output_closestEdge, uint2, 0);

static const int2 offsets[9] = {
	int2(-1, -1), int2(-1, 0), int2(-1, 1),
	int2(0, -1), int2(0, 0), int2(0, 1),
	int2(1, -1), int2(1, 0), int2(1, 1),
};

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float lineardepth = texture_lineardepth[DTid.xy];

	const float depth = texture_depth[DTid.xy];
	const float3 P = reconstructPosition(uv, depth);

	float bestDistance = FLT_MAX;
	uint2 bestEdge = 0;

	for (uint i = 0; i < 9; ++i)
	{
		int2 pixel = (int2)DTid.xy + int2(offsets[i] * xPPParams0.x);
		if (pixel.x < 0 || pixel.x >= (int)xPPResolution.x || pixel.y < 0 || pixel.y >= (int)xPPResolution.y)
			continue;

		float ld = texture_lineardepth[pixel];
		float diff = abs(lineardepth - ld);

		if (diff < 0.0001)
		{
			uint2 edge = texture_closestEdge[pixel];
			if (any(edge))
			{
				uint2 edgePixel = unpack_pixel(edge.y);
				float2 edgeUV = (edgePixel.xy + 0.5f) * xPPResolution_rcp;

				const float edgeDepth = texture_depth[edgePixel];
				const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);
				float dist = length(edgePosition - P);

				if (dist < 0.1 && dist < bestDistance)
				{
					bestDistance = dist;
					bestEdge = edge;
				}

			}
		}
	}
	output_closestEdge[DTid.xy] = bestEdge;
}
