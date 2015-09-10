#include "postProcessHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float3 color = float3(0,0,0);
	float numSampling = 0.0f;

	float2 vel = xSceneVelocityMap.SampleLevel(Sampler, PSIn.tex, 0).xy / xDimension.xy * 50;

	numSampling++;

	for(float i=0;i<=5;i+=0.5){
		color.rgb += xTexture.SampleLevel(Sampler,PSIn.tex+vel*i,0).rgb;
		numSampling++;
	}

	return float4(color/numSampling,1);
}