#include "volumeLightHF.hlsli"
#include "depthConvertHF.hlsli"

float4 main(VertexToPixel PSIn) : SV_TARGET
{
	return PSIn.col;
}