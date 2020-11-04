#include "globals.hlsli"
#include "brdf.hlsli"
#include "objectHF.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_diffuse, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(texture_specular, float4, TEXSLOT_ONDEMAND1);

float4 main(float4 pos : SV_Position, float2 uv : TEXCOORD) : SV_TARGET
{
	float3 albedo = texture_gbuffer0[pos.xy].rgb;
	float  depth = texture_depth[pos.xy];
	float3 diffuse = texture_diffuse[pos.xy].rgb;
	float3 specular = texture_specular[pos.xy].rgb;

	float4 color = float4(albedo * diffuse + specular, 1);

	const float3 P = reconstructPosition(uv, depth);
	ApplyFog(distance(P, g_xCamera_CamPos), color);

	return max(0, color);
}