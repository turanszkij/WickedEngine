Texture2D<float> xTextureSh:register(t4);


CBUFFER(SpotLightCB, CBSLOT_RENDERER_SPOTLIGHT)
{
	float4x4 lightWorld;
	float4 lightDir;
	float4 lightColor;
	float4 lightEnerDisCone;
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

inline float spotLight(in float3 pos3D, in float3 normal, out float attenuation, out float3 lightToPixel, in bool toonshaded = false)
{
	float3 lightPos = float3( lightWorld._41,lightWorld._42,lightWorld._43 );
	
	lightToPixel = normalize(lightPos-pos3D);
	float SpotFactor = dot(lightToPixel, lightDir.xyz);

	float spotCutOff = lightEnerDisCone.z;

	float l=0;
	attenuation=0;
	[branch]if (SpotFactor > spotCutOff){

		l=saturate(dot(normalize(lightDir.xyz),normalize(normal)))*lightEnerDisCone.x;
		[branch]if(toonshaded) toon(l);

		float4 ShPos = mul(float4(pos3D,1),xShMat);
		float2 ShTex = ShPos.xy / ShPos.w * float2(0.5f,-0.5f) + float2(0.5f,0.5f);
		[branch]if((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
		{
			//light.r+=1.0f;
			l *= shadowCascade(ShPos,ShTex,xTextureSh);
		}

		
		attenuation=saturate( (1.0 - (1.0 - SpotFactor) * 1.0/(1.0 - spotCutOff)) );

	}
		

	return l;
}