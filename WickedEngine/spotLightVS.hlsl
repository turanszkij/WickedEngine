#include "lightHF.hlsli"
#include "cone.hlsli"
#include "spotLightHF.hlsli"


VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CONE[vid],1);
	pos = mul( pos,xLightWorld );
	Out.pos = Out.pos2D = mul(pos,g_xCamera_VP);
	return Out;
}