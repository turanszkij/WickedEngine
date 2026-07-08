#ifndef WI_VOXEL_CONERACING_HF
#define WI_VOXEL_CONERACING_HF
#include "globals.hlsli"

// With help from: https://github.com/compix/VoxelConeTracingGI/blob/master/assets/shaders/voxelConeTracing/finalLightingPass.frag

static const float VXGI_CLIPMAP_COUNT_RCP = rcp((float)VXGI_CLIPMAP_COUNT);

// Brightness correction for the diffuse cone trace.
//
// The step-size scaling in SampleVoxelClipMap (sam *= step_dist / voxelSize)
// multiplies the sampled radiance by a factor that is >= 2 for the diffuse
// trace (stepSize == 1 and the cone diameter starts at 2 * voxelSize). That
// made VXGI diffuse GI roughly 2x brighter than DDGI / Surfel.
//
// This is applied to the FINAL ConeTraceDiffuse result rather than inside the
// per-sample compositing on purpose: ConeTraceDiffuse also drives the recursive
// voxel radiance feedback (vxgi_temporalCS), and altering the compositing there
// destabilises that loop (runaway accumulation to white). Scaling the final
// result can only reduce the feedback-loop gain, so it cannot destabilise a
// loop that was already stable at full strength.
static const half VXGI_DIFFUSE_INTENSITY = 0.5;

// Brightness correction for the specular cone trace.
//
// ConeTraceSpecular traces with stepSize == vxgi.stepsize (default 1), so it is
// subject to the same >= 2x step-size over-brightness as the diffuse trace (see
// VXGI_DIFFUSE_INTENSITY). It is applied to the final ConeTraceSpecular result
// for the same reason and keeps reflections consistent with the diffuse GI.
// Unlike the diffuse trace, the specular trace does not feed the voxel radiance
// back into itself, so this is display-only.
static const half VXGI_SPECULAR_INTENSITY = 0.5;

// Roughness above which the specular cone trace is skipped entirely.
//
// At high roughness the reflection cone is very wide, so its contribution is
// low frequency and small in magnitude - it is already well approximated by the
// diffuse GI and by environment probe reflections. Skipping the per-pixel cone
// march there avoids wasted work for a negligible visual difference.
static const half VXGI_SPECULAR_MAX_ROUGHNESS = 0.8;

inline half4 SampleVoxelClipMap(in Texture3D<half4> voxels, in float3 P, in uint clipmap_index, float step_dist, in float3 face_offsets, in float3 direction_weights, uint precomputed_direction = 0)
{
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[clipmap_index];
	float3 tc = GetFrame().vxgi.world_to_clipmap(P, clipmap);

	// half texel correction is applied to avoid sampling over current clipmap:
	const float half_texel = 0.5 * GetFrame().vxgi.resolution_rcp;
	tc = clamp(tc, half_texel, 1 - half_texel);

	tc.x = (tc.x + precomputed_direction) / (6.0 + DIFFUSE_CONE_COUNT); // remap into anisotropic
	tc.y = (tc.y + clipmap_index) * VXGI_CLIPMAP_COUNT_RCP; // remap into clipmap

	half4 sam;
	if (precomputed_direction == 0)
	{
		// Sample anisotropically 3 times, weighted by cone direction, then
		// normalize by the weight sum. direction_weights = abs(coneDirection),
		// so the weights sum to between 1 (axis-aligned) and sqrt(3) ~ 1.73
		// (diagonal). Dividing by that sum turns this into a proper directional
		// average, instead of letting diagonal cone directions pick up ~1.73x
		// more radiance than axis-aligned ones. The sum is always >= 1 for a
		// normalized direction, so the division is safe.
		//
		// Note: only the non-precomputed path (specular reflections) reaches
		// this. The diffuse path reads precomputed, already-weighted slices,
		// and it also feeds the recursive voxel radiance loop, so it is
		// intentionally left as is here.
		const float weight_sum =
			direction_weights.x + direction_weights.y + direction_weights.z;
		sam = (
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.x, tc.y, tc.z), 0) * direction_weights.x +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.y, tc.y, tc.z), 0) * direction_weights.y +
			voxels.SampleLevel(sampler_linear_clamp, float3(tc.x + face_offsets.z, tc.y, tc.z), 0) * direction_weights.z
			) / weight_sum;
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
inline half4 ConeTrace(in Texture3D<half4> voxels, in float3 P, in float3 N, in float3 coneDirection, in float coneAperture, in float stepSize, bool use_sdf = false, uint precomputed_direction = 0)
{
	half3 color = 0;
	half alpha = 0;

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
		half4 sam = SampleVoxelClipMap(voxels, p0, clipmap_index, step_dist, face_offsets, direction_weights, precomputed_direction);

		// sample second clipmap if needed and perform trilinear blend:
		if (clipmap_blend > 0 && clipmap_index < VXGI_CLIPMAP_COUNT - 1)
		{
			sam = lerp(sam, SampleVoxelClipMap(voxels, p0, clipmap_index + 1, step_dist, face_offsets, direction_weights, precomputed_direction), clipmap_blend);
		}

		// front-to back blending:
		half a = 1 - alpha;
		color += a * sam.rgb;
		alpha += a * sam.a;

		float stepSizeCurrent = stepSize;
		[branch]
		if (use_sdf)
		{
			// half texel correction is applied to avoid sampling over current clipmap:
			const float half_texel = 0.5 * GetFrame().vxgi.resolution_rcp;
			float3 tc0 = clamp(tc, half_texel, 1 - half_texel);
			tc0.y = (tc0.y + clipmap_index) * VXGI_CLIPMAP_COUNT_RCP; // remap into clipmap
			half sdf = bindless_textures3D_half4[descriptor_index(GetFrame().vxgi.texture_sdf)].SampleLevel(sampler_linear_clamp, tc0, 0).r;
			stepSizeCurrent = max(stepSize, sdf - diameter);
		}
		step_dist = diameter * stepSizeCurrent;

		// step along ray:
		dist += step_dist;
	}

	return half4(color, alpha);
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
inline half4 ConeTraceDiffuse(in Texture3D<half4> voxels, in float3 P, in float3 N)
{
	half4 amount = 0;

	half sum = 0;
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

	amount.rgb = max(0, amount.rgb) * VXGI_DIFFUSE_INTENSITY;
	amount.a = saturate(amount.a);

	return amount;
}

// voxels:			3D Texture containing voxel scene with direct diffuse lighting (or direct + secondary indirect bounce)
// P:				world-space position of receiving surface
// N:				world-space normal vector of receiving surface
// V:				world-space view-vector (cameraPosition - P)
inline half4 ConeTraceSpecular(in Texture3D<half4> voxels, in float3 P, in float3 N, in float3 V, in half roughness, in uint2 pixel)
{
	half aperture = roughness;
	float3 coneDirection = reflect(-V, N);

	// some dithering to help with banding at large step size
	P += coneDirection * (dither(pixel + GetTemporalAASampleRotation()) - 0.5) * GetFrame().vxgi.stepsize;

	half4 amount = ConeTrace(voxels, P, N, coneDirection, aperture, GetFrame().vxgi.stepsize, true);
	amount.rgb = max(0, amount.rgb) * VXGI_SPECULAR_INTENSITY;
	amount.a = saturate(amount.a);

	return amount;
}

#endif
