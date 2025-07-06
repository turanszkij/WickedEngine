#include "globals.hlsli"
#include "ShaderInterop_PostProcess.h"

// Based on: https://www.shadertoy.com/view/tl2Bzt

PUSHCONSTANT(postprocess, PostProcess);

float4 main(in float4 pos : SV_Position, in float2 uv_screen : TEXCOORD) : SV_Target
{
	float iTime = GetTime() * 0.25;
	float2 uv = 2.0 * uv_screen - 1.0 - sin(uv_screen.x * 5 + iTime * 1.8) * 0.4;

	float waves = 0;
	for (uint i = 0; i < 6; i++)
	{
		uv.y += (0.1 * sin(uv.x * 4 + i * 2 - iTime * 0.1));
		float li = 0.025 + smoothstep(0.0, 0.8, pow(abs(fmod(uv_screen.x + i * 0.1 + iTime, 2.0) - 1.0), 2.0));
		float gw = li / (32.0 * uv.y);
		float sgw = sign(gw) * (i % 2 == 0 ? 1 : -1);
		gw = abs(gw);
		gw = smoothstep(sgw < 0 ? 0.2 : 0.0, 0.6, gw);
		float ts = gw * (sin(iTime * 4) * 0.5 + 0.5 + 1.4);
		waves += ts;
	}

	return saturate(waves) * postprocess.params0;
}
