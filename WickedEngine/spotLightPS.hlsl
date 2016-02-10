#include "lightHF.hlsli"
#include "spotLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{
	DEFERREDLIGHT_MAKEPARAMS(lightColor)

	float attenuation;
	float3 lightToPixel;
	float lightInt = spotLight(P, N, attenuation, lightToPixel, toonshaded);
	color *= lightInt;
	applySpecular(color, color * lightInt, N, V, lightToPixel, 1, specular_power, specular, toonshaded);
	color *= attenuation;

	DEFERREDLIGHT_RETURN
}