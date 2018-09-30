#include "postProcessHF.hlsli"

static const float pWid = 100;
static const float pHei = 100;

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float maxStep = 32.;
	float d = 0.;

	float2 uv = PSIn.pos.xy;

	for (int count = 0; count < 100; count++) {
		if (uv.x < pWid)
			break;

		float d = 1.0 - saturate(texture_lineardepth.SampleLevel(sampler_linear_clamp, uv / g_xFrame_ScreenWidthHeight.xy, 0).r * 100);

		uv.x -= pWid - (d * maxStep);
	}

	float x = (uv.x % pWid) / pWid;
	float y = (uv.y % pHei) / pHei;
	float3 rgb = xTexture.SampleLevel(sampler_linear_wrap, float2(x, y), 0).yxz;

	float4 fragColor = float4(rgb, 1.0);


	return fragColor;
}