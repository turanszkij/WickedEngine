#ifndef _SPECULAR_HF_
#define _SPECULAR_HF_


inline void applySpecular(inout float4 color, in float4 lightColor, in float3 N, in float3 V, in float3 lightDir
						  , in float fresnelValue, in int specular_power, in float intensity)
{
	if(specular_power){
		float3 reflectionVector = normalize(lightDir.xyz - V.xyz);
		float spec = saturate( dot(reflectionVector, N) );
		spec = pow(spec, specular_power)*intensity;

		color.rgb += (lightColor.rgb * spec);
	}
}

#endif // _SPECULAR_HF_
