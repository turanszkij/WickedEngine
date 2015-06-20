#include "imageHF.hlsli"

#define FXAA_PC 1
#define FXAA_HLSL_4 1
#define FXAA_GREEN_AS_LUMA 1
#define FXAA_QUALITY__PRESET 12
//#define FXAA_QUALITY__PRESET 25
//#define FXAA_QUALITY__PRESET 39
#include "fxaa.hlsli"

struct PS_IN
{
	float4 pos: SV_POSITION;
    float2 uv: TEXCOORD;
};

float4 main(PS_IN I): SV_Target
{
    static const float fxaaSubpix = 0.75;
    static const float fxaaEdgeThreshold = 0.166;
    static const float fxaaEdgeThresholdMin = 0.0833;

    float2 fxaaFrame;
    xTexture.GetDimensions(fxaaFrame.x, fxaaFrame.y);

    FxaaTex tex = { Sampler, xTexture };

    return FxaaPixelShader(I.uv, 0, tex, tex, tex, 1 / fxaaFrame, 0, 0, 0, fxaaSubpix, fxaaEdgeThreshold, fxaaEdgeThresholdMin, 0, 0, 0, 0);
}
