#define TRANSPARENT // uses transparent light lists
#define LIGHTING_SCATTER
#include "globals.hlsli"
#include "emittedparticleHF.hlsli"
#include "ShaderInterop_EmittedParticle.h"
#include "objectHF.hlsli"

#ifdef EMITTEDPARTICLE_DISTORTION
static const uint SLOT = NORMALMAP;
#else
static const uint SLOT = BASECOLORMAP;
#endif // EMITTEDPARTICLE_DISTORTION

[earlydepthstencil]
float4 main(VertextoPixel input) : SV_TARGET
{
	// Blocker shadow map check:
	[branch]
	if ((xEmitterOptions & EMITTER_OPTION_BIT_USE_RAIN_BLOCKER) && rain_blocker_check(input.P))
	{
		return 0;
	}

	float2 ScreenCoord = input.pos.xy * GetCamera().internal_resolution_rcp; // use pixel center!
	uint2 pixel = input.pos.xy; // no longer pixel center!
	
	write_mipmap_feedback(EmitterGetGeometry().materialIndex, ddx_coarse(input.tex.xyxy), ddy_coarse(input.tex.xyxy));
	
	ShaderMaterial material = EmitterGetMaterial();

	float4 color = 1;

	[branch]
	if (material.textures[SLOT].IsValid())
	{
		color = material.textures[SLOT].Sample(sampler_linear_clamp, input.tex.xyxy);

		[branch]
		if (xEmitterOptions & EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED)
		{
			float4 color2 = material.textures[SLOT].Sample(sampler_linear_clamp, input.tex.zwzw);
			color = lerp(color, color2, input.frameBlend);
		}
	}
	
	float4 inputColor;
	inputColor.r = ((input.color >> 0)  & 0xFF) / 255.0f;
	inputColor.g = ((input.color >> 8)  & 0xFF) / 255.0f;
	inputColor.b = ((input.color >> 16) & 0xFF) / 255.0f;
	inputColor.a = ((input.color >> 24) & 0xFF) / 255.0f;

	float opacity = color.a * inputColor.a;

	float3 normal = 0;
#ifndef EMITTEDPARTICLE_DISTORTION // the "distortion" shader is just using normal map as the color map and uses it for signed blending, this normal logic won't be used for that
	[branch]
	if (material.textures[NORMALMAP].IsValid())
	{
		normal = material.textures[NORMALMAP].Sample(sampler_linear_clamp, input.tex.xyxy).rgb;

		[branch]
		if (xEmitterOptions & EMITTER_OPTION_BIT_FRAME_BLENDING_ENABLED)
		{
			float3 normal2 = material.textures[NORMALMAP].Sample(sampler_linear_clamp, input.tex.zwzw).rgb;
			normal = lerp(normal, normal2, input.frameBlend);
		}

		normal -= 0.5;
		normal *= material.GetNormalMapStrength();
		normal *= opacity;
	}
#endif // EMITTEDPARTICLE_DISTORTION
	
	[branch]
	if (GetCamera().texture_lineardepth_index >= 0)
	{
		float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, ScreenCoord) * GetCamera().z_far;
		float depthFragment = input.pos.w;
		opacity *= saturate(1.0 / input.size * (max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));
	}

	opacity = saturate(opacity);

	color.rgb *= inputColor.rgb * (1 + material.GetEmissive());
	color.a = opacity;

#ifdef EMITTEDPARTICLE_DISTORTION
	// just make normal maps blendable:
	color.rgb = color.rgb - 0.5f;
#endif // EMITTEDPARTICLE_DISTORTION

#ifdef EMITTEDPARTICLE_LIGHTING

	[branch]
	if (color.a > 0)
	{
		float3 N;
		N.x = -cos(PI * input.unrotated_uv.x);
		N.y = cos(PI * input.unrotated_uv.y);
		N.z = -sin(PI * length(input.unrotated_uv));
		N.xz += normal.rg;
		N = mul((float3x3)GetCamera().inverse_view, N);
		N = normalize(N);
		
		float3 V = GetCamera().position - input.P;
		float dist = length(V);
		V /= dist;

		Lighting lighting;
		lighting.create(0, 0, GetAmbient(N), 0);

		Surface surface;
		surface.init();
		surface.create(material, color, surfacemap_simple);
		surface.P = input.P;
		surface.N = N;
		surface.V = V;
		surface.pixel = pixel;
		surface.sss = material.GetSSS();
		surface.sss_inv = material.GetSSSInverse();
		surface.extinction = 0;
		surface.update();

		TiledLighting(surface, lighting, GetFlatTileIndex(pixel));

		color.rgb *= lighting.direct.diffuse + lighting.indirect.diffuse;
		color.rgb += lighting.indirect.specular;

		//color.rgb = float3(unrotated_uv, 0);
		//color.rgb = float3(input.tex, 0);

		ApplyFog(dist, V, color);

		color = max(0, color);
	}

#endif // EMITTEDPARTICLE_LIGHTING

	return color;
}
