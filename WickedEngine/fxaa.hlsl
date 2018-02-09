#include "imageHF.hlsli"

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

float4 main(VertexToPixelPostProcess PSIn): SV_Target
{
    float2 fxaaFrame;
    xTexture.GetDimensions(fxaaFrame.x, fxaaFrame.y);

    FxaaTex tex = { Sampler, xTexture };

    return FxaaPixelShader(PSIn.tex, 0, tex, tex, tex, 1 / fxaaFrame, 0, 0, 0, fxaaSubpix, fxaaEdgeThreshold, fxaaEdgeThresholdMin, 0, 0, 0, 0);
}
