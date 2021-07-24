#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

TEXTURE2D(coverage, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(surfelBuffer, Surfel, 0);
RWRAWBUFFER(surfelStatsBuffer, 1);
RWSTRUCTUREDBUFFER(surfelIndexBuffer, uint, 2);
RWSTRUCTUREDBUFFER(surfelCellIndexBuffer, float, 3); // sorting written for floats
RWTEXTURE2D(debugUAV, unorm float4, 4);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Early exit: slow down the propagation by chance
	if (blue_noise(DTid.xy).x < 0.98)
		return;

	uint surfel_coverage = coverage[DTid.xy];

	// Early exit: coverage not applicable (eg. sky)
	if (surfel_coverage == ~0)
		return;

	uint2 pixel = DTid.xy * 16;
	pixel.x += (surfel_coverage >> 4) & 0xF;
	pixel.y += (surfel_coverage >> 0) & 0xF;

	uint coverage_amount = surfel_coverage >> 8;

	const float depth = texture_depth[pixel.xy];

	if (depth > 0 && coverage_amount < 4)
	{
		const float2 uv = ((float2)pixel.xy + 0.5) * g_xFrame_InternalResolution_rcp;

		if (depth > 0)
		{
			const float3 P = reconstructPosition(uv, depth);

			const float4 g1 = texture_gbuffer1.SampleLevel(sampler_linear_clamp, uv, 0);
			const float3 N = normalize(g1.rgb * 2 - 1);

			uint surfel_alloc;
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_COUNT, 1, surfel_alloc);
			if (surfel_alloc < SURFEL_CAPACITY)
			{
				Surfel surfel;
				surfel.position = P;
				surfel.normal = pack_unitvector(N);
				surfelBuffer[surfel_alloc] = surfel;

				surfelIndexBuffer[surfel_alloc] = surfel_alloc;
				surfelCellIndexBuffer[surfel_alloc] = surfel_hash(P);
			}
		}
	}

	//debugUAV[pixel] = float4(1, 0, 0, 0.5);
}
