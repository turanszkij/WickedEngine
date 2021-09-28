#include "globals.hlsli"
#include "brdf.hlsli"
#include "ShaderInterop_SurfelGI.h"


void MultiscaleMeanEstimator(
	float3 y,
	inout SurfelData data,
	float shortWindowBlend = 0.08f
)
{
	float3 mean = data.mean;
	float3 shortMean = data.shortMean;
	float vbbr = data.vbbr;
	float3 variance = data.variance;
	float inconsistency = data.inconsistency;

	// Suppress fireflies.
	{
		float3 dev = sqrt(max(1e-5, variance));
		float3 highThreshold = 0.1 + shortMean + dev * 8;
		float3 overflow = max(0, y - highThreshold);
		y -= overflow;
	}

	float3 delta = y - shortMean;
	shortMean = lerp(shortMean, y, shortWindowBlend);
	float3 delta2 = y - shortMean;

	// This should be a longer window than shortWindowBlend to avoid bias
	// from the variance getting smaller when the short-term mean does.
	float varianceBlend = shortWindowBlend * 0.5;
	variance = lerp(variance, delta * delta2, varianceBlend);
	float3 dev = sqrt(max(1e-5, variance));

	float3 shortDiff = mean - shortMean;

	float relativeDiff = dot(float3(0.299, 0.587, 0.114),
		abs(shortDiff) / max(1e-5, dev));
	inconsistency = lerp(inconsistency, relativeDiff, 0.08);

	float varianceBasedBlendReduction =
		clamp(dot(float3(0.299, 0.587, 0.114),
			0.5 * shortMean / max(1e-5, dev)), 1.0 / 32, 1);

	float3 catchUpBlend = clamp(smoothstep(0, 1,
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

STRUCTUREDBUFFER(surfelBuffer, Surfel, TEXSLOT_ONDEMAND0);
RAWBUFFER(surfelStatsBuffer, TEXSLOT_ONDEMAND1);
STRUCTUREDBUFFER(surfelGridBuffer, SurfelGridCell, TEXSLOT_ONDEMAND2);
STRUCTUREDBUFFER(surfelCellBuffer, uint, TEXSLOT_ONDEMAND3);
STRUCTUREDBUFFER(surfelAliveBuffer, uint, TEXSLOT_ONDEMAND4);
TEXTURE2D(surfelMomentsTexture, float2, TEXSLOT_ONDEMAND5);

RWSTRUCTUREDBUFFER(surfelDataBuffer, SurfelData, 0);

[numthreads(SURFEL_INDIRECT_NUMTHREADS, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint surfel_count = surfelStatsBuffer.Load(SURFEL_STATS_OFFSET_COUNT);
	if (DTid.x >= surfel_count)
		return;

	uint surfel_index = surfelAliveBuffer[DTid.x];
	Surfel surfel = surfelBuffer[surfel_index];
	SurfelData surfel_data = surfelDataBuffer[surfel_index];

	float4 result = float4(surfel_data.traceresult, 1);

#if 1
	// Evaluate surfel cache at hit point for multi bounce:
	{
		Surface surface;
		surface.P = surfel_data.hitpos;
		surface.N = normalize(unpack_unitvector(surfel_data.hitnormal));

		float4 surfel_gi = 0;
		uint cellindex = surfel_cellindex(surfel_cell(surface.P));
		SurfelGridCell cell = surfelGridBuffer[cellindex];
		for (uint i = 0; i < cell.count; ++i)
		{
			uint surfel_index = surfelCellBuffer[cell.offset + i];
			Surfel surfel = surfelBuffer[surfel_index];

			float3 L = surfel.position - surface.P;
			float dist2 = dot(L, L);
			if (dist2 < sqr(surfel.radius))
			{
				float3 normal = normalize(unpack_unitvector(surfel.normal));
				float dotN = dot(surface.N, normal);
				if (dotN > 0)
				{
					float dist = sqrt(dist2);
					float contribution = 1;

					float2 moments = surfelMomentsTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, -L / dist), 0);
					contribution *= surfel_moment_weight(moments, dist);

					contribution *= saturate(1 - dist / surfel.radius);
					contribution = smoothstep(0, 1, contribution);
					contribution *= saturate(dotN);

					surfel_gi += float4(surfel.color, 1) * contribution;

				}
			}
		}
		if (surfel_gi.a > 0)
		{
			surfel_gi.rgb /= surfel_gi.a;
			surfel_gi.a = saturate(surfel_gi.a);
			result.rgb += max(0, surfel_data.hitenergy * surfel_gi.rgb);
		}
	}
#endif




#if 1
	// Surfel irradiance sharing:
	{
		Surface surface;
		surface.P = surfel.position;
		surface.N = normalize(unpack_unitvector(surfel.normal));
		float radius = surfel.radius;

		uint cellindex = surfel_cellindex(surfel_cell(surface.P));
		SurfelGridCell cell = surfelGridBuffer[cellindex];
		for (uint i = 0; i < cell.count; ++i)
		{
			uint surfel_index = surfelCellBuffer[cell.offset + i];
			Surfel surfel = surfelBuffer[surfel_index];
			surfel.radius += radius;

			float3 L = surfel.position - surface.P;
			float dist2 = dot(L, L);
			if (dist2 < sqr(surfel.radius))
			{
				float3 normal = normalize(unpack_unitvector(surfel.normal));
				float dotN = dot(surface.N, normal);
				if (dotN > 0)
				{
					float dist = sqrt(dist2);
					float contribution = 1;

					float2 moments = surfelMomentsTexture.SampleLevel(sampler_linear_clamp, surfel_moment_uv(surfel_index, normal, -L / dist), 0);
					contribution *= surfel_moment_weight(moments, dist);

					contribution *= saturate(1 - dist / surfel.radius);
					contribution = smoothstep(0, 1, contribution);
					contribution *= saturate(dotN);

					result += float4(surfel.color, 1) * contribution;

				}
			}
		}
	}
#endif

	result /= result.a;


	MultiscaleMeanEstimator(result.rgb, surfel_data, 0.08);

	surfelDataBuffer[surfel_index] = surfel_data;
}
