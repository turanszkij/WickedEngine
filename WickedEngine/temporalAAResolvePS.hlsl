#include "postProcessHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float2 velocity = texture_gbuffer1.Load(uint3(PSIn.pos.xy,0)).zw;

	float2 prevTC = PSIn.tex + velocity;
	float2 dim;
	xMaskTex.GetDimensions(dim.x, dim.y);
	float2 texelSize = 1.0f / dim;
	float4 neighborhood[9];
	neighborhood[0] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(-1, -1), 0);
	neighborhood[1] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(0, -1), 0);
	neighborhood[2] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(1, -1), 0);
	neighborhood[3] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(-1, 0), 0);
	neighborhood[4] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC, 0); // center
	neighborhood[5] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(1, 0), 0);
	neighborhood[6] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(-1, 1), 0);
	neighborhood[7] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(0, 1), 0);
	neighborhood[8] = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC + texelSize * uint2(1, 1), 0);
	float4 neighborhoodMin = neighborhood[0];
	float4 neighborhoodMax = neighborhood[0];
	[unroll]
	for (uint i = 1; i < 9; ++i)
	{
		neighborhoodMin = min(neighborhoodMin, neighborhood[i]);
		neighborhoodMax = max(neighborhoodMax, neighborhood[i]);
	}
	float4 history = clamp(neighborhood[4], neighborhoodMin, neighborhoodMax);

	//float4 history = xMaskTex.Load(uint3(PSIn.pos.xy, 0));
	//float4 history = xMaskTex.SampleLevel(sampler_linear_clamp, PSIn.tex + velocity, 0);

	float4 current = xTexture.Load(uint3(PSIn.pos.xy,0));

	return float4(lerp(history.rgb, current.rgb, 0.05f), 1);
}