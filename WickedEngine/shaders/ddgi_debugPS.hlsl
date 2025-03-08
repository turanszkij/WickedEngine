#include "globals.hlsli"
#include "ShaderInterop_DDGI.h"

struct VSOut
{
	float4 pos : SV_Position;
	float3 normal : NORMAL;
	uint probeIndex : PROBEINDEX;
};

float4 main(VSOut input) : SV_Target
{
	float3 color = 0;

	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	DDGIProbe probe = probe_buffer[input.probeIndex];
	color = SH::Evaluate(SH::Unpack(probe.radiance), input.normal);
	
	return float4(color, 1);
}
