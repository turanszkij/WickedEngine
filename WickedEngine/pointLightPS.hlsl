#include "lightHF.hlsli"
#include "pointLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{ 
	DEFERREDLIGHT_MAKEPARAMS(lightColor)

	float3 lightDir;
	float attenuation;
	float lightInt = pointLight(P, N, lightDir, attenuation);
	color *= lightInt;
	applySpecular(color, (color * lightInt).rgb, N, V, lightDir, 1, specular_power, specular);
	color *= attenuation;

	DEFERREDLIGHT_RETURN
}