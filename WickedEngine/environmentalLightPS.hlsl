#include "deferredLightHF.hlsli"

#define	xSSR texture_9


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;
	lighting.indirect.specular = max(0, EnvironmentReflection_Global(surface, envMapMIP));

	LightingContribution vxgi_contribution = VoxelGI(surface, lighting);

	float4 ssr = xSSR.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	lighting.indirect.specular = lerp(lighting.indirect.specular, ssr.rgb, ssr.a);

	diffuse_alpha = vxgi_contribution.diffuse;
	specular_alpha = max(vxgi_contribution.specular, ssr.a);

	DEFERREDLIGHT_RETURN
}