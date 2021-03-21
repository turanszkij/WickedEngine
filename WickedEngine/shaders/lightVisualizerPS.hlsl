#include "volumeLightHF.hlsli"

float4 main(VertexToPixel PSIn) : SV_TARGET
{
	return max(PSIn.col,0);
}