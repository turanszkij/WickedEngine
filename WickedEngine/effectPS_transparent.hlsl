#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "fogHF.hlsli"
#include "depthConvertHF.hlsli"
#include "gammaHF.hlsli"
#include "specularHF.hlsli"

Texture2D<float> xDepthMap:register(t7);

float4 main( PixelInputType PSIn) : SV_TARGET
{
	float3 normal = normalize(PSIn.nor);
	//uint mat = PSIn.mat;
	float4 spec = specular;
	float4 baseColor = diffuseColor;
	
		[branch]if(hasTex){
			baseColor = xTextureTex.Sample(texSampler, PSIn.tex+movingTex);
			//baseColor*=baseColor.a;
		}
		baseColor.rgb *= PSIn.instanceColor;
		//baseColor.a=1;
		clip( baseColor.a < 0.1f ? -1:1 );

		float3 eyevector = normalize(PSIn.cam - PSIn.pos3D);
		float2 screenPos;
			screenPos.x = PSIn.pos2D.x/PSIn.pos2D.w/2.0f + 0.5f;
			screenPos.y = -PSIn.pos2D.y/PSIn.pos2D.w/2.0f + 0.5f;
		


		//NORMALMAP
		float3 bumpColor=0;
		if(hasNor){
			float4 nortex = xTextureNor.Sample(texSampler,PSIn.tex+movingTex);
			if(nortex.a>0){
				float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex.xy);
				bumpColor = 2.0f * nortex.rgb - 1.0f;
				//bumpColor.g*=-1;
				normal = normalize(mul(bumpColor, tangentFrame));
			}
		}

		

		//ENVIROMENT MAP
		float4 envCol=0;
		{
			uint mip=0;
			float2 size;
			float mipLevels;
			enviroTex.GetDimensions(mip,size.x,size.y,mipLevels);

			float3 ref = normalize(reflect(-eyevector, normal));
			envCol = enviroTex.SampleLevel(texSampler,ref,(1-smoothstep(0,128,specular_power))*mipLevels);
			baseColor = lerp(baseColor,envCol,metallic*spec);
		}
		
	
		//REFRACTION 
		float2 perturbatedRefrTexCoords = screenPos.xy + bumpColor.rg * refractionIndex;   
		float4 refractiveColor = (xTextureRefrac.SampleLevel(mapSampler, perturbatedRefrTexCoords, 0));
		baseColor.rgb=lerp(refractiveColor.rgb,baseColor.rgb,baseColor.a);
		
		baseColor.rgb=pow(abs(baseColor.rgb),GAMMA);
		baseColor.a=1;
		
		float depth = PSIn.pos2D.z;
		

		//SPECULAR
		if(hasSpe && !shadeless){
			spec = xTextureSpe.Sample(texSampler, PSIn.tex);
		}
		/*float3 reflectionVector = reflect(xSun, normal);
		float specR = dot(normalize(reflectionVector), eyevector);
		specR = pow(saturate(-specR), specular_power)*spec.w;
		baseColor.rgb += saturate(xSunColor.rgb * specR);*/
		[branch]if(!shadeless.x){
			baseColor.rgb*=clamp( saturate( abs(dot(xSun.xyz,PSIn.nor)) * xSunColor.rgb ),xAmbient.rgb,1 );
			applySpecular(baseColor, xSunColor, normal, eyevector, xSun.xyz, 1, specular_power, spec.w, toonshaded);
		}
		
		baseColor.rgb = pow(abs(baseColor.rgb*(1 + emit)), INV_GAMMA);

		baseColor.rgb = applyFog(baseColor.rgb,xHorizon.xyz,getFog(getLinearDepth(depth/PSIn.pos2D.w),xFogSEH.xyz));
		
		//Out.col = saturate(baseColor);
		//Out.nor = float4((normal),1);
		//Out.spe = saturate(spec);
		//Out.vel = float4(PSIn.vel*float3(-1,1,1),1);

	return baseColor;
}