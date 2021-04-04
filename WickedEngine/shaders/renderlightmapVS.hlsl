#include "globals.hlsli"
#include "ShaderInterop_Raytracing.h"

struct Input
{
	float4 pos : POSITION_NORMAL_WIND;
	float2 atl : ATLAS;
};

struct Output
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

Output main(Input input)
{
	Output output;

	output.pos = float4(input.atl, 0, 1);
	output.pos.xy = output.pos.xy * 2 - 1;
	output.pos.y *= -1;
	output.pos.xy += xTracePixelOffset;

	output.uv = input.atl;

	output.pos3D = mul(g_xTransform, float4(input.pos.xyz, 1)).xyz;

	uint normal_wind = asuint(input.pos.w);
	output.normal.x = (float)((normal_wind >> 0) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	output.normal.y = (float)((normal_wind >> 8) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	output.normal.z = (float)((normal_wind >> 16) & 0x000000FF) / 255.0f * 2.0f - 1.0f;
	output.normal = mul((float3x3)g_xTransform, output.normal);

	return output;
}
