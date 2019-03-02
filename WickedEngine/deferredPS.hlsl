#include "postProcessHF.hlsli"
#include "fogHF.hlsli"
#include "reconstructPositionHF.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "objectHF.hlsli"
#include "normalsCompressHF.hlsli"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 g0 = texture_gbuffer0[PSIn.pos.xy];
	float4 g2 = texture_gbuffer2[PSIn.pos.xy];
	float3 albedo = ComputeAlbedo(float4(g0.rgb, 1), g2.b, g2.a);

	float  depth = texture_lineardepth[(uint2)PSIn.pos.xy].r * g_xFrame_MainCamera_ZFarP;

	float4 diffuse = texture_0[uint2(PSIn.pos.xy)]; // light diffuse
	float4 specular = texture_1[uint2(PSIn.pos.xy)]; // light specular

	float4 color = float4(diffuse.rgb * albedo + specular.rgb, 1);

	ApplyFog(depth, color);

	return color;
}