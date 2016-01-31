#ifndef _POINTLIGHTHF_
#define _POINTLIGHTHF_
#include "globals.hlsli"

TextureCube xTextureSh:register(t7);

CBUFFER(PointLightCB, CBSLOT_RENDERER_POINTLIGHT)
{
	float3 lightPos; float pad;
	float4 lightColor;
	float4 lightEnerdis;
};

inline float pointLight(in float3 pos3D, in float3 normal, out float3 lightDir, out float attenuation, in bool toonshaded = false)
{
	float  lightdis = distance(pos3D,lightPos);
	float att = ( lightEnerdis.x * (lightEnerdis.y/(lightEnerdis.y+1+lightdis)) );
	attenuation = /*saturate*/( att * (lightEnerdis.y-lightdis) / lightEnerdis.y );
	lightDir = normalize( lightPos - pos3D );
	float  lightint = saturate( dot( lightDir,normal ) );
	//[branch]if(toonshaded) toon(lightint);


	[branch]if(lightEnerdis.w){
		const float3 lv = pos3D.xyz-lightPos.xyz;
		static const float bias = 0.025;
		lightint *= xTextureSh.SampleCmpLevelZero(sampler_cmp_depth,lv,length(lv)/lightEnerdis.y-bias ).r;
	}

	return saturate(attenuation*lightint);
}

#endif // _POINTLIGHTHF_
