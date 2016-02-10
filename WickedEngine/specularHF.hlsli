#ifndef _SPECULAR_HF_
#define _SPECULAR_HF_

#include "toonHF.hlsli"
#include "gammaHF.hlsli"

inline void applySpecular(inout float4 color, in float4 lightColor, in float3 normal, in float3 eyevector, in float3 lightDir
						  , in float fresnelValue, in int specular_power, in float intensity, in bool toonshaded)
{
	if(specular_power){
		//float3 reflectionVector = reflect(lightDir.xyz, normal.xyz);
		//float spec = dot((reflectionVector), eyevector);
		//spec = pow(saturate(-spec), specular_power)*intensity;
		//color.rgb += (lightColor.rgb * spec);
		//color.rgb*=1+spec;
		////color.a=spec;


		float3 reflectionVector = normalize(lightDir.xyz + eyevector.xyz);
		float spec = saturate( dot(reflectionVector, normal) );
		spec = pow(spec, specular_power)*intensity;

		if(toonshaded) toon(spec);

		color.rgb += (lightColor.rgb * spec);
		color.rgb*=pow(abs(1+spec),GAMMA);


		//float3 h = normalize(lightDir.xyz + eyevector.xyz); // Half-vector.
		//float specBase = saturate(dot(normal.xyz, h)); // Regular spec.
		//float fresnel = 1.0 - dot(eyevector, h); // Caculate fresnel.
		//fresnel = pow(fresnel, 5.0);
		//fresnel += fresnelValue * (1.0 - fresnel);
		//float finalSpec = pow(specBase, specular_power) * intensity * fresnel;
		//color.rgb += lightColor.rgb * finalSpec;
		//color.rgb *= 1+finalSpec;
	}
}

#endif // _SPECULAR_HF_
