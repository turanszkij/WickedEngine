#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "objectHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = texture_gbuffer0[uint2(PSIn.pos.xy)];
	float emissive = texture_gbuffer2[uint2(PSIn.pos.xy)].a;

	float4 g3 = texture_gbuffer3[int2(PSIn.pos.xy)];
	float roughness = g3.x;
	float reflectance = g3.y;
	float metalness = g3.z;
	float3 albedo = ComputeAlbedo(color, metalness, reflectance);

	float  depth = texture_lineardepth[(uint2)PSIn.pos.xy].r * g_xFrame_MainCamera_ZFarP;

	float4 diffuse = texture_0[uint2(PSIn.pos.xy)]; // light diffuse
	float4 specular = texture_1[uint2(PSIn.pos.xy)]; // light specular
	float ssao = texture_2.SampleLevel(sampler_linear_clamp, PSIn.tex.xy, 0).r;
	float ao = diffuse.a;
	color.rgb = (GetAmbientColor() * ao * ssao + diffuse.rgb) * albedo + specular.rgb;
	color.rgb += color.rgb * GetEmissive(emissive);

	ApplyFog(depth, color);

	return color;
}