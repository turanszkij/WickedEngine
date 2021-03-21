#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(texture_horizontalpass, float, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float, 0);

// Step size in number of pixels
#define STEP_SIZE 4
// Number of shared-memory samples per direction
#define NUM_STEPS 8
// The last sample has weight = exp(-KERNEL_FALLOFF)
#define KERNEL_FALLOFF 3.0f
#define TAN_ANGLE_BIAS 0
#define DEPTH_FIX 2.0f

static const int TILE_BORDER = NUM_STEPS * STEP_SIZE;
static const int CACHE_SIZE = TILE_BORDER + POSTPROCESS_HBAO_THREADCOUNT + TILE_BORDER;
groupshared float2 cache[CACHE_SIZE];


float Tangent(float2 V)
{
	// Add an epsilon to avoid any division by zero.
	return -V.y / (abs(V.x) + 1.e-6f);
}

float TanToSin(float x)
{
	return x * rsqrt(x * x + 1.0f);
}

float Falloff(float sampleId)
{
	float r = sampleId / (NUM_STEPS - 1);
	return exp(-KERNEL_FALLOFF * r * r);
}

// Compute the HBAO contribution in a given direction on screen by fetching 2D (from Nvidia DX11 SDK)
// view-space coordinates available in shared memory:
// - (X,Z) for the horizontal directions (approximating Y by a constant).
// - (Y,Z) for the vertical directions (approximating X by a constant).
void IntegrateDirection(inout float ao, float2 P, float tanT, int threadId, int deltaX)
{
	float tanH = tanT;
	float sinH = TanToSin(tanH);
	float sinT = TanToSin(tanT);

	[unroll]
	for (int sampleId = 0; sampleId < NUM_STEPS; ++sampleId)
	{
		float2 S = cache[threadId + sampleId * deltaX + deltaX];
		float2 V = S - P;
		float tanS = Tangent(V);
		float d2 = dot(V, V);

		[flatten]
		if ((d2 < DEPTH_FIX) && (tanS > tanH))
		{
			// Accumulate AO between the horizon and the sample
			float sinS = TanToSin(tanS);
			ao += Falloff(sampleId) * (sinS - sinH);

			// Update the current horizon angle
			tanH = tanS;
			sinH = sinS;
		}
	}
}

[numthreads(POSTPROCESS_HBAO_THREADCOUNT, 1, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	const bool horizontal = hbao_direction.y == 0;

	uint2 tile_start = Gid.xy;
	[flatten]
	if (horizontal)
	{
		tile_start.x *= POSTPROCESS_HBAO_THREADCOUNT;
	}
	else
	{
		tile_start.y *= POSTPROCESS_HBAO_THREADCOUNT;
	}

	int i;
	for (i = groupIndex; i < CACHE_SIZE; i += POSTPROCESS_HBAO_THREADCOUNT)
	{
		const uint2 pixel = tile_start + hbao_direction * (i - TILE_BORDER);
		const float2 uv = (pixel + 0.5f) * xPPResolution_rcp;
		const float z = texture_lineardepth.Load(uint3(tile_start + hbao_direction * (i - TILE_BORDER), 1)) * g_xCamera_ZFarP;
		const float2 xy = (hbao_uv_to_view_A * uv + hbao_uv_to_view_B) * z;
		cache[i] = float2(horizontal ? xy.x : xy.y, z);
	}
	GroupMemoryBarrierWithGroupSync();

	const uint2 pixel = tile_start + groupIndex * hbao_direction;
	const int center = TILE_BORDER + groupIndex;
	if (pixel.x >= xPPResolution.x || pixel.y >= xPPResolution.y || cache[center].y >= g_xCamera_ZFarP - 0.99)
	{
		return;
	}

	const float2 sample_left = cache[center - 1];
	const float2 sample_center = cache[center];
	const float2 sample_right = cache[center + 1];

	const float2 V1 = sample_right - sample_center;
	const float2 V2 = sample_center - sample_left;
	const float tangent = Tangent((dot(V1, V1) < dot(V2, V2)) ? V1 : V2);

	float ao = 0;
	IntegrateDirection(ao, sample_center, tangent + TAN_ANGLE_BIAS, center, STEP_SIZE);
	IntegrateDirection(ao, sample_center, -tangent + TAN_ANGLE_BIAS, center, -STEP_SIZE);

	if (!horizontal)
	{
		// vertical pass combines results:
		ao = (ao + texture_horizontalpass[pixel]) * 0.25f;
		ao = pow(saturate(1 - ao), hbao_power);
	}

	output[pixel] = ao;
}
