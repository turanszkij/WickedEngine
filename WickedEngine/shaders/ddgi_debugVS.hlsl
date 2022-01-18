#include "globals.hlsli"
#include "uvsphere.hlsli"
#include "ShaderInterop_DDGI.h"

struct VSOut
{
	float4 pos : SV_Position;
	float3 normal : NORMAL;
	uint probeIndex : PROBEINDEX;
};

VSOut main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VSOut o;
	o.pos = UVSPHERE[vertexID];
	o.normal = o.pos.xyz;
	o.pos.xyz *= ddgi_max_distance() * 0.05;
	o.probeIndex = instanceID;

	const float3 probeCoord = ddgi_probe_coord(o.probeIndex);
	const float3 probePosition = ddgi_probe_position(probeCoord);

	o.pos.xyz += probePosition;
	o.pos = mul(GetCamera().view_projection, o.pos);
	return o;
}
