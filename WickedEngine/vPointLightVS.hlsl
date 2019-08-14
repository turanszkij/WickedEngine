#include "volumeLightHF.hlsli"
#include "circle.hlsli"

VertexToPixel main(uint vID : SV_VertexID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = CIRCLE[vID];
	pos = mul(lightWorld, pos);
	Out.pos = mul(g_xCamera_VP, pos);
	Out.col = lerp(
		float4(lightColor.rgb, 1), float4(0, 0, 0, 0),
		distance(pos.xyz, float3(lightWorld._14, lightWorld._24, lightWorld._34)) / (lightEnerdis.w)
	);

	return Out;
}