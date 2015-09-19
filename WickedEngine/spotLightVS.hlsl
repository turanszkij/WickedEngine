#include "lightHF.hlsli"
#include "cone.hlsli"

cbuffer staticBuffer:register(b0){
	float4x4 xViewProjection;
	float4x4 xRefViewProjection;
	float4x4 xPrevViewProjection;
	float4	 xCamPos;
	float4   xMotionBlur;
	float4	 xClipPlane;
}
cbuffer lightBuffer:register(b1){
	float4x4 lightWorld;
	float4 lightDir;
	float4 lightColor;
	float4 lightEnerdis;
	float4 xBiasResSoftshadow;
	float4x4 xShMat;
};


VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out = (VertexToPixel)0;
		
	float4 pos = float4(CONE[vid],1);
	pos = mul( pos,lightWorld );
	Out.pos = Out.pos2D = mul(pos,xViewProjection);
	Out.cam = xCamPos.xyz;
	return Out;
}