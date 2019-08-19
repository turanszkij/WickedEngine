#include "deferredLightHF.hlsli"

TEXTURE2D(texture_ssao, float, TEXSLOT_RENDERPATH_SSAO);
TEXTURE2D(texture_ssr, float4, TEXSLOT_RENDERPATH_SSR);


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;
	lighting.indirect.specular = max(0, EnvironmentReflection_Global(surface, envMapMIP));

	LightingContribution vxgi_contribution = VoxelGI(surface, lighting);

	float4 ssr = texture_ssr.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, surface.roughness * 5);
	lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb, ssr.a);

	float ssao = texture_ssao.SampleLevel(sampler_linear_clamp, ScreenCoord, 0).r;
	surface.occlusion = ssao;

	// Blend factor modulation:
	//	Alpha is not written, but blending will multiply the dest rgb with 1-alpha
	//	Modulate alpha to overwrite dest (which contains ambient and light map before this pass)
	ssao = saturate(1 - ssao);
	diffuse_alpha = max(vxgi_contribution.diffuse, ssao);
	specular_alpha = max(max(vxgi_contribution.specular, ssr.a), ssao);

	DEFERREDLIGHT_RETURN
}