#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "objectHF.hlsli"
#include "normalsCompressHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = texture_gbuffer0[uint2(PSIn.pos.xy)];

	float4 g2 = texture_gbuffer2[int2(PSIn.pos.xy)];
	float roughness = g2.x;
	float reflectance = g2.y;
	float metalness = g2.z;
	float3 albedo = ComputeAlbedo(color, metalness, reflectance);

	float  depth = texture_lineardepth[(uint2)PSIn.pos.xy].r * g_xFrame_MainCamera_ZFarP;

	float4 diffuse = texture_0[uint2(PSIn.pos.xy)]; // light diffuse
	float4 specular = texture_1[uint2(PSIn.pos.xy)]; // light specular
	color.rgb = diffuse.rgb * albedo + specular.rgb;

	ApplyFog(depth, color);

	return color;
}