#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"
#include "ShaderInterop_SurfelGI.h"

// Edge-aware a-trous denoiser for the half-res surfel GI output.
//
// Smooths the per-surfel radiance variance that shows up as blotchiness on flat
// lit surfaces. Runs OUTPUT-side (this reads/writes the GI result texture,
// never the surfel cache), so it cannot feed back into light transport. The
// edge stops are normal + depth ONLY - deliberately NO luminance term, because
// the luminance variance IS the blotch we are removing; normal + depth alone
// keep geometry edges (corners, silhouettes, depth steps) crisp while a flat
// surface blends freely.
//
// Dispatched once per a-trous pass with a doubling tap stride (see
// SURFEL_DENOISE_PASSES); ping-ponged between two half-res scratch textures by
// the caller. The guides come from the globally-bound g-buffer sampled at the
// full-res pixel (half-res dispatch -> *2, matching surfel_coverageCS).

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<float3> input : register(t0);    // half-res GI to filter (ping-pong source)
RWTexture2D<float3> output : register(u0); // filtered half-res GI (ping-pong dest)

// B3-spline 5-tap a-trous kernel: {1/16, 1/4, 3/8, 1/4, 1/16}.
static const float atrous_kernel[5] = { 0.0625, 0.25, 0.375, 0.25, 0.0625 };

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const uint2 dim = postprocess.resolution;
	if (DTid.x >= dim.x || DTid.y >= dim.y)
		return;

	const uint step_size = (uint)postprocess.params1.y;

	// Full-res pixel for the guide g-buffer (half-res dispatch -> *2).
	const uint2 full = DTid.xy * 2;
	const float center_depth = texture_depth[full];
	if (center_depth == 0)
	{
		// Sky / background: nothing to filter, pass through unchanged.
		output[DTid.xy] = input[DTid.xy];
		return;
	}
	const float center_z = compute_lineardepth(center_depth);
	const float3 center_n = decode_normal(texture_normal_roughness[full].rg);

	float3 sum = 0;
	float weight_sum = 0;

	[unroll]
	for (int dy = -2; dy <= 2; ++dy)
	{
		[unroll]
		for (int dx = -2; dx <= 2; ++dx)
		{
			const int2 tap = int2(DTid.xy) + int2(dx, dy) * (int)step_size;
			if (tap.x < 0 || tap.y < 0 ||
				tap.x >= (int)dim.x || tap.y >= (int)dim.y)
				continue;

			const uint2 tap_full = (uint2)tap * 2;
			const float tap_depth = texture_depth[tap_full];
			if (tap_depth == 0)
				continue; // sky tap

			const float tap_z = compute_lineardepth(tap_depth);
			const float3 tap_n =
				decode_normal(texture_normal_roughness[tap_full].rg);

			// Normal + depth edge stops (no luminance term - see file header).
			const float w_normal = pow(
				saturate(dot(center_n, tap_n)), SURFEL_DENOISE_NORMAL_POWER);
			const float w_depth = exp(
				-abs(center_z - tap_z) /
				(SURFEL_DENOISE_DEPTH_SCALE * center_z + 1e-3));
			const float w =
				atrous_kernel[dx + 2] * atrous_kernel[dy + 2] *
				w_normal * w_depth;

			sum += input[(uint2)tap] * w;
			weight_sum += w;
		}
	}

	output[DTid.xy] = (weight_sum > 0) ? (sum / weight_sum) : input[DTid.xy];
}
