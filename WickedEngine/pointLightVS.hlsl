#include "lightHF.hlsli"
#include "icosphere.hlsli"

cbuffer staticBuffer:register(b0){
	float4x4 xViewProjection;
	float4x4 xRefViewProjection;
	float4	 xCamPos;
	float4   xMotionBlur;
	float4	 xClipPlane;
}
cbuffer lightBuffer:register(b1){
	float3 lightPos; float pad;
	float4 lightColor;
	float4 lightEnerdis;
};

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(ICOSPHERE[vid],1);
	pos.xyz *= lightEnerdis.y+1;
	pos.xyz += lightPos;
	Out.pos = Out.pos2D = mul(pos,xViewProjection);
	Out.cam = xCamPos.xyz;
	return Out;
}