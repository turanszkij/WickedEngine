#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_closestEdge, uint2, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float4 normal = input[DTid.xy];
	normal.xyz = normal.xyz * 2 - 1;

	uint2 closestEdge = texture_closestEdge[DTid.xy];

	if (any(closestEdge))
	{
		float3 edgeNormal = unpack_unitvector(closestEdge.x);
		uint2 edgePixel = unpack_pixel(closestEdge.y);
		float2 edgeUV = (edgePixel.xy + 0.5f) * xPPResolution_rcp;

		const float edgeDepth = texture_depth[edgePixel];
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float depth = texture_depth[DTid.xy];
		const float3 P = reconstructPosition(uv, depth);

		float dist = length(edgePosition - P);
		
		float l = dist * 10;
		l = saturate(l);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

		//output[DTid.xy] = dist * 10;
		//return;
	}

	normal.xyz = normal.xyz * 0.5 + 0.5;
	output[DTid.xy] = normal;
}
