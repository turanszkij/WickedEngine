#include "deferredLightHF.hlsli"
#include "cone.hlsli"


VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CONE[vid],1);
	pos = mul( pos, g_xTransform );
	Out.pos = Out.pos2D = mul(pos,g_xCamera_VP);
	Out.lightIndex = g_xMisc_int4[0];
	return Out;
}