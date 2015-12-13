#include "volumeLightHF.hlsli"
#include "cone.hlsli"

cbuffer staticBuffer:register(b0){
	float4x4 xViewProjection;
	float4x4 xRefViewProjection;
	float4	 xCamPos;
	float4   xMotionBlur;
	float4	 xClipPlane;
}
cbuffer lightBuffer:register(b1){
	float4x4 lightWorld;
	float4 lightColor;
	float4 lightEnerdis;
};

VertexToPixel main(uint vID : SV_VertexID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CONE[vID],1);
	pos = mul( pos,lightWorld );
	Out.pos = mul(pos,xViewProjection);
	Out.col=lerp(
		float4(lightColor.rgb,1),float4(0,0,0,0),
		distance(pos.xyz,float3( lightWorld._41,lightWorld._42,lightWorld._43 ))/(lightEnerdis.w)
		);

	return Out;
}