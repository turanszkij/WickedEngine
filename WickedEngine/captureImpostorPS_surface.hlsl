#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 surface_ref_met_emi_sss = xSurfaceMap.Sample(sampler_objectshader, UV);

	float4 surface;
	surface.r = g_xMat_reflectance * surface_ref_met_emi_sss.r;
	surface.g = g_xMat_metalness * surface_ref_met_emi_sss.g;
	surface.b = g_xMat_emissive * surface_ref_met_emi_sss.b;
	surface.a = g_xMat_subsurfaceScattering * surface_ref_met_emi_sss.a;

	return surface;
}

