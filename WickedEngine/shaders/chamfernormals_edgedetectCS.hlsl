#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input_normals, float4, TEXSLOT_ONDEMAND0); // DXGI_FORMAT_R8G8B8A8_UNORM

RWTEXTURE2D(output_edgeMap, uint4, 0); // DXGI_FORMAT_R32G32B32A32_UINT

static const int2 offsets[3] = {
	int2(1, 0),
	int2(0, 1),
	int2(1, 1),
};

static float detect_edge_threshold = 0.1f; // lower: less chance for detecting edges
static float detect_corner_threshold = 0.8f; // higher: less chance for detecting corners
static float reject_depth_threshold = 0.001f; // depth difference above this will be detected as separate surface without edge

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	float4 normal = input_normals[DTid.xy];
	normal.xyz = normalize(normal.xyz * 2 - 1);

	const float lineardepth = texture_lineardepth[DTid.xy];

	float3 edgeNormal = 0;
	for (uint i = 0; i < 3; ++i)
	{
		const int2 pixel2 = DTid.xy + offsets[i];

		const float ld = texture_lineardepth[pixel2];
		const float diff = abs(lineardepth - ld);

		if (diff < reject_depth_threshold)
		{
			const float3 n = input_normals[pixel2].xyz * 2 - 1;

			if (abs(dot(normal.xyz, n.xyz)) < detect_edge_threshold)
			{
				edgeNormal = normalize(normal.xyz + n.xyz);
				break;
			}
		}
	}

	uint packed_edgeNormal = 0;
	uint packed_cornerNormal = 0;
	uint packed_pixel = 0;
	if (any(edgeNormal))
	{
		packed_pixel = pack_pixel(DTid.xy);
		packed_edgeNormal = pack_unitvector(edgeNormal);

		float3 cornerNormal = 0;

		float3 N = normal.xyz;
		float3 NR = input_normals[max(0, min(DTid.xy + int2(1, 0), xPPResolution - 1))].xyz * 2 - 1;
		float3 ND = input_normals[max(0, min(DTid.xy + int2(0, 1), xPPResolution - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}
		NR = input_normals[max(0, min(DTid.xy + int2(-1, 0), xPPResolution - 1))].xyz * 2 - 1;
		ND = input_normals[max(0, min(DTid.xy + int2(0, 1), xPPResolution - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}
		NR = input_normals[max(0, min(DTid.xy + int2(1, 0), xPPResolution - 1))].xyz * 2 - 1;
		ND = input_normals[max(0, min(DTid.xy + int2(0, -1), xPPResolution - 1))].xyz * 2 - 1;
		if (abs(dot(N, cross(NR, ND))) > detect_corner_threshold)
		{
			cornerNormal += NR + ND;
		}

		if (any(cornerNormal))
		{
			packed_cornerNormal = pack_unitvector(normalize(N + cornerNormal));
		}
	}
	output_edgeMap[DTid.xy] = uint4(packed_edgeNormal, packed_pixel, packed_cornerNormal, packed_pixel);
}
