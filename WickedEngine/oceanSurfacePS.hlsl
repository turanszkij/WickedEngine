#define DISABLE_ALPHATEST
#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"
#include "objectHF.hlsli"

#define xGradientMap		texture_0
TEXTURE1D(g_texFresnel, float4, TEXSLOT_ONDEMAND1);

[earlydepthstencil]
float4 main(PSIn input) : SV_TARGET
{
	float2 gradient = xGradientMap.Sample(sampler_aniso_wrap, input.uv).xy;
	float3 N = normalize(float3(gradient.x, xOceanTexelLength * 2, gradient.y));

	float4 baseColor = float4(xOceanWaterColor, 1);
	float opacity = 1; // keep edge diffuse shading
	baseColor = DEGAMMA(baseColor);
	baseColor.a = 1; // do not blend
	float4 color = baseColor;
	float3 P = input.pos3D;
	float3 V = g_xCamera_CamPos - P;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	float roughness = 0.001;
	float reflectance = 0.02;
	float metalness = 0;
	float ao = 1;
	float sss = 0;
	float2 pixel = input.pos.xy;
	float depth = input.pos.z;
	float3 diffuse = 0;
	float3 specular = 0;
	//float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy / input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);
	float2 velocity = 0;

	float lineardepth = input.pos2D.w;
	float2 refUV = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w * 0.5f + 0.5f;
	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w * 0.5f + 0.5f;

	OBJECT_PS_LIGHT_BEGIN

	//REFLECTION
	float2 RefTex = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
	float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror, RefTex + N.xz * 0.04f, 0);

	//REFRACTION 
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + N.xz * 0.04f;
	float refDepth = (texture_lineardepth.Sample(sampler_linear_mirror, ScreenCoord));
	float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
	float mod = saturate(0.05*(refDepth - lineardepth));
	refractiveColor = lerp(refractiveColor, baseColor.rgb, mod).rgb;

	//FRESNEL TERM
	float NdotV = abs(dot(N, V));
	float3 fresnelTerm = F_Fresnel(f0, NdotV);
	float ramp = pow(abs(1.0f / (1.0f + NdotV)), 16);
	reflectiveColor.rgb = lerp(float3(0.38f, 0.45f, 0.56f), reflectiveColor.rgb, ramp); // skycolor hack
	albedo.rgb = lerp(refractiveColor, reflectiveColor.rgb, fresnelTerm);

	OBJECT_PS_LIGHT_TILED

	OBJECT_PS_LIGHT_END

	//SOFT EDGE
	float fade = saturate(0.3 * abs(refDepth - lineardepth));
	color.a *= fade;

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD_SIMPLE



	//float3 g_SkyColor = float3(0.38f, 0.45f, 0.56f);

	//float g_Shineness = 400.0f;





	//float2 fft_tc = input.uv;

	//// Reflected ray
	//float3 reflect_vec = -reflect(V, N);
	//// dot(N, V)
	//float cos_angle = abs(dot(N, V));


	//// --------------- Reflected color

	//// ramp.x for fresnel term. ramp.y for sky blending
	//float2 ramp;
	//ramp.x = F_Fresnel(f0, cos_angle).x;
	//ramp.y = pow(abs(1.0f / (1.0f + cos_angle)), 16);

	//float3 reflection = texture_env_global.Sample(sampler_linear_clamp, reflect_vec).xyz;

	//// Blend with predefined sky color
	//float3 reflected_color = lerp(g_SkyColor, reflection, ramp.y);

	//// Combine waterbody color and reflected color
	//float3 water_color = lerp(refractiveColor, reflected_color, ramp.x);


	//// --------------- Sun spots

	//float cos_spec = clamp(dot(reflect_vec, GetSunDirection()), 0, 1);
	//float sun_spot = pow(cos_spec, g_Shineness);
	//water_color += GetSunColor() * sun_spot;


	////SOFT EDGE
	//float fade = saturate(0.3 * abs(refDepth - lineardepth));

	//return float4(water_color, 1);
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

