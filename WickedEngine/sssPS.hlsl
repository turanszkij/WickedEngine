#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"


// Gaussian weights for the six samples around the current pixel:
//   -3 -2 -1 +1 +2 +3
//static const float w[6] = { 0.006,   0.061,   0.242,  0.242,  0.061, 0.006 };
//static const float o[6] = {  -1.0, -0.6667, -0.3333, 0.3333, 0.6667,   1.0 };


#define SSSS_N_SAMPLES 11

#ifdef SSS_PROFILE_SNOW
// snow: 
static const float4 kernel[] = {
    float4(0.784728,       0.669086,      0.560479,     0),
    float4(5.07566e-005,   0.000184771,   0.00471691,   -2),
    float4(0.00084214,     0.00282018,    0.0192831,    -1.28),
    float4(0.00643685,     0.0130999,     0.03639,      -0.72),
    float4(0.0209261,      0.0358608,     0.0821904,    -0.32),
    float4(0.0793803,      0.113491,      0.0771802,    -0.08),
    float4(0.0793803,      0.113491,      0.0771802,    0.08),
    float4(0.0209261,      0.0358608,     0.0821904,    0.32),
    float4(0.00643685,     0.0130999,     0.03639,      0.72),
    float4(0.00084214,     0.00282018,    0.0192831,    1.28),
    float4(5.07565e-005,   0.000184771,   0.00471691,   2),
};
#else 
// skin:
static const float4 kernel[] = {
    float4(0.560479,    0.669086,       0.784728,       0),
    float4(0.00471691,  0.000184771,    5.07566e-005,   -2),
    float4(0.0192831,   0.00282018,     0.00084214,     -1.28),
    float4(0.03639,     0.0130999,      0.00643685,     -0.72),
    float4(0.0821904,   0.0358608,      0.0209261,      -0.32),
    float4(0.0771802,   0.113491,       0.0793803,      -0.08),
    float4(0.0771802,   0.113491,       0.0793803,      0.08),
    float4(0.0821904,   0.0358608,      0.0209261,      0.32),
    float4(0.03639,     0.0130999,      0.00643685,     0.72),
    float4(0.0192831,   0.00282018,     0.00084214,     1.28),
    float4(0.00471691,  0.000184771,    5.07565e-005,   2),
};
#endif // SSS_PROFILE_SNOW

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
    const float4 color_ref = texture_0[pos.xy];
	const float depth_ref = texture_lineardepth[pos.xy] * g_xCamera_ZFarP;

    // Accumulate center sample, multiplying it with its gaussian weight:
    float4 colorBlurred = color_ref;
    colorBlurred.rgb *= kernel[0].rgb;

    // Calculate the step that we will use to fetch the surrounding pixels,
    // where "step" is:
    //     step = sssStrength * gaussianWidth * pixelSize * dir
    // The closer the pixel, the stronger the effect needs to be, hence
    // the factor 1.0 / depthM.
    float2 finalStep = color_ref.a * sss_step.xy / depth_ref;

    // Accumulate the other samples:
    for (int i = 1; i < SSSS_N_SAMPLES; i++) 
	{
        // Fetch color and depth for current sample:
        float2 offset = uv + kernel[i].a * finalStep;
        float3 color = texture_0.SampleLevel(sampler_linear_clamp, offset, 0).rgb;
		float depth = texture_lineardepth.SampleLevel(sampler_point_clamp, offset, 0) * g_xCamera_ZFarP;

        // If the difference in depth is huge, we lerp color back to "colorM":
        const float edge_fallback = saturate(abs(depth_ref - depth));
        color = lerp(color, color_ref.rgb, edge_fallback);

        // Accumulate:
        colorBlurred.rgb += kernel[i].rgb * color.rgb;
    }

    return colorBlurred;
}