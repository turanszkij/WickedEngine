#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_closestEdge, uint2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

static const int2 offsets[8] = {
	int2(-1, -1), int2(-1, 0), int2(-1, 1),
	int2(0, -1),               int2(0, 1),
	int2(1, -1), int2(1, 0), int2(1, 1),
};

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float4 normal = input[DTid.xy];
	normal.xyz = normal.xyz * 2 - 1;

	uint2 closestEdge = texture_closestEdge[DTid.xy];

	if (any(closestEdge))
	{
		float3 cornerNormal = 0;
		bool corner = false;
		for (uint i = 0; i < 8; ++i)
		{

			cornerNormal += input[DTid.xy + offsets[i]].xyz * 2 - 1;

			uint2 edge = texture_closestEdge[DTid.xy + offsets[i]];
			if (any(edge) && edge.x != closestEdge.x)
			{
				corner = true;
			}
		}

		if (corner)
		{
			normal.xyz = normalize(cornerNormal);
			//output[DTid.xy] = 1;
			//return;
		}
	}

	normal.xyz = normal.xyz * 0.5 + 0.5;
	output[DTid.xy] = normal;
}
