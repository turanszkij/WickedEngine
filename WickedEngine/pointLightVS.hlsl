#include "lightHF.hlsli"
#include "icosphere.hlsli"
#include "pointLightHF.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(ICOSPHERE[vid],1);
	pos.xyz *= lightEnerdis.y+1;
	pos.xyz += lightPos;
	Out.pos = Out.pos2D = mul(pos,g_xCamera_VP);
	return Out;
}