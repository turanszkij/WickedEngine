#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float2 velocity = texture_gbuffer1.Load(uint3(PSIn.pos.xy,0)).zw;
	float2 prevTC = PSIn.tex + velocity;

	float4 neighborhood[9];
	neighborhood[0] = xTexture.Load(uint3(PSIn.pos.xy + float2(-1, -1),	0));
	neighborhood[1] = xTexture.Load(uint3(PSIn.pos.xy + float2(0, -1),	0));
	neighborhood[2] = xTexture.Load(uint3(PSIn.pos.xy + float2(1, -1),	0));
	neighborhood[3] = xTexture.Load(uint3(PSIn.pos.xy + float2(-1, 0),	0));
	neighborhood[4] = xTexture.Load(uint3(PSIn.pos.xy + float2(0, 0),	0)); // center
	neighborhood[5] = xTexture.Load(uint3(PSIn.pos.xy + float2(1, 0),	0));
	neighborhood[6] = xTexture.Load(uint3(PSIn.pos.xy + float2(-1, 1),	0));
	neighborhood[7] = xTexture.Load(uint3(PSIn.pos.xy + float2(0, 1),	0));
	neighborhood[8] = xTexture.Load(uint3(PSIn.pos.xy + float2(1, 1),	0));
	float4 neighborhoodMin = neighborhood[0];
	float4 neighborhoodMax = neighborhood[0];
	[unroll]
	for (uint i = 1; i < 9; ++i)
	{
		neighborhoodMin = min(neighborhoodMin, neighborhood[i]);
		neighborhoodMax = max(neighborhoodMax, neighborhood[i]);
	}

	float4 history = xMaskTex.SampleLevel(sampler_linear_clamp, prevTC, 0);
	history = clamp(history, neighborhoodMin, neighborhoodMax);

	float4 current = neighborhood[4];

	float blendfactor = saturate(lerp(0.05f, 1.0f, length(velocity) * 10)); // todo

	return float4(lerp(history.rgb, current.rgb, blendfactor), 1);
}