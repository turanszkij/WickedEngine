#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float3 color = float3(0,0,0);
	float numSampling = 0.0f;

	float2 vel = texture_gbuffer1.Load(uint3(PSIn.pos.xy, 0)).zw;
	vel *= 0.025f;

	numSampling++;

	[unroll]
	for(float i=-7.5f;i<=7.5f;i+=1.0f){
		color.rgb += xTexture.SampleLevel(Sampler,saturate(PSIn.tex+vel*i*0.5f),0).rgb;
		numSampling++;
	}

	return float4(color/numSampling,1);
}