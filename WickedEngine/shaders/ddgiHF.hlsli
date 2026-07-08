#ifndef WI_DDGI_HF
#define WI_DDGI_HF
// DDGI GPU helper functions (probe addressing, irradiance sampling, variance
// estimator). These depend on GetScene()/ShaderScene, so they live in an *HF
// header included after ShaderInterop_Renderer.h - not in the shared-data
// header ShaderInterop_DDGI.h. See also ShaderInterop_DDGI.h for the data
// types.
#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

#ifndef __cplusplus
// All probe access is parameterized by a cascade index. Cascade 0 is the fine
// inner grid; higher cascades are coarser and cover more area. The cascades
// share the probe/variance/ray buffers - a cascade's probes occupy the range
// [probe_offset, probe_offset + probe_count) - and one depth atlas, where each
// cascade occupies a distinct region starting at depth_atlas_offset.
inline float3 ddgi_cellsize(uint cascade)
{
	return GetScene().ddgi.cascades[cascade].cell_size;
}
inline float3 ddgi_cellsize_rcp(uint cascade)
{
	return GetScene().ddgi.cascades[cascade].cell_size_rcp;
}
inline float ddgi_max_distance(uint cascade)
{
	return GetScene().ddgi.cascades[cascade].max_distance;
}
// Toroidal coordinate helpers.
//
// A "world coord" is the logical lattice position in [0, grid_dimensions-1]
// measured from grid_min; it maps directly to a world-space position and is
// what neighbour arithmetic must be done in. A "buffer coord" is where that
// probe's data physically lives in the probe/variance buffers and depth atlas.
// They differ by scroll_offset. Convert world->buffer exactly once, at the
// point of storage access.
//
// Signed modular arithmetic keeps this correct for any grid dimensions (not
// just powers of two) and any scroll sign.
//
// Convention (standard clipmap toroidal addressing): scroll_offset is the
// grid's world-origin in cell units (grid_min / cell_size). A world cell's
// absolute lattice index is (world_local + scroll_offset); its physical buffer
// slot is that index modulo the grid dimensions. Because the slot depends only
// on the absolute index, a fixed world cell keeps the same slot as the grid
// scrolls (only the recycled slab changes) - which is what preserves each
// probe's accumulated light in place. Hence world->buffer ADDS scroll_offset
// and buffer->world SUBTRACTS it (they must be inverses, and this particular
// sign is what makes a slot's world position invariant across scrolls; swapping
// them makes probes drift at twice the camera speed).
inline uint3 ddgi_wrap_coord(uint cascade, int3 coord)
{
	const int3 dims = int3(GetScene().ddgi.cascades[cascade].grid_dimensions);
	int3 wrapped = coord % dims;
	wrapped = (wrapped + dims) % dims; // bring negative remainders into [0, dims)
	return uint3(wrapped);
}
inline uint3 ddgi_buffer_to_world_coord(uint cascade, uint3 buffer_coord)
{
	return ddgi_wrap_coord(cascade, int3(buffer_coord) - GetScene().ddgi.cascades[cascade].scroll_offset);
}
inline uint3 ddgi_world_to_buffer_coord(uint cascade, uint3 world_coord)
{
	return ddgi_wrap_coord(cascade, int3(world_coord) + GetScene().ddgi.cascades[cascade].scroll_offset);
}
// Returns the WORLD coord of the probe cell containing point P (not a buffer
// coord).
inline uint3 ddgi_base_probe_coord(uint cascade, float3 P)
{
	float3 normalized_pos = (P - GetScene().ddgi.cascades[cascade].grid_min) * GetScene().ddgi.cascades[cascade].grid_extents_rcp;
	return uint3(clamp(
		floor(normalized_pos * float3(GetScene().ddgi.cascades[cascade].grid_dimensions - 1)),
		0.0, float3(GetScene().ddgi.cascades[cascade].grid_dimensions - 1)));
}
// Local coord (within a cascade) from a local probe index (0..probe_count-1).
inline uint3 ddgi_probe_coord(uint cascade, uint localIndex)
{
	return unflatten3D(localIndex, GetScene().ddgi.cascades[cascade].grid_dimensions);
}
// Global buffer index of a probe from its cascade + local buffer coord.
inline uint ddgi_probe_index(uint cascade, min16uint3 probeCoord)
{
	return GetScene().ddgi.cascades[cascade].probe_offset + flatten3D(probeCoord, GetScene().ddgi.cascades[cascade].grid_dimensions);
}
// Decodes a global probe index into its cascade and local buffer coord.
inline void ddgi_decode_probe(uint globalIndex, out uint cascade, out uint3 probeCoord)
{
	cascade = 0;
	[unroll]
	for (uint c = 1; c < DDGI_CASCADE_COUNT; ++c)
	{
		if (globalIndex >= GetScene().ddgi.cascades[c].probe_offset)
			cascade = c;
	}
	const uint localIndex = globalIndex - GetScene().ddgi.cascades[cascade].probe_offset;
	probeCoord = ddgi_probe_coord(cascade, localIndex);
}
// Rest (unrelocated) world position of a probe, given its cascade + BUFFER
// coord.
inline float3 ddgi_probe_position_rest(uint cascade, min16uint3 probeCoord)
{
	uint3 world_coord = ddgi_buffer_to_world_coord(cascade, uint3(probeCoord));
	return GetScene().ddgi.cascades[cascade].grid_min + float3(world_coord) * ddgi_cellsize(cascade);
}
inline float3 ddgi_probe_position(uint cascade, min16uint3 probeCoord)
{
	float3 pos = ddgi_probe_position_rest(cascade, probeCoord);
	uint probeIndex = ddgi_probe_index(cascade, probeCoord);
	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	DDGIProbe probe = probe_buffer[probeIndex];
	float3 offset = unpack_half3(probe.offset);
	offset = offset * ddgi_cellsize(cascade) * 0.5;
	pos += offset;
	return pos;
}
inline uint2 ddgi_probe_depth_pixel(uint cascade, min16uint3 probeCoord)
{
	return GetScene().ddgi.cascades[cascade].depth_atlas_offset
		+ probeCoord.xz * DDGI_DEPTH_TEXELS
		+ uint2(probeCoord.y * GetScene().ddgi.cascades[cascade].grid_dimensions.x * DDGI_DEPTH_TEXELS, 0) + 1;
}
inline float2 ddgi_probe_depth_uv(uint cascade, min16uint3 probeCoord, half3 direction)
{
	float2 pixel = ddgi_probe_depth_pixel(cascade, probeCoord);
	pixel += (encode_oct(normalize(direction)) * 0.5 + 0.5) * DDGI_DEPTH_RESOLUTION;
	return pixel * GetScene().ddgi.depth_texture_resolution_rcp;
}


// Gathers the 8-probe interpolated radiance SH for one cascade at point P.
//
// Writes the normalized (weight-averaged) SH for that cascade to out_sh and the
// total gather weight to out_weight (0 if the cascade contributed nothing).
// Neighbour arithmetic stays in world space; conversion to a buffer coord
// happens only at the point of buffer/atlas access.
//
// `enabled` gates the (expensive) 8-probe gather: when false the per-probe loop
// runs zero iterations, so no probe-buffer reads or depth-atlas samples happen
// and the result is Zero. This must be expressed as a data-dependent loop
// bound, NOT as a branch/early-return: the shader compiler (DXC) crashes during
// optimization on ANY conditional control flow tied to an `out` parameter of
// the SH type (a conditional call, or an early return). Here the out params are
// written unconditionally once at the end, and only the loop trip count depends
// on `enabled`, which compiles. (For the same reason the SH is returned via an
// out parameter, not by value - returning it by value from a function called
// more than once also crashes DXC.) A wave sitting entirely inside one cascade,
// where the coarse gather is disabled, still pays nothing.
//
// Based on:
// https://github.com/diharaw/hybrid-rendering/blob/master/src/shaders/gi/gi_common.glsl
void ddgi_gather_sh(uint cascade, in float3 P, in half3 N, bool enabled, out SH::L1_RGB out_sh, out half out_weight)
{
	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	const min16uint3 base_world_coord = ddgi_base_probe_coord(cascade, P);
	const float3 reference_probe_pos = GetScene().ddgi.cascades[cascade].grid_min + float3(base_world_coord) * ddgi_cellsize(cascade); // rest pose of the base world probe

	half sum_weight = 0;

	// Note: the quality seems to be a lot better when weighting the whole SH
	//  instead of blending irradiance, specular and dld separately
	SH::L1_RGB sum_sh = SH::L1_RGB::Zero();

	// alpha is how far from the floor(currentVertex) position. on [0, 1] for
	// each axis.
	half3 alpha = saturate((P - reference_probe_pos) * ddgi_cellsize_rcp(cascade));

	// Iterate over adjacent probe cage (zero iterations when this gather is
	// disabled - see the note above on why this is a loop bound, not a branch).
	const uint cage_count = enabled ? 8u : 0u;
	[loop]
	for (min16uint i = 0; i < cage_count; ++i)
	{
		// Compute the neighbour in WORLD space and clamp to the real grid
		// boundary. Offset = 0 or 1 along each axis. Clamping here (in world
		// space) keeps the cage from wrapping across the toroidal seam or the
		// grid edge.
		min16uint3 offset = uint3(i, i >> 1, i >> 2) & 1;
		min16uint3 probe_world_coord = clamp(base_world_coord + offset, 0u.xxx, GetScene().ddgi.cascades[cascade].grid_dimensions - 1);
		// Convert to a buffer coord only now, to index the probe buffer and
		// depth atlas.
		min16uint3 probe_buffer_coord = ddgi_world_to_buffer_coord(cascade, probe_world_coord);
		uint probe_index = ddgi_probe_index(cascade, probe_buffer_coord);
		DDGIProbe probe = probe_buffer[probe_index];

		// Make cosine falloff in tangent plane with respect to the angle from
		// the surface to the probe so that we never test a probe that is
		// *behind* the surface. It doesn't have to be cosine, but that is
		// efficient to compute and we must clip to the tangent plane.
		float3 probe_pos = ddgi_probe_position(cascade, probe_buffer_coord);

		// Bias the position at which visibility is computed; this
		// avoids performing a shadow test *at* a surface, which is a
		// dangerous location because that is exactly the line between
		// shadowed and unshadowed. If the normal bias is too small,
		// there will be light and dark leaks. If it is too large,
		// then samples can pass through thin occluders to the other
		// side (this can only happen if there are MULTIPLE occluders
		// near each other, a wall surface won't pass through itself.)
		half3 probe_to_point = P - probe_pos + N * 0.001;
		half3 dir = normalize(-probe_to_point);

		// Compute the trilinear weights based on the grid cell vertex to
		// smoothly transition between probes. Avoid ever going entirely to zero
		// because that will cause problems at the border probes. This isn't
		// really a lerp. We're using 1-a when offset = 0 and a when offset = 1.
		half3 trilinear = lerp(1.0 - alpha, alpha, half3(offset));
		half weight = 1.0;

		// Trust a probe at full weight only when it is valid AND has its own
		// converged radiance. A buried/invalid probe is down-weighted as
		// before. A probe that was just ejected from geometry (COLOR_RESET
		// still pending) is valid, but its radiance is only the provisional
		// colour borrowed from a neighbour - systematically too bright, because
		// that neighbour sits in more open space than the freshly-revealed
		// (often shadowed) spot the probe now occupies. Showing it at full
		// weight over-brightens the revealed surface while an object moves (and
		// multi-bounce feedback compounds it). Keep it down-weighted until its
		// own rays land next update, so the surrounding converged probes light
		// the surface; then it joins at full weight already correct.
		bool probe_trusted = (probe.flags & DDGIPROBE_FLAG_VALID) != 0
			&& (probe.flags & DDGIPROBE_FLAG_COLOR_RESET) == 0;
		weight *= probe_trusted ? 1.0 : 0.02;

		// Clamp all of the multiplies. We can't let the weight go to zero
		// because then it would be possible for *all* weights to be equally low
		// and get normalized up to 1/n. We want to distinguish between weights
		// that are low because of different factors.

		// Smooth backface test
		{
			// Computed without the biasing applied to the "dir" variable. This
			// test can cause reflection-map looking errors in the image (stuff
			// looks shiny) if the transition is poor.
			half3 true_direction_to_probe = normalize(probe_pos - P);

			// The naive soft backface weight would ignore a probe when it is
			// behind the surface. That's good for walls. But for small details
			// inside of a room, the normals on the details might rule out all
			// of the probes that have mutual visibility to the point. So, we
			// instead use a "wrap shading" test below inspired by NPR work.
			// weight *= max(0.0001, dot(trueDirectionToProbe, wsN));

			// The small offset at the end reduces the "going to zero" impact
			// where this is really close to exactly opposite
			weight *= lerp(saturate(dot(dir, N)), sqr(max(0.0001, (dot(true_direction_to_probe, N) + 1.0) * 0.5)) + 0.2, (half)GetScene().ddgi.smooth_backface);
		}

		// Moment visibility test
#if 1
		[branch]
		if(GetScene().ddgi.depth_texture >= 0)
		{
			//float2 tex_coord = texture_coord_from_direction(-dir, p, ddgi.depth_texture_width, ddgi.depth_texture_height, ddgi.depth_probe_side_length);
			float2 tex_coord = ddgi_probe_depth_uv(cascade, probe_buffer_coord, -dir);

			half dist_to_probe = length(probe_to_point);

			//float2 temp = textureLod(depth_texture, tex_coord, 0.0f).rg;
			half2 temp = bindless_textures_half4[descriptor_index(GetScene().ddgi.depth_texture)].SampleLevel(sampler_linear_clamp, tex_coord, 0).xy;
			half mean = temp.x;
			half variance = abs(sqr(temp.x) - temp.y);

			// http://www.punkuser.net/vsm/vsm_paper.pdf; equation 5 Need the
			// max in the denominator because biasing can cause a negative
			// displacement
			half chebyshev_weight = variance / (variance + sqr(max(dist_to_probe - mean, 0.0)));

			// Increase contrast in the weight
			chebyshev_weight = max(pow(chebyshev_weight, 3), 0.0);

			weight *= (dist_to_probe <= mean) ? 1.0 : chebyshev_weight;
		}
#endif

		// Avoid zero weight
		weight = max(0.01, weight);

		// A tiny bit of light is really visible due to log perception, so
		// crush tiny weights but keep the curve continuous. This must be done
		// before the trilinear weights, because those should be preserved.
		const half crush_threshold = 0.2;
		if (weight < crush_threshold)
			weight *= weight * weight / sqr(crush_threshold);

		// Trilinear weights
		weight *= trilinear.x * trilinear.y * trilinear.z;

		sum_sh = SH::Add(sum_sh, SH::Multiply(probe.radiance.Unpack(), weight));

		sum_weight += weight;
	}

	out_weight = sum_weight;
	if (sum_weight > 0)
		sum_sh = SH::Multiply(sum_sh, rcp(sum_weight));
	out_sh = sum_sh;
}

// Returns true and the [0,1] per-axis fractional position of P within a cascade
// grid when P lies inside that cascade's bounds; false otherwise.
inline bool ddgi_cascade_fraction(uint cascade, float3 P, out float3 frac)
{
	frac = (P - GetScene().ddgi.cascades[cascade].grid_min) * GetScene().ddgi.cascades[cascade].grid_extents_rcp;
	return all(frac > 0.0) && all(frac < 1.0);
}

// Samples DDGI irradiance at P, blending across cascades: the finest cascade
// that contains P is used, fading into the next coarser cascade near its outer
// edge so the density transition is seamless.
half3 ddgi_sample_irradiance(in float3 P, in half3 N, inout half3 out_dominant_lightdir, inout half3 out_dominant_lightcolor)
{
	// Pick the finest cascade containing P and a blend factor toward the next
	// coarser cascade near its boundary. A fragment outside every fine cascade
	// falls through to the coarsest one. Written as an ascending loop that
	// stops at the first (finest) containing cascade - a descending unrolled
	// loop crashes the shader compiler.
	uint fine_cascade = DDGI_CASCADE_COUNT - 1; // default: coarsest
	half blend_to_coarser = 0;
	bool found = false;
	[unroll]
	for (uint c = 0; c < DDGI_CASCADE_COUNT; ++c)
	{
		float3 frac;
		if (!found && c < DDGI_CASCADE_COUNT - 1 && ddgi_cascade_fraction(c, P, frac))
		{
			found = true;
			fine_cascade = c;
			// distance from cascade center in [0,1] (0 center, 1 at the edge)
			const half3 d = abs((half3)frac - 0.5) * 2.0;
			const half t = max(d.x, max(d.y, d.z));
			// fade into the coarser cascade over the outer 15% of the volume
			blend_to_coarser = smoothstep(0.85, 1.0, t);
		}
	}

	const uint coarse_cascade = min(fine_cascade + 1, DDGI_CASCADE_COUNT - 1);

	// The fine cascade is always gathered. The coarse cascade is only gathered
	// when it will actually contribute (near the fine cascade's outer edge, and
	// only if it is a distinct cascade). Both calls are unconditional - the
	// `enabled` flag gates the work inside ddgi_gather_sh - so a wave sitting
	// deep inside a cascade (the common case) pays for a single gather.
	half w_fine = 0;
	SH::L1_RGB sh_fine;
	ddgi_gather_sh(fine_cascade, P, N, true, sh_fine, w_fine);

	const bool need_coarse = (blend_to_coarser > 0) && (coarse_cascade != fine_cascade);
	half w_coarse = 0;
	SH::L1_RGB sh_coarse;
	ddgi_gather_sh(coarse_cascade, P, N, need_coarse, sh_coarse, w_coarse);

	// Combine the two normalized cascade SHs by the blend factor, ignoring a
	// cascade that contributed no weight (P outside its bounds, or the coarse
	// gather was disabled).
	const half a_fine = (w_fine > 0) ? (1.0 - blend_to_coarser) : 0.0;
	const half a_coarse = (w_coarse > 0) ? blend_to_coarser : 0.0;
	const half a_sum = a_fine + a_coarse;

	if (a_sum > 0)
	{
		SH::L1_RGB sum_sh = SH::Add(SH::Multiply(sh_fine, a_fine), SH::Multiply(sh_coarse, a_coarse));
		sum_sh = SH::Multiply(sum_sh, rcp(a_sum));

		// Evaluate the diffuse irradiance in full (float) precision.
		//
		// The SH helpers run in half precision, and CalculateIrradiance()
		// convolves the radiance with a cosine lobe - a multiply by PI on the
		// L0 band. For a bright probe near a light source the L0 coefficient is
		// already large, so that multiply overflows half to +Inf. That Inf is
		// then fed back into the probes by the raytrace pass, where the mean
		// estimator cannot recover from it, and it spreads across the whole
		// probe field, blowing the scene out to white. Doing the
		// convolve+evaluate in float keeps the full dynamic range without
		// overflowing (so the GI brightness does not have to be clamped, which
		// would flatten it).
		//
		// This is exactly SH::CalculateIrradiance(sum_sh, N) / PI, inlined in
		// float: convolve with the cosine lobe (CosineA0/CosineA1) then
		// evaluate in direction N (BasisL0/BasisL1).
		half3 net_irradiance;
		{
			const float cosine_a0 = PI;               // CosineA0
			const float cosine_a1 = (2.0 * PI) / 3.0; // CosineA1
			const float basis_l0 = 1.0 / (2.0 * sqrt(PI));
			const float basis_l1 = sqrt(3.0) / (2.0 * sqrt(PI));
			const float3 nrm = (float3)N;
			const float3 irradiance =
				basis_l0 * cosine_a0 * (float3)sum_sh.C[0] +
				basis_l1 * cosine_a1 * nrm.y * (float3)sum_sh.C[1] +
				basis_l1 * cosine_a1 * nrm.z * (float3)sum_sh.C[2] +
				basis_l1 * cosine_a1 * nrm.x * (float3)sum_sh.C[3];
			net_irradiance = (half3)clamp(irradiance / PI, 0.0, MEDIUMP_FLT_MAX);
		}

		// The dominant-light extraction still uses the half-precision SH path.
		// Clamp the averaged SH so its dot products stay finite for very bright
		// probes; this only affects the directional specular approximation, not
		// the diffuse irradiance computed above. 40000 keeps DotProduct finite.
		[unroll]
		for (uint sh_coeff = 0; sh_coeff < SH::L1_RGB::NumCoefficients; ++sh_coeff)
			sum_sh.C[sh_coeff] = clamp(sum_sh.C[sh_coeff], -40000.0, 40000.0);

		SH::ApproximateDirectionalLight(sum_sh, out_dominant_lightdir, out_dominant_lightcolor);

		// bending with normal direction to push dld above surface level since light can't fall to surface from behind
		out_dominant_lightdir = normalize(out_dominant_lightdir + N * 0.8);

		return net_irradiance;
	}

	return 0;
}

static const uint4 DDGI_DEPTH_BORDER_OFFSETS[68] = {
	uint4(16, 1, 1, 0),
	uint4(15, 1, 2, 0),
	uint4(14, 1, 3, 0),
	uint4(13, 1, 4, 0),
	uint4(12, 1, 5, 0),
	uint4(11, 1, 6, 0),
	uint4(10, 1, 7, 0),
	uint4(9, 1, 8, 0),
	uint4(8, 1, 9, 0),
	uint4(7, 1, 10, 0),
	uint4(6, 1, 11, 0),
	uint4(5, 1, 12, 0),
	uint4(4, 1, 13, 0),
	uint4(3, 1, 14, 0),
	uint4(2, 1, 15, 0),
	uint4(1, 1, 16, 0),
	uint4(16, 16, 1, 17),
	uint4(15, 16, 2, 17),
	uint4(14, 16, 3, 17),
	uint4(13, 16, 4, 17),
	uint4(12, 16, 5, 17),
	uint4(11, 16, 6, 17),
	uint4(10, 16, 7, 17),
	uint4(9, 16, 8, 17),
	uint4(8, 16, 9, 17),
	uint4(7, 16, 10, 17),
	uint4(6, 16, 11, 17),
	uint4(5, 16, 12, 17),
	uint4(4, 16, 13, 17),
	uint4(3, 16, 14, 17),
	uint4(2, 16, 15, 17),
	uint4(1, 16, 16, 17),
	uint4(1, 16, 0, 1),
	uint4(1, 15, 0, 2),
	uint4(1, 14, 0, 3),
	uint4(1, 13, 0, 4),
	uint4(1, 12, 0, 5),
	uint4(1, 11, 0, 6),
	uint4(1, 10, 0, 7),
	uint4(1, 9, 0, 8),
	uint4(1, 8, 0, 9),
	uint4(1, 7, 0, 10),
	uint4(1, 6, 0, 11),
	uint4(1, 5, 0, 12),
	uint4(1, 4, 0, 13),
	uint4(1, 3, 0, 14),
	uint4(1, 2, 0, 15),
	uint4(1, 1, 0, 16),
	uint4(16, 16, 17, 1),
	uint4(16, 15, 17, 2),
	uint4(16, 14, 17, 3),
	uint4(16, 13, 17, 4),
	uint4(16, 12, 17, 5),
	uint4(16, 11, 17, 6),
	uint4(16, 10, 17, 7),
	uint4(16, 9, 17, 8),
	uint4(16, 8, 17, 9),
	uint4(16, 7, 17, 10),
	uint4(16, 6, 17, 11),
	uint4(16, 5, 17, 12),
	uint4(16, 4, 17, 13),
	uint4(16, 3, 17, 14),
	uint4(16, 2, 17, 15),
	uint4(16, 1, 17, 16),
	uint4(16, 16, 0, 0),
	uint4(1, 16, 17, 0),
	uint4(16, 1, 0, 17),
	uint4(1, 1, 17, 17)
};

void MultiscaleMeanEstimator(
	half3 y,
	inout DDGIVarianceData data,
	half shortWindowBlend = 0.08f
)
{
	half3 mean = data.mean;
	half3 shortMean = data.shortMean;
	half vbbr = data.vbbr;
	half3 variance = data.variance;
	half inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		half3 dev = sqrt(max(1e-5, variance));
		half3 highThreshold = 0.1 + shortMean + dev * 8;
		half3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	half3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	half3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	half varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	half3 dev = sqrt(max(1e-5, variance));

	half3 shortDiff = mean - shortMean;

	half relativeDiff = dot(half3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	half varianceBasedBlendReduction =
		clamp(dot(half3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	half3 catchUpBlend = clamp(smoothstep(0, 1,
		relativeDiff * max(0.02, inconsistency - 0.2)), 1.0 / 256, 1);
	catchUpBlend *= vbbr;

	vbbr = lerp(vbbr, varianceBasedBlendReduction, 0.1);
	mean = lerp(mean, y, saturate(catchUpBlend));

	// Output
	data.mean = mean;
	data.shortMean = shortMean;
	data.vbbr = vbbr;
	data.variance = variance;
	data.inconsistency = inconsistency;
}

#endif // __cplusplus

#endif // WI_DDGI_HF
