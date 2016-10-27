#include "deferredLightHF.hlsli"
#include "icosphere.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(ICOSPHERE[vid],1);
	Out.pos = Out.pos2D = mul(mul(pos,g_xTransform),g_xCamera_VP);
	Out.lightIndex = (int)g_xColor.x;
	return Out;
}