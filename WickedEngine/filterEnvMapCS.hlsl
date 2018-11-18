#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

// Hemisphere point generation from:
//	http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

float radicalInverse_VdC(uint bits) {
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
float2 hammersley2d(uint i, uint N) {
	return float2(float(i) / float(N), radicalInverse_VdC(i));
}

float3 hemisphereSample_uniform(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = 1.0 - u;
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

float3 hemisphereSample_cos(float u, float v) {
	float phi = v * 2.0 * PI;
	float cosTheta = sqrt(1.0 - u);
	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
	return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

inline float3x3 GetTangentSpace(float3 normal)
{
	// Choose a helper vector for the cross product
	float3 helper = abs(normal.x) > 0.99f ? float3(0, 0, 1) : float3(1, 0, 0);

	// Generate vectors
	float3 tangent = normalize(cross(normal, helper));
	float3 binormal = normalize(cross(normal, tangent));
	return float3x3(tangent, binormal, normal);
}

TEXTURECUBEARRAY(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2DARRAY(output, float4, 0);

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < filterResolution.x && DTid.y < filterResolution.y)
	{
		float2 uv = (DTid.xy + 0.5f) / filterResolution.xy;
		float3 N = UV_to_CubeMap(uv, DTid.z);

		float3x3 tangentSpace = GetTangentSpace(N);

		float4 col = 0;

		for (uint i = 0; i < filterRayCount; ++i)
		{
			float2 hamm = hammersley2d(i, filterRayCount);
			float3 hemisphere = hemisphereSample_cos(hamm.x, hamm.y);
			float3 cone = mul(hemisphere, tangentSpace);
			cone = lerp(N, cone, filterRoughness);

			col += input.SampleLevel(sampler_linear_clamp, float4(cone, filterArrayIndex), 0);
		}
		col /= (float)filterRayCount;

		output[uint3(DTid.xy, DTid.z + filterArrayIndex * 6)] = col;
	}
}
