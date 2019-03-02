#include "objectHF.hlsli"
#include "skyHF.hlsli"

struct VSOut {
	float4 pos : SV_POSITION;
	float3 nor : TEXCOORD0;
	float4 pos2D : SCREENPOSITION;
	float4 pos2DPrev : SCREENPOSITIONPREV;
};

GBUFFEROutputType_Thin main(VSOut input)
{
	float3 normal = normalize(input.nor);

	float4 color = float4(DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, normal, 0).rgb), 1);
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy / input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	GBUFFEROutputType_Thin Out;
	Out.g0 = color;
	Out.g1 = float4(0, 0, velocity);
	return Out;
}
