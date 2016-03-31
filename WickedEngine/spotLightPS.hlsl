#include "lightHF.hlsli"
#include "spotLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{
	DEFERREDLIGHT_MAKEPARAMS(lightColor)

	float attenuation;
	float3 lightToPixel;
	float lightInt = spotLight(P, N, attenuation, lightToPixel);
	color *= lightInt;
	applySpecular(color, (color * lightInt).rgb, N, V, lightToPixel, 1, specular_power, specular);
	color *= attenuation;

	DEFERREDLIGHT_RETURN
}