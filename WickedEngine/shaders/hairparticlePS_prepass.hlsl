#include "globals.hlsli"
#include "hairparticleHF.hlsli"

TEXTURE2D(texture_color, float4, TEXSLOT_ONDEMAND0);

float2 main(VertexToPixel input) : SV_TARGET
{
	const float lineardepth = input.pos.w;

	clip(dither(input.pos.xy + GetTemporalAASampleRotation()) - input.fade);

	float4 color = texture_color.Sample(sampler_linear_clamp,input.tex);

	float alphatest = g_xMaterial.alphaTest;
	if (g_xFrame_Options & OPTION_BIT_TEMPORALAA_ENABLED)
	{
		alphatest = clamp(blue_noise(input.pos.xy, lineardepth).r, 0, 0.99);
	}
	clip(color.a - alphatest);

	float2 ScreenCoord = input.pos.xy * g_xFrame_InternalResolution_rcp;
	float2 pos2D = ScreenCoord * 2 - 1;
	pos2D.y *= -1;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);
	return velocity;
}
