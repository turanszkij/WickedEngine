#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "objectHF.hlsli"

float4 main(VertextoPixel input) : SV_TARGET
{
#ifdef SPIRV
	input.pos.w = rcp(input.pos.w);
#endif // SPIRV

	float4 color = texture_0.Sample(sampler_linear_clamp, input.tex.xy);

	[branch]
	if (xEmitterOptions & EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED)
	{
		float4 color2 = texture_0.Sample(sampler_linear_clamp, input.tex.zw);
		color = lerp(color, color2, input.frameBlend);
	}

	clip(color.a - 1.0f / 255.0f);

	float2 pixel = input.pos.xy;
	float2 ScreenCoord = pixel * g_xFrame_InternalResolution_rcp;
	float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, ScreenCoord) * g_xCamera_ZFarP;
	float depthFragment = input.pos.w;
	float fade = saturate(1.0 / input.size*(max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));

	float4 inputColor;
	inputColor.r = ((input.color >> 0)  & 0x000000FF) / 255.0f;
	inputColor.g = ((input.color >> 8)  & 0x000000FF) / 255.0f;
	inputColor.b = ((input.color >> 16) & 0x000000FF) / 255.0f;
	inputColor.a = ((input.color >> 24) & 0x000000FF) / 255.0f;

	float opacity = saturate(color.a * inputColor.a * fade);

	color.rgb *= inputColor.rgb * (1 + xParticleEmissive);
	color.a = opacity;

#ifdef EMITTEDPARTICLE_DISTORTION
	// just make normal maps blendable:
	color.rgb = color.rgb - 0.5f;
#endif // EMITTEDPARTICLE_DISTORTION

#ifdef EMITTEDPARTICLE_LIGHTING

	float3 N;
	N.x = -cos(PI * input.unrotated_uv.x);
	N.y = cos(PI * input.unrotated_uv.y);
	N.z = -sin(PI * length(input.unrotated_uv));
	N = mul((float3x3)g_xCamera_InvV, N);
	N = normalize(N);

	Lighting lighting = CreateLighting(0, 0, GetAmbient(N), 0);
	Surface surface = CreateSurface(input.P, N, 0, color, 1, 1, 0, 0);
	surface.pixel = pixel;

	TiledLighting(surface, lighting);

	color.rgb *= lighting.direct.diffuse + lighting.indirect.diffuse;

	//color.rgb = float3(unrotated_uv, 0);
	//color.rgb = float3(input.tex, 0);

#endif // EMITTEDPARTICLE_LIGHTING

	return max(color, 0);
}
