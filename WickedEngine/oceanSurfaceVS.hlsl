#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

VS_OUTPUT main(float2 vPos : POSITION)
{
	VS_OUTPUT Output;

	// Local position
	float4 pos_local = mul(float4(vPos, 0, 1), g_matLocal);
	// UV
	float2 uv_local = pos_local.xy * g_UVScale + g_UVOffset;

	// Blend displacement to avoid tiling artifact
	float3 eye_vec = pos_local.xyz - g_LocalEye;
	float dist_2d = length(eye_vec.xy);
	float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
	blend_factor = clamp(blend_factor, 0, 1);

	// Add perlin noise to distant patches
	float perlin = 0;
	if (blend_factor < 1)
	{
		float2 perlin_tc = uv_local * g_PerlinSize + g_UVBase;
		float perlin_0 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.x + g_PerlinMovement, 0).w;
		float perlin_1 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.y + g_PerlinMovement, 0).w;
		float perlin_2 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.z + g_PerlinMovement, 0).w;

		perlin = perlin_0 * g_PerlinAmplitude.x + perlin_1 * g_PerlinAmplitude.y + perlin_2 * g_PerlinAmplitude.z;
	}

	// Displacement map
	float3 displacement = 0;
	if (blend_factor > 0)
		displacement = g_texDisplacement.SampleLevel(sampler_point_wrap, uv_local, 0).xyz;
	displacement = lerp(float3(0, 0, perlin), displacement, blend_factor);
	pos_local.xyz += displacement;

	// Transform
	Output.Position = mul(pos_local, g_matWorldViewProj);
	Output.LocalPos = pos_local.xyz;

	// Pass thru texture coordinate
	Output.TexCoord = uv_local;

	return Output;
}
