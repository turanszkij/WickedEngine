#ifndef CHAMFERNORMALS_UTIL_H
#define CHAMFERNORMALS_UTIL_H

// Overridable functions:
//	A sample implementation of these functions can be seen below:
#ifdef CHAMFERNORMALS_COMPILE_USER_PROTOTYPES
cbuffer ChamferNormalsCB
{
	uint2 xPPResolution;
	float2 xPPResolution_rcp;
	float4 xPPParams0;
	float4x4 g_xCamera_InvVP;
};
Texture2D<float4> input_normals;
Texture2D<uint4> input_edgeMask;
RWTexture2D<uint4> output_edgeMask;
RWTexture2D<unorm float4> output_normals;

// resolution of Gbuffer normals texture
uint2 GetResolution()
{
	return xPPResolution; 
}

// 1.0f / resolution of Gbuffer normals texture
float2 GetResolutionRcp()
{
	return xPPResolution_rcp;
}

// it should return pow(2.0f, float(passcount - current_pass - 1));
//	where passcount is the number of passes: (int)ceil(log2((float)max(resolutionWidth, resolutionHeight)));
//	and current_pass is the zero-based pass number
float GetJumpFloodRadius()
{
	return xPPParams0.x;
}

// inverse of camera's view-projection matrix (column-major layout is assumed)
float4x4 GetCameraInverseViewProjection()
{
	return g_xCamera_InvVP;
}

float LoadDepth(uint2 pixel)
{
	return texture_depth[pixel];
}
float LoadLinearDepth(uint2 pixel)
{
	return texture_lineardepth[pixel];
}
float4 LoadNormal(uint2 pixel)
{
	float4 normal = input_normals[pixel];
	normal.xyz = normalize(normal.xyz * 2 - 1);
	return normal;
}
void StoreNormal(uint2 pixel, float4 normal)
{
	normal.xyz = normal.xyz * 0.5 + 0.5;
	output_normals[pixel] = normal.xyz * 0.5 + 0.5;
}
uint4 LoadEdgeMap(uint2 pixel)
{
	return input_edgeMap[pixel];
}
void StoreEdgeMap(uint2 pixel, uint4 edgemap)
{
	output_edgeMap[DTid.xy] = edgemap;
}
#endif // CHAMFERNORMALS_COMPILE_USER_PROTOTYPES



//// Helper functions:
//inline uint pack_unitvector(in float3 value)
//{
//	uint retVal = 0;
//	retVal |= (uint)((value.x * 0.5 + 0.5) * 255.0) << 0u;
//	retVal |= (uint)((value.y * 0.5 + 0.5) * 255.0) << 8u;
//	retVal |= (uint)((value.z * 0.5 + 0.5) * 255.0) << 16u;
//	return retVal;
//}
//inline float3 unpack_unitvector(in uint value)
//{
//	float3 retVal;
//	retVal.x = (float)((value >> 0u) & 0xFF) / 255.0 * 2 - 1;
//	retVal.y = (float)((value >> 8u) & 0xFF) / 255.0 * 2 - 1;
//	retVal.z = (float)((value >> 16u) & 0xFF) / 255.0 * 2 - 1;
//	return retVal;
//}
//inline uint pack_pixel(uint2 pixel)
//{
//	uint retVal = 0;
//	retVal |= pixel.x & 0xFFFF;
//	retVal |= (pixel.y & 0xFFFF) << 16;
//	return retVal;
//}
//inline uint2 unpack_pixel(uint value)
//{
//	uint2 retVal;
//	retVal.x = value & 0xFFFF;
//	retVal.y = (value >> 16) & 0xFFFF;
//	return retVal;
//}
//
//// Reconstructs world-space position from depth buffer
////	uv		: screen space coordinate in [0, 1] range
////	z		: depth value at current pixel
////	InvVP	: Inverse of the View-Projection matrix that was used to generate the depth value
//inline float3 reconstructPosition(in float2 uv, in float z, in float4x4 InvVP)
//{
//	float x = uv.x * 2 - 1;
//	float y = (1 - uv.y) * 2 - 1;
//	float4 position_s = float4(x, y, z, 1);
//	float4 position_v = mul(InvVP, position_s);
//	return position_v.xyz / position_v.w;
//}
//inline float3 reconstructPosition(in float2 uv, in float z)
//{
//	return reconstructPosition(uv, z, GetCameraInverseViewProjection());
//}




// Shader base implementations below:

// Pass 1: Edge detection implementation
#ifdef CHAMFERNORMALS_EDGEDETECT_IMPLEMENTATION
static const int2 offsets[3] = {
	int2(1, 0),
	int2(0, 1),
	int2(1, 1),
};

static float detect_edge_threshold = 0.1f; // lower: less chance for detecting edges
static float detect_corner_threshold = 0.8f; // higher: less chance for detecting corners
static float reject_depth_threshold = 0.001f; // depth difference above this will be detected as separate surface without edge

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 normal = LoadNormal(DTid.xy);

	const float lineardepth = LoadLinearDepth(DTid.xy);

	float3 edgeNormal = 0;
	for (uint i = 0; i < 3; ++i)
	{
		const int2 pixel2 = DTid.xy + offsets[i];

		const float ld = LoadLinearDepth(pixel2);
		const float diff = abs(lineardepth - ld);

		if (diff < reject_depth_threshold)
		{
			const float3 n = LoadNormal(pixel2).xyz;

			if (abs(dot(normal.xyz, n.xyz)) < detect_edge_threshold)
			{
				edgeNormal = normalize(normal.xyz + n.xyz);
				break;
			}
		}
	}

	uint packed_edgeNormal = 0;
	uint packed_cornerNormal = 0;
	uint packed_pixel = 0;
	if (any(edgeNormal))
	{
		packed_pixel = pack_pixel(DTid.xy);
		packed_edgeNormal = pack_unitvector(edgeNormal);

		float3 cornerNormal = 0;

		float3 N = normal.xyz;
		float3 NR = input_normals[max(0, min(DTid.xy + int2(1, 0), GetResolution() - 1))].xyz * 2 - 1;
		float3 ND = input_normals[max(0, min(DTid.xy + int2(0, 1), GetResolution() - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}
		NR = input_normals[max(0, min(DTid.xy + int2(-1, 0), GetResolution() - 1))].xyz * 2 - 1;
		ND = input_normals[max(0, min(DTid.xy + int2(0, 1), GetResolution() - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}
		NR = input_normals[max(0, min(DTid.xy + int2(1, 0), GetResolution() - 1))].xyz * 2 - 1;
		ND = input_normals[max(0, min(DTid.xy + int2(0, -1), GetResolution() - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}

		if (any(cornerNormal))
		{
			packed_cornerNormal = pack_unitvector(normalize(N + cornerNormal));
		}
	}
	StoreEdgeMap(DTid.xy, uint4(packed_edgeNormal, packed_pixel, packed_cornerNormal, packed_pixel));
}
#endif // CHAMFERNORMALS_EDGEDETECT_IMPLEMENTATION

// Pass 2: Jump flooding implementation:
#ifdef CHAMFERNORMALS_JUMPFLOOD_IMPLEMENTATION
static const int2 offsets[9] = {
	int2(-1, -1), int2(-1, 0), int2(-1, 1),
	int2(0, -1), int2(0, 0), int2(0, 1),
	int2(1, -1), int2(1, 0), int2(1, 1),
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5) * GetResolutionRcp();

	const float lineardepth = LoadLinearDepth(DTid.xy);

	const float depth = LoadDepth(DTid.xy);
	const float3 P = reconstructPosition(uv, depth);

	float bestEdgeDistance = FLT_MAX;
	uint2 bestEdge = 0;
	float bestCornerDistance = FLT_MAX;
	uint2 bestCorner = 0;

	for (uint i = 0; i < 9; ++i)
	{
		const int2 pixel = (int2)DTid.xy + int2(offsets[i] * GetJumpFloodRadius());
		const uint4 edgeMap = LoadEdgeMap(pixel);

		const uint2 edge = edgeMap.xy;
		if (edge.x)
		{
			const uint2 edgePixel = unpack_pixel(edge.y);
			const float2 edgeUV = (edgePixel.xy + 0.5) * GetResolutionRcp();

			const float edgeDepth = LoadDepth(edgePixel);
			const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);
			const float dist = length(edgePosition - P);

			if (dist < bestEdgeDistance)
			{
				bestEdgeDistance = dist;
				bestEdge = edge;
			}

		}

		const uint2 corner = edgeMap.zw;
		if (corner.x)
		{
			const uint2 cornerPixel = unpack_pixel(corner.y);
			const float2 cornerUV = (cornerPixel.xy + 0.5) * GetResolutionRcp();

			const float cornerDepth = LoadDepth(cornerPixel);
			const float3 cornerPosition = reconstructPosition(cornerUV, cornerDepth);
			const float dist = length(cornerPosition - P);

			if (dist < bestCornerDistance)
			{
				bestCornerDistance = dist;
				bestCorner = corner;
			}

		}
	}
	StoreEdgeMap(DTid.xy, uint4(bestEdge, bestCorner));
}
#endif // CHAMFERNORMALS_JUMPFLOOD_IMPLEMENTATION


#ifdef CHAMFERNORMALS_TWISTNORMAL_IMPLEMENTATION
static const float chamfer_edge_radius = 0.1; // radius of the edge chamfering in world space
static const float chamfer_edge_strength = 1; // falloff for edge smoothness (lower: smaller chamfer, higher: stronger chamfer)

static const float chamfer_corner_radius = 0.2; // radius of the corner chamfering in world space
static const float chamfer_corner_strength = 3; // falloff for corner smoothness (lower: smaller chamfer, higher: stronger chamfer)

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * GetResolutionRcp();
	const float depth = LoadDepth(DTid.xy);
	const float3 P = reconstructPosition(uv, depth);

	float4 normal = LoadNormal(DTid.xy);

	const uint4 edgeMap = LoadEdgeMap(DTid.xy);

	// edges:
	uint2 closestEdge = edgeMap.xy;
	if (closestEdge.x)
	{
		const float3 edgeNormal = unpack_unitvector(closestEdge.x);
		const uint2 edgePixel = unpack_pixel(closestEdge.y);
		const float2 edgeUV = (edgePixel.xy + 0.5f) * GetResolutionRcp();

		const float edgeDepth = LoadDepth(edgePixel);
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float dist = length(edgePosition - P);

		float l = dist / chamfer_edge_radius;
		l = saturate(l);
		l = pow(l, chamfer_edge_strength);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

#ifdef CHAMFERNORMALS_TWISTNORMAL_DEBUG_EDGES
		input_output_normals[DTid.xy] = dist * 10;
		return;
#endif // CHAMFERNORMALS_TWISTNORMAL_DEBUG_EDGES
	}

	// corners:
	closestEdge = edgeMap.zw;
	if (closestEdge.x)
	{
		const float3 edgeNormal = unpack_unitvector(closestEdge.x);
		const uint2 edgePixel = unpack_pixel(closestEdge.y);
		const float2 edgeUV = (edgePixel.xy + 0.5f) * GetResolutionRcp();

		const float edgeDepth = LoadDepth(edgePixel);
		const float3 edgePosition = reconstructPosition(edgeUV, edgeDepth);

		const float dist = length(edgePosition - P);

		float l = dist / chamfer_corner_radius;
		l = saturate(l);
		l = pow(l, chamfer_corner_strength);
		normal.xyz = lerp(edgeNormal, normal.xyz, l);
		normal.xyz = normalize(normal.xyz);

#ifdef CHAMFERNORMALS_TWISTNORMAL_DEBUG_CORNERS
		input_output_normals[DTid.xy] = dist * 10;
		return;
#endif // CHAMFERNORMALS_TWISTNORMAL_DEBUG_CORNERS
	}

	StoreNormal(DTid.xy, normal);
}
#endif // CHAMFERNORMALS_TWISTNORMAL_IMPLEMENTATION


#endif // CHAMFERNORMALS_UTIL_H
