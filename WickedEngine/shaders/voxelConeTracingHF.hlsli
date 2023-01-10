#ifndef WI_VOXEL_CONERACING_HF
#define WI_VOXEL_CONERACING_HF
#include "globals.hlsli"

// With help from: https://github.com/compix/VoxelConeTracingGI/blob/master/assets/shaders/voxelConeTracing/finalLightingPass.frag

inline float4 SampleVoxelClipMap(in Texture3D<float4> voxels, in float3 P, in uint clipmap_index, float step_dist, in float3 face_offsets, in float3 direction_weights, uint precomputed_direction = 0)
{
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];
	float3 tc = GetFrame().vxgi.world_to_clipmap(P, clipmap);

	// half texel correction is applied to avoid sampling over current clipmap:
	const float half_texel = 0.5 * GetFrame().vxgi.resolution_rcp;
	tc = clamp(tc, half_texel, 1 - half_texel);

	tc.x = (tc.x + precomputed_direction) / (6.0 + DIFFUSE_CONE_COUNT); // remap into anisotropic
	tc.y = (tc.y + clipmap_index) / VXGI_CLIPMAP_COUNT; // remap into clipmap

	float4 sam;
	if (precomputed_direction == 0)
	{
		// sample anisotropically 3 times, weighted by cone direction:
		sam =
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.x, tc.y, tc.z), 0) * direction_weights.x +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.y, tc.y, tc.z), 0) * direction_weights.y +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.z, tc.y, tc.z), 0) * direction_weights.z
			;
	}
	else
	{
		// sample once for precomputed anisotropically weighted cone direction (uses precomputed_direction):
		sam = voxels.SampleLevel(sampler_linear_clamp, tc, 0);
	}

	// correction:
	sam *= step_dist / clipmap.voxelSize;

	return sam;
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// coneDirection:	world-space cone direction in the direction to perform the trace
// coneAperture:	cone width
// precomputed_direction : avoid 3x anisotropic weight sampling, and instead directly use a slice that has precomputed cone direction weighted data
inline float4 ConeTrace(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 coneDirection, in float coneAperture, in float stepSize, bool use_sdf = false, uint precomputed_direction = 0)
{
	float3 color = 0;
	float alpha = 0;

	uint clipmap_index0 = 0;
	VoxelClipMap clipmap0 = GetFrame().vxgi.clipmaps[clipmap_index0];
	const float voxelSize0 = clipmap0.voxelSize * 2; // full extent
	const float voxelSize0_rcp = rcp(voxelSize0);

	const float coneCoefficient = 2 * tan(coneAperture * 0.5);
	
	// We need to offset the cone start position to avoid sampling its own voxel (self-occlusion):
	float dist = voxelSize0; // offset by cone dir so that first sample of all cones are not the same
	float step_dist = dist;
	float3 startPos = P + N * voxelSize0;

	float3 aniso_direction = -coneDirection;
	float3 face_offsets = float3(
		aniso_direction.x > 0 ? 0 : 1,
		aniso_direction.y > 0 ? 2 : 3,
		aniso_direction.z > 0 ? 4 : 5
	) / (6.0 + DIFFUSE_CONE_COUNT);
	float3 direction_weights = abs(coneDirection);
	//float3 direction_weights = sqr(coneDirection);

	// We will break off the loop if the sampling distance is too far for performance reasons:
	while (dist < GetFrame().vxgi.max_distance && alpha < 1 && clipmap_index0 < VXGI_CLIPMAP_COUNT)
	{
		float3 p0 = startPos + coneDirection * dist;

		float diameter = max(voxelSize0, coneCoefficient * dist);
		float lod = clamp(log2(diameter * voxelSize0_rcp), clipmap_index0, VXGI_CLIPMAP_COUNT - 1);

		float clipmap_index = floor(lod);
		float clipmap_blend = frac(lod);

		VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];
		float3 tc = GetFrame().vxgi.world_to_clipmap(p0, clipmap);
		if (!is_saturated(tc))
		{
			clipmap_index0++;
			continue;
		}

		// sample first clipmap level:
		float4 sam = SampleVoxelClipMap(voxels, p0, clipmap_index, step_dist, face_offsets, direction_weights, precomputed_direction);

		// sample second clipmap if needed and perform trilinear blend:
		if(clipmap_blend > 0 && clipmap_index < VXGI_CLIPMAP_COUNT - 1)
		{
			sam = lerp(sam, SampleVoxelClipMap(voxels, p0, clipmap_index + 1, step_dist, face_offsets, direction_weights, precomputed_direction), clipmap_blend);
		}

		// front-to back blending:
		float a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		float stepSizeCurrent = stepSize;
		if (use_sdf)
		{
			// half texel correction is applied to avoid sampling over current clipmap:
			const float half_texel = 0.5 * GetFrame().vxgi.resolution_rcp;
			float3 tc0 = clamp(tc, half_texel, 1 - half_texel);
			tc0.y = (tc0.y + clipmap_index) / VXGI_CLIPMAP_COUNT; // remap into clipmap
			float sdf = bindless_textures3D[GetFrame().vxgi.texture_sdf].SampleLevel(sampler_linear_clamp, tc0, 0).r;
			stepSizeCurrent = max(stepSize, sdf - diameter);
		}
		step_dist = diameter * stepSizeCurrent;

		// step along ray:
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

	float sum = 0;
	for (uint i = 0; i < DIFFUSE_CONE_COUNT; ++i)
	{
		const float3 coneDirection = DIFFUSE_CONE_DIRECTIONS[i];
		const float cosTheta = dot(N, coneDirection);
		if (cosTheta <= 0)
			continue;
		const uint precomputed_direction = 6 + i; // optimization, avoids sampling 3 times aniso weights
		amount += ConeTrace(voxels, P, N, coneDirection, DIFFUSE_CONE_APERTURE, 1, false, precomputed_direction) * cosTheta;
		sum += cosTheta;
	}
	amount /= sum;

	amount.rgb = max(0, amount.rgb);
	amount.a = saturate(amount.a);

	return amount;
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// V:				world-space view-vector (cameraPosition - P)
inline float4 ConeTraceSpecular(in Texture3D<float4> voxels, in float3 P, in float3 N, in float3 V, in float roughness, in uint2 pixel)
{
	float aperture = roughness;
	float3 coneDirection = reflect(-V, N);

	// some dithering to help with banding at large step size
	P += coneDirection * (dither(pixel + GetTemporalAASampleRotation()) - 0.5) * GetFrame().vxgi.stepsize;

	float4 amount = ConeTrace(voxels, P, N, coneDirection, aperture, GetFrame().vxgi.stepsize, true);
	amount.rgb = max(0, amount.rgb);
	amount.a = saturate(amount.a);

	return amount;
}

#endif
