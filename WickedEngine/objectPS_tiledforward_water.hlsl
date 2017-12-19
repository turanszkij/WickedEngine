#define DISABLE_DECALS
#define DIRECTIONALLIGHT_SOFT
#define DISABLE_ALPHATEST
#include "objectHF.hlsli"

[earlydepthstencil]
float4 main(PixelInputType input) : SV_TARGET
{
	OBJECT_PS_MAKE

	OBJECT_PS_COMPUTETANGENTSPACE

	OBJECT_PS_SAMPLETEXTURES

	OBJECT_PS_DEGAMMA

	OBJECT_PS_LIGHT_BEGIN

	color.a = 1;

	//NORMALMAP
	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	bumpColor0 = 2.0f * xNormalMap.Sample(sampler_objectshader,UV - g_xMat_texMulAdd.ww).rg - 1.0f;
	bumpColor1 = 2.0f * xNormalMap.Sample(sampler_objectshader,UV + g_xMat_texMulAdd.zw).rg - 1.0f;
	bumpColor2 = xWaterRipples.Sample(sampler_objectshader,ScreenCoord).rg;
	bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2,1)  * g_xMat_refractionIndex;
	N = normalize(lerp(N, mul(normalize(bumpColor), TBN), g_xMat_normalMapStrength));
	bumpColor *= g_xMat_normalMapStrength;

	//REFLECTION
	float2 RefTex = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
	float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror,RefTex + bumpColor.rg,0);


	//REFRACTION 
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + bumpColor.rg;
	float refDepth = (texture_lineardepth.Sample(sampler_linear_mirror, ScreenCoord));
	float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
	float mod = saturate(0.05*(refDepth - lineardepth));
	refractiveColor = lerp(refractiveColor, baseColor.rgb, mod).rgb;

	//FRESNEL TERM
	float NdotV = abs(dot(N, V)) + 1e-5f;
	float3 fresnelTerm = F_Fresnel(f0, NdotV);
	albedo.rgb = lerp(refractiveColor, reflectiveColor.rgb, fresnelTerm);

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_LIGHT_END

	OBJECT_PS_EMISSIVE

	//SOFT EDGE
	float fade = saturate(0.3 * abs(refDepth - lineardepth));
	color.a *= fade;

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD_SIMPLE
}
