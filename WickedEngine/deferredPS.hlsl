#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"
#include "brdf.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 baseColor_emissive = texture_gbuffer0.Load(int3(PSIn.pos.xy, 0));
	float4 color = float4(baseColor_emissive.rgb,1);

	float2 reflectance_metalness = texture_gbuffer2.Load(int3(PSIn.pos.xy, 0)).zw;
	BRDF_HELPER_MAKEINPUTS(baseColor_emissive.rgb, reflectance_metalness.x, reflectance_metalness.y)

	float  depth = texture_lineardepth.Load(int3(PSIn.pos.xy, 0)).r;

	[branch]if(depth<g_xCamera_ZFarP)
	{
		float3 diffuse = texture_0.SampleLevel(sampler_point_clamp, PSIn.tex, 0).rgb; // light diffuse
		float3 specular = texture_1.SampleLevel(sampler_point_clamp, PSIn.tex, 0).rgb; // light specular
		float ao = texture_2.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).r; // ao
		color.rgb = (GetAmbientColor() * ao + diffuse) * albedo + specular;
		color.rgb += baseColor_emissive.rgb * baseColor_emissive.a;

		float fog = getFog((depth));
		color.rgb = applyFog(color.rgb,fog);
	}

	return color;
}