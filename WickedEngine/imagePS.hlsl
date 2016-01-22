#include "imageHF.hlsli"


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);

		
	//[branch]if(xMaskFadOpaDis.w==1){
	//	float2 distortionCo;
	//		distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
	//		distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
	//		float2 distort = normalize(float3(xNormalMap.SampleLevel(Sampler, PSIn.tex.xy, PSIn.mip).rg, 1)).rg*0.04f*xMaskFadOpaDis.z;
	//	PSIn.tex.xy=distortionCo+distort;
	//}

	color += xTexture.SampleLevel(Sampler, PSIn.tex.xy + PSIn.tex.zw, PSIn.mip);
	[branch]if(xMaskDistort.x==1)
		color *= xMaskTex.SampleLevel(Sampler, PSIn.tex.xy, PSIn.mip).a;
	[branch]if(xMaskDistort.w==2)
		color=2*color-1;
	

	color.rgb*=1- xOffsetMirFade.w;
	color.a*= xBlurOpaPiv.z;
	color.a=saturate(color.a);


	return color;
}