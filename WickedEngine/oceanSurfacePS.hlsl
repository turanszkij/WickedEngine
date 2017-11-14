#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"


#define g_texGradient		texture_0 // Perlin wave displacement & gradient map in both


float4 main(GSOut input) : SV_TARGET
{
	float3 gradient = g_texGradient.Sample(sampler_aniso_wrap, input.uv).xyz;

	return float4(gradient, 1);
}


//// Nvidia:
//float4 main(VS_OUTPUT In) : SV_Target
//{
//	// Calculate eye vector.
//	float3 eye_vec = g_LocalEye - In.LocalPos;
//	float3 eye_dir = normalize(eye_vec);
//
//
//	// --------------- Blend perlin noise for reducing the tiling artifacts
//
//	// Blend displacement to avoid tiling artifact
//	float dist_2d = length(eye_vec.xy);
//	float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
//	blend_factor = clamp(blend_factor * blend_factor * blend_factor, 0, 1);
//
//	// Compose perlin waves from three octaves
//	float2 perlin_tc = In.TexCoord * g_PerlinSize + g_UVBase;
//	float2 perlin_tc0 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.x + g_PerlinMovement : 0;
//	float2 perlin_tc1 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.y + g_PerlinMovement : 0;
//	float2 perlin_tc2 = (blend_factor < 1) ? perlin_tc * g_PerlinOctave.z + g_PerlinMovement : 0;
//
//	float2 perlin_0 = g_texPerlin.Sample(sampler_aniso_wrap, perlin_tc0).xy;
//	float2 perlin_1 = g_texPerlin.Sample(sampler_aniso_wrap, perlin_tc1).xy;
//	float2 perlin_2 = g_texPerlin.Sample(sampler_aniso_wrap, perlin_tc2).xy;
//
//	float2 perlin = (perlin_0 * g_PerlinGradient.x + perlin_1 * g_PerlinGradient.y + perlin_2 * g_PerlinGradient.z);
//
//
//	// --------------- Water body color
//
//	// Texcoord mash optimization: Texcoord of FFT wave is not required when blend_factor > 1
//	float2 fft_tc = (blend_factor > 0) ? In.TexCoord : 0;
//
//	float2 grad = g_texGradient.Sample(sampler_aniso_wrap, fft_tc).xy;
//	grad = lerp(perlin, grad, blend_factor);
//
//	// Calculate normal here.
//	float3 normal = normalize(float3(grad, g_TexelLength_x2));
//	// Reflected ray
//	float3 reflect_vec = reflect(-eye_dir, normal);
//	// dot(N, V)
//	float cos_angle = dot(normal, eye_dir);
//
//	// A coarse way to handle transmitted light
//	float3 body_color = g_WaterbodyColor;
//
//
//	// --------------- Reflected color
//
//	// ramp.x for fresnel term. ramp.y for sky blending
//	float4 ramp = g_texFresnel.Sample(sampler_linear_clamp, cos_angle).xyzw;
//	// A workaround to deal with "indirect reflection vectors" (which are rays requiring multiple
//	// reflections to reach the sky).
//	if (reflect_vec.z < g_BendParam.x)
//		ramp = lerp(ramp, g_BendParam.z, (g_BendParam.x - reflect_vec.z) / (g_BendParam.x - g_BendParam.y));
//	reflect_vec.z = max(0, reflect_vec.z);
//
//	float3 reflection = g_texReflectCube.Sample(sampler_linear_clamp, reflect_vec).xyz;
//	// Hack bit: making higher contrast
//	reflection = reflection * reflection * 2.5f;
//
//	// Blend with predefined sky color
//	float3 reflected_color = lerp(g_SkyColor, reflection, ramp.y);
//
//	// Combine waterbody color and reflected color
//	float3 water_color = lerp(body_color, reflected_color, ramp.x);
//
//
//	// --------------- Sun spots
//
//	float cos_spec = clamp(dot(reflect_vec, g_SunDir), 0, 1);
//	float sun_spot = pow(cos_spec, g_Shineness);
//	water_color += g_SunColor * sun_spot;
//
//
//	return float4(water_color, 1);
//}

