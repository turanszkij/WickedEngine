#include "lightHF.hlsli"
#include "quad.hlsli"

cbuffer staticBuffer:register(b0){
	float4x4 xViewProjection;
	float4x4 xRefViewProjection;
	float4x4 xPrevViewProjection;
	float4	 xCamPos;
	float4   xMotionBlur;
	float4	 xClipPlane;
}

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	Out.pos = Out.pos2D = float4(QUAD[vid],1);
	Out.cam = xCamPos.xyz;
	return Out;
}