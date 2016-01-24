#include "effectHF_PS.hlsli"
#include "effectHF_PSVS.hlsli"
#include "globalsHF.hlsli"
#include "ditherHF.hlsli"

PixelOutputType main(PixelInputType PSIn)
{
	clip(dither(PSIn.pos.xy) - PSIn.dither);

	PixelOutputType Out = (PixelOutputType)0;

	float3 normal = normalize(PSIn.nor);
	float4 baseColor = g_xMat_diffuseColor;
	float4 spec= g_xMat_specular;

	float depth = PSIn.pos.z/PSIn.pos.w;
	float3 eyevector = normalize(g_xCamera_CamPos - PSIn.pos3D);

	float2 ScreenCoord, ScreenCoordPrev;
	ScreenCoord.x = PSIn.pos2D.x / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoord.y = -PSIn.pos2D.y / PSIn.pos2D.w / 2.0f + 0.5f;
	ScreenCoordPrev.x = PSIn.pos2DPrev.x / PSIn.pos2DPrev.w / 2.0f + 0.5f;
	ScreenCoordPrev.y = -PSIn.pos2DPrev.y / PSIn.pos2DPrev.w / 2.0f + 0.5f;

	PSIn.tex *= g_xMat_texMulAdd.xy;
	PSIn.tex += g_xMat_texMulAdd.zw;
	[branch]if(g_xMat_hasTex) {
		baseColor *= xTextureTex.Sample(texSampler,PSIn.tex);
	}
	baseColor.rgb *= PSIn.instanceColor;
	
	clip( baseColor.a - 0.1f );
		
	if(depth){

	

	//NORMALMAP
	float3 bumpColor=0;
	if(g_xMat_hasNor){
		float4 nortex = xTextureNor.Sample(texSampler,PSIn.tex);
		if(nortex.a>0){
			float3x3 tangentFrame = compute_tangent_frame(normal, eyevector, -PSIn.tex.xy);
			bumpColor = 2.0f * nortex.rgb - 1.0f;
			//bumpColor.g*=-1;
			normal = normalize(mul(bumpColor, tangentFrame));
		}
	}

	
	//SPECULAR
	//if(hasRefNorTexSpe.w){
		spec = lerp(spec,xTextureSpe.Sample(texSampler, PSIn.tex).r, g_xMat_hasSpe);
	//}
	

	//ENVIROMENT MAP
	float4 envCol=0;
	{
		uint mip=0;
		float2 size;
		float mipLevels;
		enviroTex.GetDimensions(mip,size.x,size.y,mipLevels);

		float3 ref = normalize(reflect(-eyevector, normal));
		envCol = enviroTex.SampleLevel(texSampler,ref,(1-smoothstep(0,128, g_xMat_specular_power))*mipLevels);
		baseColor = lerp(baseColor,envCol, g_xMat_metallic*spec);
	}


	[branch]if(g_xMat_hasRef){
		//REFLECTION
		float2 RefTex;
			RefTex.x = PSIn.ReflectionMapSamplingPos.x/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
			RefTex.y = -PSIn.ReflectionMapSamplingPos.y/PSIn.ReflectionMapSamplingPos.w/2.0f + 0.5f;
		float colorMat = 0;
		colorMat = xTextureMat.SampleLevel(texSampler,PSIn.tex,0);
		normal = normalize( lerp(normal,PSIn.nor,pow(abs(colorMat.x),0.02f)) );
		float4 colorReflection = xTextureRef.SampleLevel(mapSampler,RefTex+normal.xz,0);
		baseColor.rgb=lerp(baseColor.rgb,colorReflection.rgb,colorMat);
	}
		



		

	}

	bool unshaded = g_xMat_shadeless;
	float properties = unshaded ? RT_UNSHADED : 0.0f;

	float2 vel = ScreenCoord - ScreenCoordPrev;

			
	Out.col = float4(baseColor.rgb*(1+ g_xMat_emissive)*PSIn.ao,1);
	Out.nor = float4(normal.xyz,properties);
	Out.vel = float4(vel, g_xMat_specular_power,spec.a);

	
	return Out;
}

