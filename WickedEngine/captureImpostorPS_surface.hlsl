#include "objectHF.hlsli"

float4 main(PixelInputType input) : SV_Target0
{
	float4 surface_occlusion_roughness_metallic_reflectance;
	[branch]
	if (g_xMat_uvset_surfaceMap >= 0)
	{
		const float2 UV_surfaceMap = g_xMat_uvset_surfaceMap == 0 ? input.uvsets.xy : input.uvsets.zw;
		surface_occlusion_roughness_metallic_reflectance = xSurfaceMap.Sample(sampler_objectshader, UV_surfaceMap);

		if (g_xMat_specularGlossinessWorkflow)
		{
			ConvertToSpecularGlossiness(surface_occlusion_roughness_metallic_reflectance);
		}
	}
	else
	{
		surface_occlusion_roughness_metallic_reflectance = 1;
	}

	float4 surface;
	surface.r = surface_occlusion_roughness_metallic_reflectance.r;
	surface.g = g_xMat_roughness * surface_occlusion_roughness_metallic_reflectance.g;
	surface.b = g_xMat_metalness * surface_occlusion_roughness_metallic_reflectance.b;
	surface.a = g_xMat_reflectance * surface_occlusion_roughness_metallic_reflectance.a;

	return surface;
}

