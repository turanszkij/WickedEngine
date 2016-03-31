#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = texture_gbuffer0.Load(int3(PSIn.pos.xy, 0));
	float  depth = texture_lineardepth.Load(int3(PSIn.pos.xy, 0)).r;

	[branch]if(depth<g_xCamera_ZFarP){
		float4 lighting = texture_0.SampleLevel(sampler_point_clamp,PSIn.tex,0); // light
		color.rgb *= lighting.rgb + GetAmbientColor();
		color.rgb *= texture_1.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).rrr; // ao

		float fog = getFog((depth));
		color.rgb = applyFog(color.rgb,fog);
	}

	return color;
}