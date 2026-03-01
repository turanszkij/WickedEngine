#include "globals.hlsli"
#include "ShaderInterop_GaussianSplat.h"

StructuredBuffer<GaussianSplat> splats : register(t0);
StructuredBuffer<uint> sortedIndexBuffer : register(t1);

static const float3 BILLBOARD[] = {
	float3(-1, -1, 0),	// 0
	float3(1, -1, 0),	// 1
	float3(-1, 1, 0),	// 2
	float3(1, 1, 0),	// 4
};

static const float sqrt8    = sqrt(8.0);
static const float SH_C1    = 0.4886025119029199f;
static const float SH_C2[5] = { 1.0925484, -1.0925484, 0.3153916, -1.0925484, 0.5462742 };
static const float SH_C3[7] = { -0.5900435899266435f, 2.890611442640554f, -0.4570457994644658f, 0.3731763325901154f, -0.4570457994644658f, 1.445305721320277f, -0.5900435899266435f };

float3 fetchViewDependentRadiance(in uint splatIndex, in float3 worldViewDir)
{
	// contribution is null if MAX_SH_DEGREE < 1
	float3 rgb = float3(0.0, 0.0, 0.0);

	// SH coefficients for degree 1 (1,2,3)
	float3 shd1[3] = {
		splats[splatIndex].f_rest[0],
		splats[splatIndex].f_rest[1],
		splats[splatIndex].f_rest[2],
	};
	// SH coefficients for degree 2 (4 5 6 7 8)
	float3 shd2[5] = {
		splats[splatIndex].f_rest[3],
		splats[splatIndex].f_rest[4],
		splats[splatIndex].f_rest[5],
		splats[splatIndex].f_rest[6],
		splats[splatIndex].f_rest[7],
	};
	// SH coefficients for degree 3 (9,10,11,12,13,14,15)
	float3 shd3[7] = {
		splats[splatIndex].f_rest[8],
		splats[splatIndex].f_rest[9],
		splats[splatIndex].f_rest[10],
		splats[splatIndex].f_rest[11],
		splats[splatIndex].f_rest[12],
		splats[splatIndex].f_rest[13],
		splats[splatIndex].f_rest[14],
	};

	const float x = worldViewDir.x;
	const float y = worldViewDir.y;
	const float z = worldViewDir.z;

	// Degree 1 contributions
	rgb += SH_C1 * (-shd1[0] * y + shd1[1] * z - shd1[2] * x);

	const float xx = x * x;
	const float yy = y * y;
	const float zz = z * z;
	const float xy = x * y;
	const float yz = y * z;
	const float xz = x * z;

	// Degree 2 contributions
	rgb += (SH_C2[0] * xy) * shd2[0] + (SH_C2[1] * yz) * shd2[1] + (SH_C2[2] * (2.0 * zz - xx - yy)) * shd2[2]
			+ (SH_C2[3] * xz) * shd2[3] + (SH_C2[4] * (xx - yy)) * shd2[4];

	const float xyy = x * yy;
	const float yzz = y * zz;
	const float zxx = z * xx;
	const float xyz = x * y * z;

	// Degree 3 contributions
	rgb += SH_C3[0] * shd3[0] * (3.0 * x * x - y * y) * y + SH_C3[1] * shd3[1] * x * y * z
			+ SH_C3[2] * shd3[2] * (4.0 * z * z - x * x - y * y) * y
			+ SH_C3[3] * shd3[3] * z * (2.0 * z * z - 3.0 * x * x - 3.0 * y * y)
			+ SH_C3[4] * shd3[4] * x * (4.0 * z * z - x * x - y * y) + SH_C3[5] * shd3[5] * (x * x - y * y) * z
			+ SH_C3[6] * shd3[6] * x * (x * x - 3.0 * y * y);

	return rgb;
}

void main(in uint vertexID : SV_VertexID, in uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out float2 localPos : LOCALPOS, out float3 color : COLOR, out uint splatID : SPLATID)
{
	splatID = sortedIndexBuffer[instanceID];

	float3 position = splats[splatID].position;
	float3 scale = splats[splatID].scale;
	float4 quaternion = splats[splatID].rotation;

	float3 quadPos = BILLBOARD[vertexID];
	quadPos.xy *= scale.xy;
	quadPos = rotate_vector(quadPos, quaternion);

	pos = float4(position + quadPos, 1);
	pos = mul(GetCamera().view_projection, pos);

	localPos = BILLBOARD[vertexID] * sqrt8;

	color = splats[splatID].f_dc;
	float3 viewDir = normalize(position - GetCamera().position);
	color += fetchViewDependentRadiance(splatID, viewDir);
}
