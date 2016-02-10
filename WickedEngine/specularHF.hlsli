#ifndef _SPECULAR_HF_
#define _SPECULAR_HF_

#include "toonHF.hlsli"
#include "gammaHF.hlsli"

inline void applySpecular(inout float4 color, in float4 lightColor, in float3 N, in float3 V, in float3 lightDir
						  , in float fresnelValue, in int specular_power, in float intensity, in bool toonshaded)
{
	if(specular_power){
		float3 reflectionVector = normalize(lightDir.xyz - V.xyz);
		float spec = saturate( dot(reflectionVector, N) );
		spec = pow(spec, specular_power)*intensity;

		if(toonshaded) toon(spec);

		color.rgb += (lightColor.rgb * spec);
		color.rgb*=pow(abs(1+spec),GAMMA);
	}
}

#endif // _SPECULAR_HF_
