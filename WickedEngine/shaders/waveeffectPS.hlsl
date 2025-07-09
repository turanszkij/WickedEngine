#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

PUSHCONSTANT(postprocess, PostProcess);

float4 main(in float4 pos : SV_Position, in float2 uv : TEXCOORD) : SV_Target
{
	uv.y -= 0.26 * (1 - sqr(uv.x));
	float time = GetTime() * 0.32;
	
	float wave = 0;
	const uint num = 2;
	for (uint i = 0; i < num; ++i)
	{
		float2 clipspace = uv_to_clipspace(uv) + sin(uv.x * 3 + time * 0.8 + i * 5.234) * 0.34 * (i * 1.2334);
		float linne = sin(clipspace.x * 4 + time * 0.6);
		float dist = clipspace.y * 10 - linne;
		wave += smoothstep(1.0, 0.0, abs(dist * (dist < 0 ? 0.01 : 10))); // long gradient down
		wave += smoothstep(1.0, 0.0, abs(dist * (dist < 0 ? 0.6 : 10))); // short gradient down
	}

	wave /= float(num * 2);
	
	float4 color = saturate(wave) * postprocess.params0;
	color.rgb *= color.a;
	color.a = 1;
	return color;
}
