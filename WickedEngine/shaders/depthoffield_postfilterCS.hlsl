#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_main, float3, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_alpha, float, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output_postfilter, float3, 0);
RWTEXTURE2D(output_alpha, unorm float, 1);

float min3(float a, float b, float c)
{
	return min(min(a, b), c);
}

float max3(float a, float b, float c)
{
	return max(max(a, b), c);
}

float med3(float a, float b, float c)
{
	return max(min(a, b), min(max(a, b), c));
}

float2 min3(float2 a, float2 b, float2 c)
{
	return float2(min3(a.x, b.x, c.x), min3(a.y, b.y, c.y));
}

float3 min3(float3 a, float3 b, float3 c)
{
	return float3(min3(a.x, b.x, c.x), min3(a.y, b.y, c.y), min3(a.z, b.z, c.z));
}

float4 min3(float4 a, float4 b, float4 c)
{
	return float4(min3(a.x, b.x, c.x), min3(a.y, b.y, c.y), min3(a.z, b.z, c.z), min3(a.w, b.w, c.w));
}

float2 max3(float2 a, float2 b, float2 c)
{
	return float2(max3(a.x, b.x, c.x), max3(a.y, b.y, c.y));
}

float3 max3(float3 a, float3 b, float3 c)
{
	return float3(max3(a.x, b.x, c.x), max3(a.y, b.y, c.y), max3(a.z, b.z, c.z));
}

float4 max3(float4 a, float4 b, float4 c)
{
	return float4(max3(a.x, b.x, c.x), max3(a.y, b.y, c.y), max3(a.z, b.z, c.z), max3(a.w, b.w, c.w));
}

float2 med3(float2 a, float2 b, float2 c)
{
	return float2(med3(a.x, b.x, c.x), med3(a.y, b.y, c.y));
}

float3 med3(float3 a, float3 b, float3 c)
{
	return float3(med3(a.x, b.x, c.x), med3(a.y, b.y, c.y), med3(a.z, b.z, c.z));
}

float4 med3(float4 a, float4 b, float4 c)
{
	return float4(med3(a.x, b.x, c.x), med3(a.y, b.y, c.y), med3(a.z, b.z, c.z), med3(a.w, b.w, c.w));
}

float min4(float4 values)
{
	return min(min3(values.x, values.y, values.z), values.w);
}

float max4(float4 values)
{
	return max(max3(values.x, values.y, values.z), values.w);
}

static const float2 squareOffsets[] =
{
	{  -1.0, -1.0 },
	{   0.0, -1.0 },
	{   1.0, -1.0 },
	{  -1.0,  0.0 },
	{   0.0,  0.0 },
	{   1.0,  0.0 },
	{  -1.0,  1.0 },
	{   0.0,  1.0 },
	{   1.0,  1.0 },
};

static const float2 circleOffsets[] =
{
	{  0.7071, -0.7071 },
	{ -0.7071,  0.7071 },
	{ -1.0000,  0.0000 },
	{  1.0000,  0.0000 },
	{  0.0000,  0.0000 },
	{ -0.7071, -0.7071 },
	{  0.7071,  0.7071 },
	{ -0.0000, -1.0000 },
	{  0.0000,  1.0000 },
};

void sort3(in out float p1, in out float p2, in out float p3)
{
	float minValue = min3(p1, p2, p3);
	float medValue = med3(p1, p2, p3);
	float maxValue = max3(p1, p2, p3);

	p1 = minValue;
	p2 = medValue;
	p3 = maxValue;
}
void sort3(in out float3 p1, in out float3 p2, in out float3 p3)
{
	float3 minValue = min3(p1, p2, p3);
	float3 medValue = med3(p1, p2, p3);
	float3 maxValue = max3(p1, p2, p3);

	p1 = minValue;
	p2 = medValue;
	p3 = maxValue;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float alpha;
	float3 color;

	const float2 uv = DTid.xy + 0.5f;

	// Median filter [Smith1996]
	{
		float p[9];

		p[0] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[0]) * xPPResolution_rcp, 0);
		p[1] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[1]) * xPPResolution_rcp, 0);
		p[2] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[2]) * xPPResolution_rcp, 0);
		sort3(p[0], p[1], p[2]);

		p[3] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[3]) * xPPResolution_rcp, 0);
		p[4] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[4]) * xPPResolution_rcp, 0);
		p[5] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[5]) * xPPResolution_rcp, 0);
		sort3(p[3], p[4], p[5]);

		p[6] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[6]) * xPPResolution_rcp, 0);
		p[7] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[7]) * xPPResolution_rcp, 0);
		p[8] = texture_alpha.SampleLevel(sampler_point_clamp, (uv + circleOffsets[8]) * xPPResolution_rcp, 0);
		sort3(p[6], p[7], p[8]);

		p[6] = max3(p[0], p[3], p[6]);
		p[2] = min3(p[2], p[5], p[8]);

		sort3(p[1], p[4], p[7]);
		sort3(p[2], p[4], p[6]);

		alpha = p[4];
	}

	{
		float3 p[9];

		p[0] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[0]) * xPPResolution_rcp, 0);
		p[1] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[1]) * xPPResolution_rcp, 0);
		p[2] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[2]) * xPPResolution_rcp, 0);
		sort3(p[0], p[1], p[2]);

		p[3] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[3]) * xPPResolution_rcp, 0);
		p[4] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[4]) * xPPResolution_rcp, 0);
		p[5] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[5]) * xPPResolution_rcp, 0);
		sort3(p[3], p[4], p[5]);

		p[6] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[6]) * xPPResolution_rcp, 0);
		p[7] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[7]) * xPPResolution_rcp, 0);
		p[8] = texture_main.SampleLevel(sampler_point_clamp, (uv + circleOffsets[8]) * xPPResolution_rcp, 0);
		sort3(p[6], p[7], p[8]);

		p[6] = max3(p[0], p[3], p[6]);
		p[2] = min3(p[2], p[5], p[8]);

		sort3(p[1], p[4], p[7]);
		sort3(p[2], p[4], p[6]);

		color = p[4];
	}

	output_postfilter[DTid.xy] = color;
	output_alpha[DTid.xy] = alpha;
	//output_postfilter[DTid.xy] = texture_main[DTid.xy];
}