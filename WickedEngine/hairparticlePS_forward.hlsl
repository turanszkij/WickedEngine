#include "globals.hlsli"
#include "objectHF.hlsli"
#include "hairparticleHF.hlsli"
#include "ditherHF.hlsli"

GBUFFEROutputType_Thin main(VertexToPixel input)
{
#ifdef GRASS_FADE_DITHER
	clip(dither(input.pos.xy) - input.fade);
#endif

	float4 color = texture_0.Sample(sampler_linear_clamp, input.tex);
	color.rgb = DEGAMMA(color.rgb);
	color.rgb *= input.color;
	ALPHATEST(color.a)
	float opacity = 1; // keep edge diffuse shading
	float3 V = g_xCamera_CamPos - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	Surface surface = CreateSurface(input.pos3D, input.nor, V, color, 1, 1, 0, 0);
	Lighting lighting = CreateLighting(0, 0, GetAmbient(surface.N), 0);
	float2 pixel = input.pos.xy;
	float depth = input.pos.z;
	float2 velocity = ((input.pos2DPrev.xy / input.pos2DPrev.w - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy / input.pos2D.w - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);

	ForwardLighting(surface, lighting);

	ApplyLighting(surface, lighting, color);

	ApplyFog(dist, color);

	return CreateGbuffer_Thin(color, surface, velocity);
}