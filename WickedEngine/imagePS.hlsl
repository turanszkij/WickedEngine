#include "imageHF.hlsli"

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = xColor;

	//return float4(PSIn.tex, 0, 1);
		
	[branch]if(xDistort==1){
		float2 distortionCo;
			distortionCo.x = PSIn.dis.x/PSIn.dis.w/2.0f + 0.5f;
			distortionCo.y = -PSIn.dis.y/PSIn.dis.w/2.0f + 0.5f;
			float2 distort = normalize(float3(texture_2.SampleLevel(Sampler, PSIn.tex.xy, xMipLevel).rg, 1)).rg*0.04f*xOpacity;
		PSIn.tex.xy=distortionCo+distort;
	}

	color *= xTexture.SampleLevel(Sampler, PSIn.tex.xy, xMipLevel);
	[branch]if(xMask==1)
		color *= xMaskTex.SampleLevel(Sampler, PSIn.tex.xy, xMipLevel).a;
	[branch]if(xNormalmapSeparate==2)
		color=2*color-1;
	

	color.rgb *= 1 - xFade;
	color.a*= xOpacity;
	color.a=saturate(color.a);


	return color;
}