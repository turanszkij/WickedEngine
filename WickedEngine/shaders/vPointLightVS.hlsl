#include "volumeLightHF.hlsli"
#include "circle.hlsli"
#include "lightingHF.hlsli"

VertexToPixel main(uint vID : SV_VertexID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = CIRCLE[vID];
	Out.col = float4(xLightColor.rgb, 1) * saturate(1 - dot(pos.xyz, pos.xyz));

	pos = mul(xLightWorld, pos);
	Out.pos = mul(GetCamera().view_projection, pos);

	return Out;
}
