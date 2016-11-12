#include "volumeLightHF.hlsli"
#include "circle.hlsli"

VertexToPixel main(uint vID : SV_VertexID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = CIRCLE[vID];
	pos = mul( pos,lightWorld );
	Out.pos = mul(pos,g_xCamera_VP);
	Out.col=lerp(
		float4(lightColor.rgb,1),float4(0,0,0,0),
		distance(pos.xyz,float3( lightWorld._41,lightWorld._42,lightWorld._43 ))/(lightEnerdis.w)
		);

	return Out;
}