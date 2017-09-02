#include "depthConvertHF.hlsli"


struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
	nointerpolation float4 opaAddDarkSiz		: TEXCOORD1;
	//float3 col				: TEXCOORD2;
	float4 pp				: TEXCOORD2;
};


float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float2 pTex = float2(1,-1) * PSIn.pp.xy / PSIn.pp.w / 2.0f + 0.5f;
	float4 depthScene=(texture_lineardepth.GatherRed(sampler_linear_clamp,pTex));
	float depthFragment=PSIn.pp.w;
	float fade = saturate(1.0/PSIn.opaAddDarkSiz.w*(max(max(depthScene.x,depthScene.y),max(depthScene.z,depthScene.w))-depthFragment));
	//fade = depthScene<depthFragment?0:1;

	float4 color = float4(0,0,0,0);
	color=texture_0.Sample(sampler_linear_clamp,PSIn.tex);

	[branch]
	if(PSIn.opaAddDarkSiz.z)
	{
		color.rgb=float3(0,0,0);
	}
	else{
		color.a-=PSIn.opaAddDarkSiz.x;
		color.a*=fade;
	}

	//color.rgb*=PSIn.col.rgb;

	//return clamp( color, 0, inf );
	return max(color, 0);
}
