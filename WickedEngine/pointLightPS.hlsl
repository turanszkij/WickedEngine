#include "lightHF.hlsli"
#include "pointLightHF.hlsli"


float4 main(VertexToPixel PSIn) : SV_TARGET
{ 
	DEFERREDLIGHT_MAKEPARAMS(lightColor)

	float3 lightDir;
	float attenuation;
	float lightInt = pointLight(pos3D, normal, lightDir, attenuation, toonshaded);
	color *= lightInt;
	applySpecular(color, color * lightInt, normal, eyevector, lightDir, 1, specular_power, specular, toonshaded);
	color *= attenuation;

	DEFERREDLIGHT_RETURN
}