#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

// Purpose of this shader: used for multipass jump flood to expand edges.
//	Always replaces edges only if a closer one was found, working down from high to low spread and improving precision in each pass

PUSHCONSTANT(postprocess, PostProcess);

Texture2D<uint2> edgemap : register(t0);
Texture2D<half> mask : register(t1);

RWTexture2D<uint2> output : register(u0);

static const int2 offsets[] = {
	int2(-1, 0),
	int2(1, 0),
	int2(0, -1),
	int2(0, 1),
	
	int2(-1, -1),
	int2(-1, 1),
	int2(1, -1),
	int2(1, 1),
};

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint primitiveID = texture_primitiveID[DTid.xy];
	if (primitiveID == 0)
		return;
	
	const half current_id = mask[DTid.xy];
	
	uint2 current_edge = edgemap[DTid.xy];
	float2 current_diff = (float2)DTid.xy - (float2)current_edge;
	float best_dist = dot(current_diff, current_diff);

	for (uint i = 0; i < arraysize(offsets); ++i)
	{
		const uint2 edge = edgemap[clamp(DTid.xy + offsets[i] * postprocess.params0.x, 0, postprocess.resolution - 1)];
		const half id = mask[edge];
		if (id != current_id)
		{
			const float2 diff = (float2)DTid.xy - (float2)edge;
			const float dist = dot(diff, diff);
			if (dist < best_dist)
			{
				best_dist = dist;
				current_edge = edge;
			}
		}
	}

	output[DTid.xy] = current_edge;
}
