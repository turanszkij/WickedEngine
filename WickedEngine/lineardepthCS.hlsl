#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "depthConvertHF.hlsli"

TEXTURE2D(input, float, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	output[DTid.xy] = getLinearDepth(input[DTid.xy]) * g_xFrame_MainCamera_ZFarP_Recip; // store in range 0-1 for reduced precision
}
