#include "pointLightHF.hlsli"

Texture2D<float4> xTextureTex:register(t0);

struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float3 pos3D			: POSITION3D;
	float2 tex				: TEXCOORD0;
    uint RTIndex			: SV_RenderTargetArrayIndex;
};

float main(VertextoPixel PSIn) : SV_DEPTH
{
	[branch]if(g_xMat_hasTex)
		clip( xTextureTex.Sample(sampler_linear_wrap,PSIn.tex).a<0.1?-1:1 );
	return distance(PSIn.pos3D.xyz,lightPos.xyz)/lightEnerdis.y;
}