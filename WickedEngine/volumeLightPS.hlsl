#include "volumeLightHF.hlsli"
//#include "depthConvertHF.hlsli"
//#include "effectHF_PS.hlsli"
//#include "fogHF.hlsli"

float4 main(VertexToPixel PSIn) : SV_TARGET
{
	//float4 color = PSIn.col;
	//color.rgb = applyFog(color.rgb,xHorizon,getFog(getLinearDepth(PSIn.pos2D.z/PSIn.pos2D.w),xFogSEH));
	return PSIn.col;
}