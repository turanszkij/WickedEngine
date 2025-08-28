#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

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

static const float depth_diff_allowed = 0.14; // world space dist

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint primitiveID = texture_primitiveID[DTid.xy];
	if (primitiveID == 0)
		return;
		
	const half current_id = mask[DTid.xy];
	const half current_depth = texture_lineardepth[DTid.xy];
	
	ShaderCamera camera = GetCamera();
	half depth_diff_allowed_norm = depth_diff_allowed * camera.z_far_rcp;
	
	uint2 current_edge = edgemap[DTid.xy];
	float2 current_diff = (float2)DTid.xy - (float2)current_edge;
	float best_dist = dot(current_diff, current_diff);

	for (uint i = 0; i < arraysize(offsets); ++i)
	{
		const int2 pixel = DTid.xy + offsets[i] * postprocess.params0.x;
		const uint2 edge = edgemap[pixel];
		const half id = mask[edge];
		if (id != current_id)
		{
			const half depth = texture_lineardepth[edge];
			const half diff = abs(current_depth - depth);
			//if (diff <= depth_diff_allowed_norm)
			{
				const float2 diff2 = (float2)pixel - (float2)edge;
				const float dist = dot(diff2, diff2);
				if (dist < best_dist)
				{
					best_dist = dist;
					current_edge = edge;
				}
			}
		}
	}

	output[DTid.xy] = current_edge;
}
