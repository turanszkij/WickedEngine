#include "postProcessHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float3 color = float3(0,0,0);
	float numSampling = 0.0f;

	float2 vel = texture_gbuffer2.SampleLevel(Sampler, PSIn.tex, 0).xy - 0.5;
	//return float4(vel,0,1);
	vel *= 0.06f;

	numSampling++;

	[unroll]
	for(float i=-7.5f;i<=7.5f;i+=1.0f){
		color.rgb += xTexture.SampleLevel(Sampler,saturate(PSIn.tex+vel*i*0.5f),0).rgb;
		numSampling++;
	}

	return float4(color/numSampling,1);
}