#include "objectHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "gammaHF.hlsli"
#include "specularHF.hlsli"
#include "globals.hlsli"

//Texture2D<float> xDepthMap:register(t7);
//Texture2D<float4> xRippleMap:register(t8);

float4 main( PixelInputType PSIn) : SV_TARGET
{
	PixelOutputType Out = (PixelOutputType)0;

	float3 normal = normalize(PSIn.nor);
	float4 spec = g_xMat_specular;
	float4 baseColor = float4(0,0,0,1); 
	
	PSIn.tex *= g_xMat_texMulAdd.xy;
	
		float3 eyevector = normalize(g_xCamera_CamPos - PSIn.pos3D);
		float2 screenPos;
			screenPos.x = PSIn.pos2D.x/PSIn.pos2D.w/2.0f + 0.5f;
			screenPos.y = -PSIn.pos2D.y/PSIn.pos2D.w/2.0f + 0.5f;
		


		//NORMALMAP
		float2 bumpColor0=0;
		float2 bumpColor1=0;
		float2 bumpColor2=0;
		float3 bumpColor=0;
		if(g_xMat_hasNor){
			float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex);
			bumpColor0 = 2.0f * xNormalMap.Sample(sampler_aniso_wrap,PSIn.tex - g_xMat_texMulAdd.ww).rg - 1.0f;
			bumpColor1 = 2.0f * xNormalMap.Sample(sampler_aniso_wrap,PSIn.tex + g_xMat_texMulAdd.zw).rg - 1.0f;
			bumpColor2 = xWaterRipples.Sample(sampler_aniso_wrap,screenPos).rg;
			bumpColor= float3( bumpColor0+bumpColor1+bumpColor2,1 )  * g_xMat_refractionIndex;
			normal = normalize(mul(normalize(bumpColor), tangentFrame));
			//normal = (bumpColor.x * PSIn.tan) + (bumpColor.y * PSIn.bin) + (bumpColor.z * PSIn.nor);
			//normal = normalize(normal);
		}
		

		////ENVIROMENT MAP
		//float3 ref = (reflect(-eyevector, normal));
		//float3 skyColor = enviroTex.Sample(texSampler,ref).rgb;

		//REFLECTION
		float2 RefTex;
			RefTex.x = PSIn.ReflectionMapSamplingPos.x/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
			RefTex.y = -PSIn.ReflectionMapSamplingPos.y/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
		float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror,RefTex+bumpColor.rg,0);
		
		//reflectiveColor.rgb = lerp(skyColor, reflectiveColor.rgb, reflectiveColor.a);
	
		//REFRACTION 
		float2 perturbatedRefrTexCoords = screenPos.xy + bumpColor.rg;
		float depth = PSIn.pos2D.z;
		float refDepth = (texture_lineardepth.Sample(sampler_linear_mirror, screenPos));
		float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
		float eyeDot = dot(normal, eyevector);
		float mod = saturate(0.05*(refDepth-depth));
		float3 dullColor = lerp(refractiveColor, g_xMat_diffuseColor.rgb, saturate(eyeDot));
		refractiveColor = lerp(refractiveColor, dullColor, mod).rgb;
		
		//FRESNEL TERM
		float fresnelTerm = abs(eyeDot);
		baseColor.rgb = lerp(reflectiveColor.rgb, refractiveColor, fresnelTerm);

		//DULL COLOR
		baseColor.rgb = lerp(baseColor.rgb, g_xMat_diffuseColor.rgb, 0.16);

		baseColor.rgb=pow(abs(baseColor.rgb),GAMMA);
		
		
		//SOFT EDGE
		float fade = saturate(0.3* abs(refDepth-depth));
		baseColor.a*=fade;
		
		baseColor.rgb*= clamp( saturate( abs(dot(g_xWorld_SunDir.xyz,PSIn.nor)) *g_xWorld_SunColor.rgb ), g_xWorld_Ambient.rgb,1 );

		//SPECULAR
		if(g_xMat_hasSpe){
			spec = xSpecularMap.Sample(sampler_aniso_wrap, PSIn.tex);
		}
		/*float3 reflectionVector = reflect(xSun, normal);
		float specR = dot(normalize(reflectionVector), eyevector);
		specR = pow(abs(specR), specular_power)*spec.w;
		baseColor.rgb += saturate(xSunColor.rgb) * specR;*/
		applySpecular(baseColor, g_xWorld_SunColor, normal, eyevector, g_xWorld_SunDir.xyz, 1, g_xMat_specular_power, spec.w, 0);
		

		baseColor.rgb = pow(abs(baseColor.rgb*(1 + g_xMat_emissive)), INV_GAMMA);

		baseColor.rgb = applyFog(baseColor.rgb,getFog(getLinearDepth(depth/PSIn.pos2D.w)));
		
		//Out.col = saturate(baseColor);
		//Out.nor = float4((normal),1);
		//Out.spe = saturate(spec);
		//Out.vel = float4(PSIn.vel*float3(-1,1,1),1);

	return baseColor;
}