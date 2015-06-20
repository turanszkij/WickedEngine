#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "gammaHF.hlsli"
#include "specularHF.hlsli"

Texture2D<float> xDepthMap:register(t7);
Texture2D<float4> xRippleMap:register(t8);

float4 main( PixelInputType PSIn) : SV_TARGET
{
	PixelOutputType Out = (PixelOutputType)0;

	float3 normal = normalize(PSIn.nor);
	float4 spec = specular;
	float4 baseColor = float4(0,0,0,1);
	
		float3 eyevector = normalize(PSIn.cam - PSIn.pos3D);
		float2 screenPos;
			screenPos.x = PSIn.pos2D.x/PSIn.pos2D.w/2.0f + 0.5f;
			screenPos.y = -PSIn.pos2D.y/PSIn.pos2D.w/2.0f + 0.5f;
		
		float depth = PSIn.pos2D.z;
		float refDepth = ( xDepthMap.Sample(mapSampler,screenPos) );


		//NORMALMAP
		float2 bumpColor0=0;
		float2 bumpColor1=0;
		float2 bumpColor2=0;
		float3 bumpColor=0;
		if(hasNor){
			float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex);
			bumpColor0 = 2.0f * xTextureNor.Sample(texSampler,PSIn.tex-movingTex.yy).rg - 1.0f;
			bumpColor1 = 2.0f * xTextureNor.Sample(texSampler,PSIn.tex+movingTex).rg - 1.0f;
			bumpColor2 = xRippleMap.Sample(texSampler,screenPos).rg;
			bumpColor= float3( bumpColor0+bumpColor1+bumpColor2,1 )  * refractionIndex;
			normal = normalize(mul(normalize(bumpColor), tangentFrame));
			//normal = (bumpColor.x * PSIn.tan) + (bumpColor.y * PSIn.bin) + (bumpColor.z * PSIn.nor);
			//normal = normalize(normal);
		}
		

		//ENVIROMENT MAP
		//float3 ref = (reflect(-eyevector, normal));
		//float3 reflectiveColor = enviroTex.Sample(texSampler,ref);

		//REFLECTION
		float2 RefTex;
			RefTex.x = PSIn.ReflectionMapSamplingPos.x/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
			RefTex.y = -PSIn.ReflectionMapSamplingPos.y/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
		float3 reflectiveColor = xTextureRef.Sample(mapSampler,RefTex+bumpColor).rgb;
		
	
		//REFRACTION 
		float2 perturbatedRefrTexCoords = screenPos + bumpColor;   
		float3 refractiveColor = xTextureRefrac.Sample(mapSampler, perturbatedRefrTexCoords);
		float mod = saturate(0.05*(refDepth-depth));
		refractiveColor = lerp(refractiveColor,float4(0,0,0.23,1),mod).rgb;
		//baseColor.rgb=lerp(baseColor,refractiveColor,diffuseColor.a);

		
		//FRESNEL TERM
		float fresnelTerm = dot(normal, eyevector);
		float3 combinedColor = lerp(reflectiveColor, refractiveColor, fresnelTerm);


		//DULL COLOR
		float3 dullColor = diffuseColor.rgb;
		baseColor.rgb = lerp(combinedColor, dullColor, 0.16);

		baseColor.rgb=pow(baseColor.rgb,GAMMA);
		
		
		//SOFT EDGE
		float fade = saturate(0.3* (refDepth-depth));
		baseColor.a*=fade;
		
		baseColor.rgb*= clamp( saturate( abs(dot(xSun,PSIn.nor)) *xSunColor.rgb ),xAmbient,1 );

		//SPECULAR
		if(hasSpe && xSun.w){
			spec = xTextureSpe.Sample(texSampler, PSIn.tex);
		}
		/*float3 reflectionVector = reflect(xSun, normal);
		float specR = dot(normalize(reflectionVector), eyevector);
		specR = pow(abs(specR), specular_power)*spec.w;
		baseColor.rgb += saturate(xSunColor.rgb) * specR;*/
		applySpecular(baseColor, xSunColor, normal, eyevector, xSun, 1, specular_power, spec.w, toonshaded);
		

		baseColor.rgb=pow(baseColor.rgb,INV_GAMMA)+emit;

		baseColor.rgb = applyFog(baseColor.rgb,xHorizon,getFog(getLinearDepth(depth/PSIn.pos2D.w),PSIn.pos3D,xFogSEH));
		
		//Out.col = saturate(baseColor);
		//Out.nor = float4((normal),1);
		//Out.spe = saturate(spec);
		//Out.vel = float4(PSIn.vel*float3(-1,1,1),1);

	return baseColor;
}