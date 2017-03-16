#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = texture_gbuffer0.Load(int3(PSIn.pos.xy, 0));
	float emissive = texture_gbuffer2.Load(int3(PSIn.pos.xy, 0)).a;

	float4 g3 = texture_gbuffer3.Load(int3(PSIn.pos.xy, 0));
	float roughness = g3.x;
	float reflectance = g3.y;
	float metalness = g3.z;
	//float ao = g3.a;
	BRDF_HELPER_MAKEINPUTS(color, reflectance, metalness)

	float  depth = texture_lineardepth.Load(int3(PSIn.pos.xy, 0)).r;

	float4 diffuse = texture_0.Load(int3(PSIn.pos.xy, 0)); // light diffuse
	float4 specular = texture_1.Load(int3(PSIn.pos.xy, 0)); // light specular
	float ssao = texture_2.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).r;
	float ao = diffuse.a;
	color.rgb = (GetAmbientColor() * ao * ssao + diffuse.rgb) * albedo + specular.rgb;
	color.rgb += color.rgb * GetEmissive(emissive);

	float fog = getFog((depth));
	color.rgb = applyFog(color.rgb,fog);

	return color;
}