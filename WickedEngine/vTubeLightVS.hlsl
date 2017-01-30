#include "volumeLightHF.hlsli"
#include "cylinder.hlsli"

VertexToPixel main(uint vID : SV_VertexID)
{
	VertexToPixel Out = (VertexToPixel)0;

	float4 pos = CYLINDER[vID];
	Out.pos = mul(pos, lightWorld);
	Out.col = float4(lightColor.rgb * lightEnerdis.x, 1);

	return Out;
}