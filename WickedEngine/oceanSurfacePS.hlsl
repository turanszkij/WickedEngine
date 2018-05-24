#define DISABLE_ALPHATEST
#define DISABLE_DECALS
#define DISABLE_ENVMAPS
#define DISABLE_TRANSPARENT_SHADOWMAP
#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"
#include "objectHF.hlsli"

#define xGradientMap		texture_0

[earlydepthstencil]
float4 main(PSIn input) : SV_TARGET
{
	float2 gradient = xGradientMap.Sample(sampler_aniso_wrap, input.uv).xy;

	float4 color = float4(xOceanWaterColor, 1);
	float opacity = 1; // keep edge diffuse shading
	color.rgb = DEGAMMA(color.rgb);
	float3 V = g_xCamera_CamPos - input.pos3D;
	float dist = length(V);
	V /= dist;
	float emissive = 0;
	Surface surface = CreateSurface(input.pos3D, normalize(float3(gradient.x, xOceanTexelLength * 2, gradient.y)), V, color, 0.001, 0.02, 0);
	float ao = 1;
	float sss = 0;
	float2 pixel = input.pos.xy;
	float depth = input.pos.z;
	float3 diffuse = 0;
	float3 specular = 0;
	float3 reflection = 0;

	float lineardepth = input.pos2D.w;
	float2 refUV = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w * 0.5f + 0.5f;
	float2 ScreenCoord = float2(1, -1) * input.pos2D.xy / input.pos2D.w * 0.5f + 0.5f;

	//REFLECTION
	float2 RefTex = float2(1, -1)*input.ReflectionMapSamplingPos.xy / input.ReflectionMapSamplingPos.w / 2.0f + 0.5f;
	float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror, RefTex + surface.N.xz * 0.04f, 0);

	//REFRACTION 
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + surface.N.xz * 0.04f;
	float refDepth = texture_lineardepth.Sample(sampler_linear_mirror, ScreenCoord) * g_xFrame_MainCamera_ZFarP;
	float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
	float mod = saturate(0.05*(refDepth - lineardepth));
	refractiveColor = lerp(refractiveColor, surface.baseColor.rgb, mod).rgb;

	//FRESNEL TERM
	float NdotV = abs(dot(surface.N, surface.V));
	float3 fresnelTerm = F_Fresnel(surface.f0, NdotV);
	float ramp = pow(abs(1.0f / (1.0f + NdotV)), 16);
	reflectiveColor.rgb = lerp(float3(0.38f, 0.45f, 0.56f), reflectiveColor.rgb, ramp); // skycolor hack
	surface.albedo.rgb = lerp(refractiveColor, reflectiveColor.rgb, fresnelTerm);

	TiledLighting(pixel, surface, diffuse, specular, reflection);

	specular += reflection * fresnelTerm;

	ApplyLighting(surface, diffuse, specular, ao, color);

	//SOFT EDGE
	float fade = saturate(0.3 * abs(refDepth - lineardepth));
	color.a *= fade;

	ApplyFog(dist, color);

	return color;
}

