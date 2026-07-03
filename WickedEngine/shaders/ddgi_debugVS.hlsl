#include "globals.hlsli"
#include "uvsphere.hlsli"
#include "ShaderInterop_DDGI.h"

void main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = UVSPHERE[vertexID];

	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	DDGIProbe probe = probe_buffer[instanceID];
	col = float4(SH::Evaluate(probe.radiance.Unpack(), pos.xyz), 1);

	const float3 probeCoord = ddgi_probe_coord(instanceID);
	const float3 probePosition = ddgi_probe_position(probeCoord);

	pos.xyz *= ddgi_max_distance() * 0.05;
	pos.xyz += probePosition;
	pos = mul(GetCamera().view_projection, pos);
}
