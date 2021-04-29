#include "volumetricLightHF.hlsli"
#include "icosphere.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;

	float4 pos = ICOSPHERE[vid];
	Out.pos = Out.pos2D = mul(g_xTransform, pos);
	return Out;
}