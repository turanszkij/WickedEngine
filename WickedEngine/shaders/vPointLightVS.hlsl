#include "volumeLightHF.hlsli"
#include "circle.hlsli"
#include "lightingHF.hlsli"

VertexToPixel main(float4 pos : POSITION)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	Out.col = float4(xLightColor.rgb, 1);
	Out.pos = mul(GetCamera().view_projection, pos);

	return Out;
}
