#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float4 surface_ao_roughness_metallic_reflectance = xSurfaceMap.Sample(sampler_objectshader, UV);

	float4 surface;
	surface.r = surface_ao_roughness_metallic_reflectance.r;
	surface.g = g_xMat_roughness * surface_ao_roughness_metallic_reflectance.g;
	surface.b = g_xMat_metalness * surface_ao_roughness_metallic_reflectance.b;
	surface.a = g_xMat_reflectance * surface_ao_roughness_metallic_reflectance.a;

	return surface;
}

