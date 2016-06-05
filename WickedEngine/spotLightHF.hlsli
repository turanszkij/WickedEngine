#ifndef _SPOTLIGHT_HF_
#define _SPOTLIGHT_HF_

//Texture2D<float> xTextureSh:register(t4);


CBUFFER(SpotLightCB, CBSLOT_RENDERER_SPOTLIGHT)
{
	float4x4 xLightWorld;
	float4 xLightDir;
	float4 xLightColor;
	float4 xLightEnerDisCone;
	float4 xBiasResSoftshadow;
	float4x4 xShMat;
};

inline float offset_lookup(Texture2D<float> intex, SamplerComparisonState map,
                     float2 loc,
                     float2 offset,
					 float scale,
					 float realDistance)
{
	float BiasedDistance = realDistance - xBiasResSoftshadow.x;

	return intex.SampleCmpLevelZero( map, loc + offset / scale, BiasedDistance).r;
}
inline float shadowCascade(float4 shadowPos, float2 ShTex, Texture2D<float> shadowTexture){
	float realDistance = shadowPos.z/shadowPos.w;
	float sum = 0;
	float scale = xBiasResSoftshadow.y;
	float retVal = 1;
	retVal *= offset_lookup(shadowTexture, sampler_cmp_depth, ShTex, float2(0, 0), scale, realDistance);

	return retVal;
}

inline void spotLight(in float3 P, in float3 N, in float3 V, in float roughness, in float3 f0,
	out float3 diffuse, out float3 specular)
{
	float3 lightPos = float3( xLightWorld._41, xLightWorld._42, xLightWorld._43 );
	
	float3 L = normalize(lightPos - P);
	BRDF_MAKE(N, L, V);
	specular = xLightColor.rgb * BRDF_SPECULAR(roughness, f0);
	diffuse = xLightColor.rgb * BRDF_DIFFUSE(roughness);
	diffuse *= xLightEnerDisCone.x;
	specular *= xLightEnerDisCone.x;

	float SpotFactor = dot(L, xLightDir.xyz);

	float spotCutOff = xLightEnerDisCone.z;

	[branch]if (SpotFactor > spotCutOff){

		//color.rgb = max(dot(normalize(xLightDir.xyz), normalize(N)), 0)*xLightEnerDisCone.x;

		float sh = 1;
		float4 ShPos = mul(float4(P,1),xShMat);
		float2 ShTex = ShPos.xy / ShPos.w * float2(0.5f,-0.5f) + float2(0.5f,0.5f);
		[branch]if((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
		{
			//light.r+=1.0f;
			sh *= shadowCascade(ShPos,ShTex,texture_shadow0);
		}
		diffuse *= sh;
		specular *= sh;

		
		float attenuation=saturate( (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - spotCutOff)) );
		diffuse *= attenuation;
		specular *= attenuation;

	}

	diffuse = max(diffuse, 0);
	specular = max(specular, 0);

}

// MACROS

#define DEFERRED_SPOTLIGHT_MAIN	\
	spotLight(P, N, V, roughness, f0, diffuse, specular);

#endif // _SPOTLIGHT_HF_
