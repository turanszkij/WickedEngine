#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(texture_closestEdge, uint2, 0);

static const int2 offsets[3] = {
	int2(1, 0),
	int2(0, 1),
	int2(1, 1),
};

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 normal = input[DTid.xy];
	normal.xyz = normal.xyz * 2 - 1;

	float lineardepth = texture_lineardepth[DTid.xy];

	for (uint i = 0; i < 3; ++i)
	{
		int2 pixel2 = DTid.xy + offsets[i];

		float ld = texture_lineardepth[pixel2];
		float diff = abs(lineardepth - ld);

		if (diff < 0.0001)
		{
			float3 n = input[pixel2].xyz * 2 - 1;

			if (abs(dot(normal.xyz, n.xyz)) < 0.5)
			{
				float3 edgeNormal = normalize(normal.xyz + n.xyz);

				texture_closestEdge[DTid.xy] = uint2(pack_unitvector(edgeNormal), pack_pixel(DTid.xy));
				return;
			}
		}
	}

	texture_closestEdge[DTid.xy] = 0;
}
