#include "deferredLightHF.hlsli"
#include "icosphere.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;
		
	float4 pos = ICOSPHERE[vid];
	Out.pos = Out.pos2D = mul(pos, g_xTransform);
	return Out;
}