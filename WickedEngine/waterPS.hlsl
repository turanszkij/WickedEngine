#include "objectHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "gammaHF.hlsli"
#include "specularHF.hlsli"
#include "globals.hlsli"

float4 main(PixelInputType input) : SV_TARGET
{
	OBJECT_PS_MAKE

	OBJECT_PS_SPECULARMAPPING


	baseColor.a = 1;

	//NORMALMAP
	float2 bumpColor0=0;
	float2 bumpColor1=0;
	float2 bumpColor2=0;
	if(g_xMat_hasNor){
		float3x3 tangentFrame = compute_tangent_frame(N, P, UV);
		bumpColor0 = 2.0f * xNormalMap.Sample(sampler_aniso_wrap,input.tex - g_xMat_texMulAdd.ww).rg - 1.0f;
		bumpColor1 = 2.0f * xNormalMap.Sample(sampler_aniso_wrap,input.tex + g_xMat_texMulAdd.zw).rg - 1.0f;
		bumpColor2 = xWaterRipples.Sample(sampler_aniso_wrap,ScreenCoord).rg;
		bumpColor= float3( bumpColor0+bumpColor1+bumpColor2,1 )  * g_xMat_refractionIndex;
		N = normalize(mul(normalize(bumpColor), tangentFrame));
	}
		

	//REFLECTION
	float2 RefTex = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
	float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror,RefTex+bumpColor.rg,0);
		
	
	//REFRACTION 
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + bumpColor.rg;
	float refDepth = (texture_lineardepth.Sample(sampler_linear_mirror, ScreenCoord));
	float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
	float eyeDot = dot(N, -V);
	float mod = saturate(0.05*(refDepth- lineardepth));
	float3 dullColor = lerp(refractiveColor, g_xMat_diffuseColor.rgb, saturate(eyeDot));
	refractiveColor = lerp(refractiveColor, dullColor, mod).rgb;
		
	//FRESNEL TERM
	float fresnelTerm = abs(eyeDot);
	baseColor.rgb = lerp(reflectiveColor.rgb, refractiveColor, fresnelTerm);

	//DULL COLOR
	baseColor.rgb = lerp(baseColor.rgb, g_xMat_diffuseColor.rgb, 0.16);
		
		
	//SOFT EDGE
	float fade = saturate(0.3* abs(refDepth- lineardepth));
	baseColor.a*=fade;


	OBJECT_PS_DEGAMMA
		
	OBJECT_PS_DIRECTIONALLIGHT
		
	OBJECT_PS_GAMMA

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD
}