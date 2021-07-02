#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_edgeMap, uint4, TEXSLOT_ONDEMAND0); // DXGI_FORMAT_R32G32B32A32_UINT

RWTEXTURE2D(input_output_normals, unorm float4, 0); // DXGI_FORMAT_R8G8B8A8_UNORM

static const float chamfer_edge_radius = 0.05; // radius of the edge chamfering in world space
static const float chamfer_edge_strength = 1; // falloff for edge smoothness (lower: smaller chamfer, higher: stronger chamfer)

static const float chamfer_corner_radius = 0.1; // radius of the corner chamfering in world space
static const float chamfer_corner_strength = 2; // falloff for corner smoothness (lower: smaller chamfer, higher: stronger chamfer)

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;
	const float depth = texture_depth[DTid.xy];
	const float3 P = reconstructPosition(uv, depth);

	float4 normal = input_output_normals[DTid.xy];
	normal.xyz = normalize(normal.xyz * 2 - 1);

	const uint4 edgeMap = input_edgeMap[DTid.xy];

	// edges:
	uint2 closestEdge = edgeMap.xy;
	if (closestEdge.x)
	{
		const float3 edgeNormal = unpack_unitvector(closestEdge.x);
		const uint2 edgePixel = unpack_pixel(closestEdge.y);
		const float2 edgeUV = (edgePixel.xy + 0.5f) * xPPResolution_rcp;

		const float edgeDepth = texture_depth[edgePixel];
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float dist = length(edgePosition - P);
		
		float l = dist / chamfer_edge_radius;
		l = saturate(l);
		l = pow(l, chamfer_edge_strength);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

		//input_output_normals[DTid.xy] = dist * 10;
		//return;
	}

	// corners:
	closestEdge = edgeMap.zw;
	if (closestEdge.x)
	{
		const float3 edgeNormal = unpack_unitvector(closestEdge.x);
		const uint2 edgePixel = unpack_pixel(closestEdge.y);
		const float2 edgeUV = (edgePixel.xy + 0.5f) * xPPResolution_rcp;

		const float edgeDepth = texture_depth[edgePixel];
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float dist = length(edgePosition - P);

		float l = dist / chamfer_corner_radius;
		l = saturate(l);
		l = pow(l, chamfer_corner_strength);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

		//input_output_normals[DTid.xy] = dist * 10;
		//return;
	}

	normal.xyz = normal.xyz * 0.5 + 0.5;
	input_output_normals[DTid.xy] = normal;
}
