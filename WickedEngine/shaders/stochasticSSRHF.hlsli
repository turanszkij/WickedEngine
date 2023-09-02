#ifndef WI_STOCHASTICSSR_HF
#define WI_STOCHASTICSSR_HF
#include "brdf.hlsli"
#include "ShaderInterop_Postprocess.h"

#define GGX_SAMPLE_VISIBLE

// Bias used on GGX importance sample when denoising, to remove part of the tale that create a lot more noise.
#define GGX_IMPORTANCE_SAMPLE_BIAS 0.1

// Shared Reflection settings:

uint2 GetReflectionIndirectDispatchCoord(uint3 Gid, uint3 GTid, StructuredBuffer<uint> tiles, uint downsample)
{
	uint tile_replicate = sqr(SSR_TILESIZE / downsample / POSTPROCESS_BLOCKSIZE);
	uint tile_idx = Gid.x / tile_replicate;
	uint tile_packed = tiles[tile_idx];
	uint2 tile = uint2(tile_packed & 0xFFFF, (tile_packed >> 16) & 0xFFFF);
	uint subtile_idx = Gid.x % tile_replicate;
	uint2 subtile = unflatten2D(subtile_idx, SSR_TILESIZE / downsample / POSTPROCESS_BLOCKSIZE);
	uint2 subtile_upperleft = tile * SSR_TILESIZE / downsample + subtile * POSTPROCESS_BLOCKSIZE;
	return subtile_upperleft + unflatten2D(GTid.x, POSTPROCESS_BLOCKSIZE);
}

bool NeedReflection(float roughness, float depth, float roughness_cutoff)
{
	return (roughness <= roughness_cutoff) && (depth > 0.0);
}

// Brian Karis, Epic Games "Real Shading in Unreal Engine 4"
float4 ImportanceSampleGGX(float2 Xi, float Roughness)
{
	Roughness = clamp(Roughness, 0.045, 1);
	float m = Roughness * Roughness;
	float m2 = m * m;

	float Phi = 2 * PI * Xi.x;

	float CosTheta = sqrt((1.0 - Xi.y) / (1.0 + (m2 - 1.0) * Xi.y));
	float SinTheta = sqrt(max(1e-5, 1.0 - CosTheta * CosTheta));

	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi);
	H.z = CosTheta;

	float d = (CosTheta * m2 - CosTheta) * CosTheta + 1;
	float D = m2 / (PI * d * d);
	float pdf = D * CosTheta;

	return float4(H, pdf);
}

// [ Duff et al. 2017, "Building an Orthonormal Basis, Revisited" ]
// http://jcgt.org/published/0006/01/01/
float3x3 GetTangentBasis(float3 TangentZ)
{
	const float Sign = TangentZ.z >= 0 ? 1 : -1;
	const float a = -rcp(Sign + TangentZ.z);
	const float b = TangentZ.x * TangentZ.y * a;

	float3 TangentX = { 1 + Sign * a * pow(TangentZ.x, 2), Sign * b, -Sign * TangentZ.x };
	float3 TangentY = { b, Sign + a * pow(TangentZ.y, 2), -TangentZ.y };

	return float3x3(TangentX, TangentY, TangentZ);
}

float2 SampleDisk(float2 Xi)
{
	float theta = 2 * PI * Xi.x;
	float radius = sqrt(Xi.y);
	return radius * float2(cos(theta), sin(theta));
}

// Adapted from: "Sampling the GGX Distribution of Visible Normals", by E. Heitz
// http://jcgt.org/published/0007/04/01/paper.pdf
float4 ImportanceSampleVisibleGGX(float2 diskXi, float roughness, float3 V)
{
	roughness = clamp(roughness, 0.045, 1);
	float alphaRoughness = roughness * roughness;
	float alphaRoughnessSq = alphaRoughness * alphaRoughness;

	// Transform the view direction to hemisphere configuration
	float3 Vh = normalize(float3(alphaRoughness * V.xy, V.z));

	// Orthonormal basis
	// tangent0 is orthogonal to N.
	float3 tangent0 = (Vh.z < 0.9999) ? normalize(cross(float3(0, 0, 1), Vh)) : float3(1, 0, 0);
	float3 tangent1 = cross(Vh, tangent0);

	float2 p = diskXi;
	float s = 0.5 + 0.5 * Vh.z;
	p.y = (1 - s) * sqrt(1 - p.x * p.x) + s * p.y;

	// Reproject onto hemisphere
	float3 H;
	H = p.x * tangent0;
	H += p.y * tangent1;
	H += sqrt(saturate(1 - dot(p, p))) * Vh;

	// Transform the normal back to the ellipsoid configuration
	H = normalize(float3(alphaRoughness * H.xy, max(0.0, H.z)));

	float NdotV = V.z;
	float NdotH = H.z;
	float VdotH = dot(V, H);

	// Microfacet Distribution
	float f = (NdotH * alphaRoughnessSq - NdotH) * NdotH + 1;
	float D = alphaRoughnessSq / (PI * f * f);

	// Smith Joint masking function
	float SmithGGXMasking = 2.0 * NdotV / (sqrt(NdotV * (NdotV - NdotV * alphaRoughnessSq) + alphaRoughnessSq) + NdotV);

	// D_Ve(Ne) = G1(Ve) * max(0, dot(Ve, Ne)) * D(Ne) / Ve.z
	float PDF = SmithGGXMasking * VdotH * D / NdotV;

	return float4(H, PDF);
}

float4 ReflectionDir_GGX(float3 V, float3 N, float roughness, float2 random2)
{
	roughness = clamp(roughness, 0.045, 1);
	float4 H;
	float3 L;
	if (roughness > 0.05f)
	{
		float3x3 tangentBasis = GetTangentBasis(N);
		float3 tangentV = mul(tangentBasis, V);
		float2 Xi = random2;
		Xi.y = lerp(Xi.y, 0.0f, GGX_IMPORTANCE_SAMPLE_BIAS);
		H = ImportanceSampleVisibleGGX(SampleDisk(Xi), roughness, tangentV);
		H.xyz = mul(H.xyz, tangentBasis);
		L = reflect(-V, H.xyz);
	}
	else
	{
		H = float4(N.xyz, 1.0f);
		L = reflect(-V, H.xyz);
	}
	float PDF = H.w;
	return float4(L, PDF);
}

float Luminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

#endif // WI_STOCHASTICSSR_HF
