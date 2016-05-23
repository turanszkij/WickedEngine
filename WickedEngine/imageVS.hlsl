#include "globals.hlsli"
#include "imageHF.hlsli"

CBUFFER(ImageCB_VS, CBSLOT_IMAGE_IMAGE)
{
	float4x4	xTransform;
	float4		xTexMulAdd;
	float2		xPivot;
	uint		xMirror;
	float		xPadding_ImageCB_VS[1];
};

VertextoPixel main(uint vI : SV_VERTEXID)
{
	VertextoPixel Out = (VertextoPixel)0;

	// This vertex shader generates a trianglestrip like this:
	//	1--2
	//	  /
	//	 /
	//	3--4
	float2 inTex = float2(vI % 2, vI % 4 / 2);

	float4 inPos = float4(inTex - xPivot, 0, 1);
	
	Out.pos = mul(inPos, xTransform);

	Out.tex.xy = inTex * xTexMulAdd.xy + xTexMulAdd.zw;

	Out.dis = Out.pos;

	return Out;
}

