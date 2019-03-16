#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	const float2 UV_surfaceMap = g_xMat_uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
	float4 surface_ao_roughness_metallic_reflectance = xSurfaceMap.Sample(sampler_objectshader, UV_surfaceMap);

	if (g_xMat_specularGlossinessWorkflow)
	{
		ConvertToSpecularGlossiness(surface_ao_roughness_metallic_reflectance);
	}

	float4 surface;
	surface.r = surface_ao_roughness_metallic_reflectance.r;
	surface.g = g_xMat_roughness * surface_ao_roughness_metallic_reflectance.g;
	surface.b = g_xMat_metalness * surface_ao_roughness_metallic_reflectance.b;
	surface.a = g_xMat_reflectance * surface_ao_roughness_metallic_reflectance.a;

	return surface;
}

