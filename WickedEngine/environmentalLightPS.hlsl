#include "deferredLightHF.hlsli"

#define	xSSAO texture_8
#define	xSSR texture_9


LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	diffuse = 0;
	float envMapMIP = roughness * g_xFrame_EnvProbeMipCount;
	specular = max(0, EnvironmentReflection_Global(surface, envMapMIP));

	VoxelGI(surface, diffuse, specular, ao);
	float ssao = xSSAO.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0).r;
	float3 ambient = GetAmbient(N) * ao * ssao;
	diffuse += ambient;

	float4 ssr = xSSR.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	specular = lerp(specular, ssr.rgb, ssr.a);

	specular *= surface.F;

	DEFERREDLIGHT_RETURN
}