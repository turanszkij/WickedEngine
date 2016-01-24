#include "postProcessHF.hlsli"
//#include "ViewProp.h"


float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,0);
	//float numSampling = 0.0f;

	color += xTexture.SampleLevel(Sampler,PSIn.tex,0);
	//numSampling++;

	float targetDepth = xPPParams0[2];

	//if(targetDepth){
		float fragmentDepth = ( xSceneDepthMap.SampleLevel(Sampler,PSIn.tex,0).r );
		float difference = abs(targetDepth-fragmentDepth);
		//[branch]if(difference>0.0001){
			/*static const float loop = 1.5f;
			[unroll]
			for(float x = -loop; x <= loop; x += 1.0f)
				for(float y = -loop; y <= loop; y += 1.0f){
					float2 offset = clamp(float2( x,y ) * difference * 0.1,-0.0012,0.0012);
					color += xTexture.SampleLevel(Sampler, PSIn.tex + offset,0);
					numSampling++;
				}*/
			color=lerp(color,xMaskTex.SampleLevel(Sampler,PSIn.tex,0),abs(clamp(difference*0.008f,-1,1)));
		//}
	//}

	return color/*/numSampling*/;
}