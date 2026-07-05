#include "globals.hlsli"
#include "uvsphere.hlsli"
#include "ShaderInterop_DDGI.h"

// Set to true to also draw probes that are invalid (buried in geometry and thus
// not contributing to the lighting). Off by default so the debug view is not
// cluttered by non-contributing probes and stays cheap - drawing one sphere per
// probe is a lot of overdraw, and buried probes can be a large fraction of the
// grid on terrain. Flip to true when you specifically want to inspect all
// probes.
static const bool DDGI_DEBUG_SHOW_INVALID = false;

void main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = UVSPHERE[vertexID];

	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	DDGIProbe probe = probe_buffer[instanceID];
	col = float4(SH::Evaluate(probe.radiance.Unpack(), pos.xyz), 1);

	const float3 probeCoord = ddgi_probe_coord(instanceID);
	const float3 probePosition = ddgi_probe_position(probeCoord);

	// Invalid (buried) probes are collapsed to a point so their sphere is not
	// rasterized - this both declutters the view and skips their overdraw cost.
	const bool probe_valid = (probe.flags & DDGIPROBE_FLAG_VALID) != 0;
	const float radius = (DDGI_DEBUG_SHOW_INVALID || probe_valid)
		? ddgi_max_distance() * 0.05
		: 0.0;

	pos.xyz *= radius;
	pos.xyz += probePosition;
	pos = mul(GetCamera().view_projection, pos);
}
