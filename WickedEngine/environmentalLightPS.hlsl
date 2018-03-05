#include "deferredLightHF.hlsli"



LightOutputType main(VertexToPixel PSIn)
{
	DEFERREDLIGHT_MAKEPARAMS

	diffuse = 0;
	float envMapMIP = roughness * g_xWorld_EnvProbeMipCount;
	specular = max(0, EnvironmentReflection_Global(surface, envMapMIP));
	VoxelGI(surface, diffuse, specular, ao);

	specular *= surface.F;

	DEFERREDLIGHT_RETURN
}