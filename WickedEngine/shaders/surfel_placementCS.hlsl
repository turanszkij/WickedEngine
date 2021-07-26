#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

static const uint SURFEL_TARGET_COVERAGE = 1;

TEXTURE2D(coverage, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWRAWBUFFER(surfelStatsBuffer, 1);
RWSTRUCTUREDBUFFER(surfelIndexBuffer, uint, 2);
RWSTRUCTUREDBUFFER(surfelCellIndexBuffer, float, 3); // sorting written for floats

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_coverage = coverage[DTid.xy];

	// Early exit: coverage not applicable (eg. sky)
	if (surfel_coverage == ~0)
		return;

	uint2 pixel = DTid.xy * 16;
	pixel.x += (surfel_coverage >> 4) & 0xF;
	pixel.y += (surfel_coverage >> 0) & 0xF;

	uint coverage_amount = surfel_coverage >> 8;

	const float depth = texture_depth[pixel.xy];

	// Early exit: slow down the propagation by chance
	//	Closer surfaces have less chance to avoid excessive clumping of surfels
	const float lineardepth = getLinearDepth(depth) * g_xCamera_ZFarP_rcp;
	const float chance = pow(1 - lineardepth, 8);
	if (blue_noise(DTid.xy).x < chance)
		return;

	const float4 g1 = texture_gbuffer1[pixel];

	if (depth > 0 && any(g1) && coverage_amount < SURFEL_TARGET_COVERAGE)
	{
		const float2 uv = ((float2)pixel.xy + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);
		const float3 N = normalize(g1.rgb * 2 - 1);

		uint surfel_alloc;
		surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_COUNT, 1, surfel_alloc);
		if (surfel_alloc < SURFEL_CAPACITY)
		{
			Surfel surfel = (Surfel)0;
			surfel.position = P;
			surfel.normal = pack_unitvector(N);
			surfelBuffer[surfel_alloc] = surfel;

			surfelIndexBuffer[surfel_alloc] = surfel_alloc;
			surfelCellIndexBuffer[surfel_alloc] = surfel_hash(surfel_cell(P));
		}
	}
}
