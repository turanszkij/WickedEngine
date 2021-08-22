#include "globals.hlsli"
#include "ShaderInterop_SurfelGI.h"
#include "brdf.hlsli"

static const uint2 pixel_offsets[4] = {
	uint2(0, 0), uint2(1, 0),
	uint2(0, 1), uint2(1, 1),
};

TEXTURE2D(coverage, uint, TEXSLOT_ONDEMAND0);

RWSTRUCTUREDBUFFER(surfelDataBuffer, SurfelData, 0);
RWRAWBUFFER(surfelStatsBuffer, 1);
RWSTRUCTUREDBUFFER(surfelIndexBuffer, uint, 2);

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
	pixel = pixel * 2 + pixel_offsets[g_xFrame_FrameCount % 4];

	uint coverage_amount = surfel_coverage >> 8;

	const float depth = texture_depth[pixel.xy];

	// Early exit: slow down the propagation by chance
	//	Closer surfaces have less chance to avoid excessive clumping of surfels
	const float lineardepth = getLinearDepth(depth) * g_xCamera_ZFarP_rcp;
	const float chance = pow(1 - lineardepth, 4);
	if (blue_noise(DTid.xy).x < chance)
		return;

	if (depth > 0 && coverage_amount < SURFEL_TARGET_COVERAGE)
	{
		const float2 uv = ((float2)pixel.xy + 0.5) * g_xFrame_InternalResolution_rcp;
		const float3 P = reconstructPosition(uv, depth);


		uint2 primitiveID = texture_gbuffer0[pixel];
		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		if (surface.load(prim, P))
		{
			uint surfel_alloc;
			surfelStatsBuffer.InterlockedAdd(SURFEL_STATS_OFFSET_COUNT, 1, surfel_alloc);
			if (surfel_alloc < SURFEL_CAPACITY)
			{
				SurfelData surfel_data = (SurfelData)0;
				surfel_data.primitiveID = primitiveID;
				surfel_data.bary = surface.bary.xy;
				surfel_data.inconsistency = 1;
				surfelDataBuffer[surfel_alloc] = surfel_data;

				surfelIndexBuffer[surfel_alloc] = surfel_alloc;
			}
		}

	}
}
