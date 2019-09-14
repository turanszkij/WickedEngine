#include "globals.hlsli"
#include "brdf.hlsli"
#include "objectHF.hlsli"
#include "ShaderInterop_Postprocess.h"


float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	float4 g0 = texture_gbuffer0[pos.xy];
	float4 g2 = texture_gbuffer2[pos.xy];
	float3 albedo = ComputeAlbedo(float4(g0.rgb, 1), g2.b, g2.a);

	float  depth = texture_lineardepth[pos.xy].r * g_xCamera_ZFarP;

	float3 diffuse = texture_0[pos.xy].rgb; // light diffuse
	float3 specular = texture_1[pos.xy].rgb; // light specular

	float4 color = float4(albedo * diffuse + specular, 1);

	ApplyFog(depth, color);

	return color;
}