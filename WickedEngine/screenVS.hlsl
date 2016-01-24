#include "imageHF.hlsli"

VertexToPixelPostProcess main(uint vI : SV_VERTEXID)
{
	//float2 inTex = float2(vI%2,vI%4/2);
	float2 inTex = float2(vI&1,vI>>1);
	VertexToPixelPostProcess Out = (VertexToPixelPostProcess)0;
	Out.pos=float4((inTex.x-0.5f)*2,-(inTex.y-0.5f)*2,0,1);
	Out.tex=inTex;
	return Out;
}