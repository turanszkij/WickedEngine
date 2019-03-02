#include "postProcessHF.hlsli"


// Gaussian weights for the six samples around the current pixel:
//   -3 -2 -1 +1 +2 +3
//static const float w[6] = { 0.006,   0.061,   0.242,  0.242,  0.061, 0.006 };
//static const float o[6] = {  -1.0, -0.6667, -0.3333, 0.3333, 0.6667,   1.0 };


#define SSSS_N_SAMPLES 11
static const float4 kernel[] = {
    float4(0.560479, 0.669086, 0.784728, 0),
    float4(0.00471691, 0.000184771, 5.07566e-005, -2),
    float4(0.0192831, 0.00282018, 0.00084214, -1.28),
    float4(0.03639, 0.0130999, 0.00643685, -0.72),
    float4(0.0821904, 0.0358608, 0.0209261, -0.32),
    float4(0.0771802, 0.113491, 0.0793803, -0.08),
    float4(0.0771802, 0.113491, 0.0793803, 0.08),
    float4(0.0821904, 0.0358608, 0.0209261, 0.32),
    float4(0.03639, 0.0130999, 0.00643685, 0.72),
    float4(0.0192831, 0.00282018, 0.00084214, 1.28),
    float4(0.00471691, 0.000184771, 5.07565e-005, 2),
};

float4 main(VertexToPixelPostProcess input) : SV_TARGET
{
    // Fetch color and linear depth for current pixel:
    float4 colorM = xTexture.Sample(Sampler, input.tex);
	float depthM = texture_lineardepth[input.pos.xy];
	float sss = texture_gbuffer0[input.pos.xy].a;
	sss *= 0.01f;

    // Accumulate center sample, multiplying it with its gaussian weight:
    float4 colorBlurred = colorM;
    colorBlurred.rgb *= kernel[0].rgb;

    // Calculate the step that we will use to fetch the surrounding pixels,
    // where "step" is:
    //     step = sssStrength * gaussianWidth * pixelSize * dir
    // The closer the pixel, the stronger the effect needs to be, hence
    // the factor 1.0 / depthM.
	float2 step = xPPParams0.xy * sss;
    float2 finalStep = colorM.a * step / depthM;

    // Accumulate the other samples:
    [unroll]
    for (int i = 1; i < SSSS_N_SAMPLES; i++) {
        // Fetch color and depth for current sample:
        float2 offset = input.tex + kernel[i].a * finalStep;
        float3 color = xTexture.SampleLevel(Sampler, offset, 0).rgb;
        float depth = ( texture_lineardepth.SampleLevel(Sampler,offset,0).r );

        // If the difference in depth is huge, we lerp color back to "colorM":
		static const float correction = 500;
        float s = min(0.0125 * correction * abs(depthM - depth), 1.0);
        color = lerp(color, colorM.rgb, s);

        // Accumulate:
        colorBlurred.rgb += kernel[i].rgb * color.rgb;
    }

    return colorBlurred;
}