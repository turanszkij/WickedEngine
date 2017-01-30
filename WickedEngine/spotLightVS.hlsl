#include "deferredLightHF.hlsli"
#include "cone.hlsli"


VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = CONE[vid];
	Out.pos = Out.pos2D = mul( pos, g_xTransform );
	Out.lightIndex = (int)g_xColor.x;
	return Out;
}