#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_GREEN_AS_LUMA 1
//#define FXAA_QUALITY__PRESET 12
//#define FXAA_QUALITY__PRESET 25
#define FXAA_QUALITY__PRESET 39
#include "fxaa.hlsli"

static const float fxaaSubpix = 0.75;
static const float fxaaEdgeThreshold = 0.166;
static const float fxaaEdgeThresholdMin = 0.0833;

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

    FxaaTex tex = { sampler_linear_clamp, input };

    output[DTid.xy] = FxaaPixelShader(uv, 0, tex, tex, tex, xPPResolution_rcp, 0, 0, 0, fxaaSubpix, fxaaEdgeThreshold, fxaaEdgeThresholdMin, 0, 0, 0, 0);
}
