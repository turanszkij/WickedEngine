#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_edgeMap, uint4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(input_output_normals, unorm float4, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float4 normal = input_output_normals[DTid.xy];
	normal.xyz = normal.xyz * 2 - 1;

	uint4 edgeMap = input_edgeMap[DTid.xy];

	uint2 closestEdge = edgeMap.xy;
	if (closestEdge.x)
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

	closestEdge = edgeMap.zw;
	if (closestEdge.x)
	{
		float3 edgeNormal = unpack_unitvector(closestEdge.x);
		uint2 edgePixel = unpack_pixel(closestEdge.y);
		float2 edgeUV = (edgePixel.xy + 0.5f) * xPPResolution_rcp;

		const float edgeDepth = texture_depth[edgePixel];
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float depth = texture_depth[DTid.xy];
		const float3 P = reconstructPosition(uv, depth);

		float dist = length(edgePosition - P);

		float l = dist * 5;
		l = saturate(l);
		l = pow(l, 2);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

		//output[DTid.xy] = dist * 10;
		//return;
	}

	normal.xyz = normal.xyz * 0.5 + 0.5;
	input_output_normals[DTid.xy] = normal;
}
