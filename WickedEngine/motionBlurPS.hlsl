#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float3 color = float3(0,0,0);
	float numSampling = 0.0f; 

	float depth = texture_depth.SampleLevel(sampler_point_clamp, PSIn.tex, 0);
	float3 P = getPosition(PSIn.tex, depth);
	float2 pos2D = PSIn.tex.xy*2-1;
	float4 pos2DPrev = mul(float4(P, 1), g_xFrame_MainCamera_PrevVP);
	pos2DPrev.xy = float2(1,-1)*pos2DPrev.xy/pos2DPrev.w;
	float2 vel = pos2D - pos2DPrev.xy;
	vel *= 0.025f;

	numSampling++;

	[unroll]
	for(float i=-7.5f;i<=7.5f;i+=1.0f){
		color.rgb += xTexture.SampleLevel(Sampler,saturate(PSIn.tex+vel*i*0.5f),0).rgb;
		numSampling++;
	}

	return float4(color/numSampling,1);
}