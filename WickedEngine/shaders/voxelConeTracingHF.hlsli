#ifndef WI_VOXEL_CONERACING_HF
#define WI_VOXEL_CONERACING_HF
#include "globals.hlsli"

// With help from: https://github.com/compix/VoxelConeTracingGI/blob/master/assets/shaders/voxelConeTracing/finalLightingPass.frag

#ifndef VOXEL_INITIAL_OFFSET
#define VOXEL_INITIAL_OFFSET 1
#endif // VOXEL_INITIAL_OFFSET

//#define USE_32_CONES
#ifdef USE_32_CONES
// 32 Cones for higher quality (16 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 32;
static const float DIFFUSE_CONE_APERTURE = 0.628319;

static const float3 DIFFUSE_CONE_DIRECTIONS[32] = {
	float3(0.898904, 0.435512, 0.0479745),
	float3(0.898904, -0.435512, -0.0479745),
	float3(0.898904, 0.0479745, -0.435512),
	float3(0.898904, -0.0479745, 0.435512),
	float3(-0.898904, 0.435512, -0.0479745),
	float3(-0.898904, -0.435512, 0.0479745),
	float3(-0.898904, 0.0479745, 0.435512),
	float3(-0.898904, -0.0479745, -0.435512),
	float3(0.0479745, 0.898904, 0.435512),
	float3(-0.0479745, 0.898904, -0.435512),
	float3(-0.435512, 0.898904, 0.0479745),
	float3(0.435512, 0.898904, -0.0479745),
	float3(-0.0479745, -0.898904, 0.435512),
	float3(0.0479745, -0.898904, -0.435512),
	float3(0.435512, -0.898904, 0.0479745),
	float3(-0.435512, -0.898904, -0.0479745),
	float3(0.435512, 0.0479745, 0.898904),
	float3(-0.435512, -0.0479745, 0.898904),
	float3(0.0479745, -0.435512, 0.898904),
	float3(-0.0479745, 0.435512, 0.898904),
	float3(0.435512, -0.0479745, -0.898904),
	float3(-0.435512, 0.0479745, -0.898904),
	float3(0.0479745, 0.435512, -0.898904),
	float3(-0.0479745, -0.435512, -0.898904),
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, 0.57735, -0.57735),
	float3(0.57735, -0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, 0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.57735, -0.57735, -0.57735)
};
#else // 16 cones for lower quality (8 on average per hemisphere)
static const int DIFFUSE_CONE_COUNT = 16;
static const float DIFFUSE_CONE_APERTURE = 0.872665;

static const float3 DIFFUSE_CONE_DIRECTIONS[16] = {
	float3(0.57735, 0.57735, 0.57735),
	float3(0.57735, -0.57735, -0.57735),
	float3(-0.57735, 0.57735, -0.57735),
	float3(-0.57735, -0.57735, 0.57735),
	float3(-0.903007, -0.182696, -0.388844),
	float3(-0.903007, 0.182696, 0.388844),
	float3(0.903007, -0.182696, 0.388844),
	float3(0.903007, 0.182696, -0.388844),
	float3(-0.388844, -0.903007, -0.182696),
	float3(0.388844, -0.903007, 0.182696),
	float3(0.388844, 0.903007, -0.182696),
	float3(-0.388844, 0.903007, 0.182696),
	float3(-0.182696, -0.388844, -0.903007),
	float3(0.182696, 0.388844, -0.903007),
	float3(-0.182696, 0.388844, 0.903007),
	float3(0.182696, -0.388844, 0.903007)
};
#endif

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// coneDirection:	world-space cone direction in the direction to perform the trace
// coneAperture:	cone width
inline float4 ConeTrace(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 coneDirection, in float coneAperture)
{
	float3 color = 0;
	float alpha = 0;

	const float coneCoefficient = 2 * tan(coneAperture * 0.5);
	const float voxelSize = GetFrame().voxelradiance_size * 2; // full extent
	const float voxelSize_rcp = rcp(voxelSize);
	
	// We need to offset the cone start position to avoid sampling its own voxel (self-occlusion):
	//	Unfortunately, it will result in disconnection between nearby surfaces :(
	float dist = voxelSize; // offset by cone dir so that first sample of all cones are not the same
	float step_dist = dist;
	float3 startPos = P + N * voxelSize * VOXEL_INITIAL_OFFSET * SQRT2; // sqrt2 is diagonal voxel half-extent

	float3 aniso_direction = -coneDirection;
	float3 face_offsets = float3(
		aniso_direction.x > 0 ? 0 : 1,
		aniso_direction.y > 0 ? 2 : 3,
		aniso_direction.z > 0 ? 4 : 5
	) / 6.0;
	float3 direction_weights = abs(coneDirection);
	//float3 direction_weights = sqr(coneDirection);

	// We will break off the loop if the sampling distance is too far for performance reasons:
	while (dist < GetFrame().voxelradiance_max_distance && alpha < 1)
	{
		float diameter = max(voxelSize, coneCoefficient * dist);
		float mip = log2(diameter * voxelSize_rcp);

		// Because we do the ray-marching in world space, we need to remap into 3d texture space before sampling:
		float3 tc = startPos + coneDirection * dist;
		tc = (tc - GetFrame().voxelradiance_center) * GetFrame().voxelradiance_size_rcp;
		tc *= GetFrame().voxelradiance_resolution_rcp;
		tc = tc * float3(0.5f, -0.5f, 0.5f) + 0.5f;

		// break if the ray exits the voxel grid:
		if (!is_saturated(tc))
			break;

		tc.x /= 6.0;
		float4 sam =
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.x, tc.y, tc.z), mip) * direction_weights.x +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.y, tc.y, tc.z), mip) * direction_weights.y +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.z, tc.y, tc.z), mip) * direction_weights.z
			;

		// correction:
		float correction = step_dist * voxelSize_rcp;
		sam *= correction;

		// this is the correct blending to avoid black-staircase artifact (ray stepped front-to back, so blend front to back):
		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		// step along ray:
		step_dist = diameter * GetFrame().voxelradiance_stepsize;
		dist += step_dist;
	}

	return float4(color, alpha);
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
inline float4 ConeTraceDiffuse(in Texture3D<float4> voxels, in float3 P, in float3 N)
{
	float4 amount = 0;

#if 0
	float3x3 tangentSpace = get_tangentspace(N);
	const float aperture = tan(PI * 0.5 * 0.33);
	for (uint cone = 0; cone < GetFrame().voxelradiance_numcones; ++cone) // quality is between 1 and 16 cones
	{
		float2 hamm = hammersley2d(cone, GetFrame().voxelradiance_numcones);
		float3 hemisphere = hemispherepoint_cos(hamm.x, hamm.y);
		float3 coneDirection = mul(hemisphere, tangentSpace);

		amount += ConeTrace(voxels, P, N, coneDirection, aperture);
	}
	amount *= GetFrame().voxelradiance_numcones_rcp;
#else
	float sum = 0;
	for (uint i = 0; i < DIFFUSE_CONE_COUNT; ++i)
	{
		const float3 coneDirection = DIFFUSE_CONE_DIRECTIONS[i];
		const float cosTheta = dot(N, coneDirection);
		if (cosTheta <= 0)
			continue;
		amount += ConeTrace(voxels, P, N, coneDirection, DIFFUSE_CONE_APERTURE) * cosTheta;
		sum += cosTheta;
	}
	amount /= sum;
#endif 

	amount.rgb = max(0, amount.rgb);
	amount.a = saturate(amount.a);

	return amount;
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// V:				world-space view-vector (cameraPosition - P)
inline float4 ConeTraceSpecular(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 V, in float roughness)
{
	float aperture = roughness;
	float3 coneDirection = reflect(-V, N);

	float4 amount = ConeTrace(voxels, P, N, coneDirection, aperture);
	amount.rgb = max(0, amount.rgb);
	amount.a = saturate(amount.a);

	return amount;
}

#endif
