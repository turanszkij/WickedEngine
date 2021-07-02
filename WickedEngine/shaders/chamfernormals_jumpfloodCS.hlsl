#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_edgeMap, uint4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output_edgeMap, uint4, 0);

static const int2 offsets[9] = {
	int2(-1, -1), int2(-1, 0), int2(-1, 1),
	int2(0, -1), int2(0, 0), int2(0, 1),
	int2(1, -1), int2(1, 0), int2(1, 1),
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5) * xPPResolution_rcp;

	float lineardepth = texture_lineardepth[DTid.xy];

	const float depth = texture_depth[DTid.xy];
	const float3 P = reconstructPosition(uv, depth);

	float bestEdgeDistance = FLT_MAX;
	uint2 bestEdge = 0;
	float bestCornerDistance = FLT_MAX;
	uint2 bestCorner = 0;

	for (uint i = 0; i < 9; ++i)
	{
		int2 pixel = (int2)DTid.xy + int2(offsets[i] * xPPParams0.x);
		uint4 edgeMap = input_edgeMap[pixel];

		uint2 edge = edgeMap.xy;
		if (edge.x)
		{
			uint2 edgePixel = unpack_pixel(edge.y);
			float2 edgeUV = (edgePixel.xy + 0.5) * xPPResolution_rcp;

			const float edgeDepth = texture_depth[edgePixel];
			const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);
			float dist = length(edgePosition - P);

			if (dist < bestEdgeDistance)
			{
				bestEdgeDistance = dist;
				bestEdge = edge;
			}

		}

		uint2 corner = edgeMap.zw;
		if (corner.x)
		{
			uint2 cornerPixel = unpack_pixel(corner.y);
			float2 cornerUV = (cornerPixel.xy + 0.5) * xPPResolution_rcp;

			const float cornerDepth = texture_depth[cornerPixel];
			const float3 cornerPosition = reconstructPosition(cornerUV, cornerDepth);
			float dist = length(cornerPosition - P);

			if (dist < bestCornerDistance)
			{
				bestCornerDistance = dist;
				bestCorner = corner;
			}

		}
	}
	output_edgeMap[DTid.xy] = uint4(bestEdge, bestCorner);
}
