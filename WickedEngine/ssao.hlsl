#include "postProcessHF.hlsli"
#include "reconstructPositionHF.hlsli"

// Define this to use reduced precision, but faster depth buffer:
#define USE_LINEARDEPTH

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

float main(VertexToPixelPostProcess input) : SV_Target
{
	const float range = xPPParams0.x;
	const uint sampleCount = xPPParams0.y;

	float seed = 1;
	const float3 noise = float3(rand(seed, input.tex), rand(seed, input.tex), rand(seed, input.tex)) * 2 - 1;
	const float3 P = getPosition(input.tex, texture_depth.SampleLevel(sampler_linear_clamp, input.tex, 0));
	//const float3 normal = decode(texture_gbuffer1.SampleLevel(sampler_linear_clamp, input.tex, 0).xy);
	const float3 normal = normalize(cross(ddx(P), ddy(P))); // instead of reading normals g-buffer, reconstruct flat normals from position

	const float3 tangent = normalize(noise - normal * dot(noise, normal));
	const float3 bitangent = cross(normal, tangent);
	const float3x3 tangentSpace = float3x3(tangent, bitangent, normal);

	float ao = 0;
	for (uint i = 0; i < sampleCount; ++i)
	{
		const float2 hamm = hammersley2d(i, sampleCount);
		const float3 hemisphere = hemisphereSample_uniform(hamm.x, hamm.y);
		const float3 cone = mul(hemisphere, tangentSpace);
		const float ray_range = range * lerp(0.2f, 1.0f, rand(seed, input.tex)); // modulate ray-length a bit to avoid uniform look
		const float3 sam = P + cone * ray_range;

		float4 vProjectedCoord = mul(float4(sam, 1.0f), g_xFrame_MainCamera_VP);
		vProjectedCoord.xyz /= vProjectedCoord.w;
		vProjectedCoord.xy = vProjectedCoord.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

		if (is_saturated(vProjectedCoord.xy))
		{
#ifdef USE_LINEARDEPTH
			const float ray_depth_real = vProjectedCoord.w; // .w is also linear depth, could be also written as getLinearDepth(vProjectedCoord.z)
			const float ray_depth_sample = texture_lineardepth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0) * g_xFrame_MainCamera_ZFarP;
			const float depth_fix = 1 - saturate(abs(ray_depth_real - ray_depth_sample) * 0.2f); // too much depth difference cancels the effect
			ao += (ray_depth_sample < ray_depth_real) * depth_fix;
#else
			const float ray_depth_real = vProjectedCoord.z;
			const float ray_depth_sample = texture_depth.SampleLevel(sampler_point_clamp, vProjectedCoord.xy, 0);
			const float depth_fix = 1 - saturate(abs(vProjectedCoord.w - getLinearDepth(ray_depth_sample)) * 0.2f); // too much depth difference cancels the effect (vProjectedCoord.w is also linear depth)
			ao += (ray_depth_sample > ray_depth_real) * depth_fix;
#endif // USE_LINEARDEPTH
		}
	}
	ao /= (float)sampleCount;

	return saturate(1 - ao);
}
