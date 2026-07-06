#include "globals.hlsli"
#include "uvsphere.hlsli"
#include "ShaderInterop_DDGI.h"

// Set to true to also draw probes that don't contribute to the lighting (buried
// in geometry / receiving no light). Off by default so the debug view is not
// cluttered by non-contributing probes and stays cheap - drawing one sphere per
// probe is a lot of overdraw, and buried probes can be a large fraction of the
// grid. Flip to true when you specifically want to inspect all probes.
static const bool DDGI_DEBUG_SHOW_INVALID = false;

// A probe whose average irradiance luminance is below this is treated as
// receiving no light and hidden. This is a robust, geometry-independent test
// for "sealed inside a solid": such a probe only ever sees interior backfaces
// and converges to black, no matter how the surrounding faces confuse the
// backface classifier (e.g. a plane clipping through a box). It is also stable
// frame to frame (unlike the ray-count-based valid flag), so it does not
// flicker. Raise it to prune dim probes too, lower it toward 0 to only prune
// pure black.
static const half DDGI_DEBUG_MIN_LUMINANCE = 0.002;

void main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out half4 col : COLOR)
{
	pos = UVSPHERE[vertexID];

	StructuredBuffer<DDGIProbe> probe_buffer = bindless_structured_ddgi_probes[descriptor_index(GetScene().ddgi.probe_buffer)];
	DDGIProbe probe = probe_buffer[instanceID];
	SH::L1_RGB radiance = probe.radiance.Unpack();
	col = float4(SH::Evaluate(radiance, pos.xyz), 1);

	// instanceID is a global probe index spanning all cascades; decode it into
	// a cascade + local buffer coord to place the debug sphere.
	uint cascade;
	uint3 probeCoord;
	ddgi_decode_probe(instanceID, cascade, probeCoord);
	const float3 probePosition = ddgi_probe_position(cascade, probeCoord);

	// Hide probes that don't contribute: either flagged invalid (buried) by the
	// depth pass, or receiving essentially no light. C[0] is the SH DC term -
	// the probe's average radiance - so its luminance measures total received
	// light. Collapsing the sphere to a point skips its overdraw as well as
	// decluttering.
	const bool probe_valid = (probe.flags & DDGIPROBE_FLAG_VALID) != 0;
	const half received_light = dot(max(0, radiance.C[0]), half3(0.2126, 0.7152, 0.0722));
	const bool contributes = probe_valid && received_light >= DDGI_DEBUG_MIN_LUMINANCE;
	const float radius = (DDGI_DEBUG_SHOW_INVALID || contributes)
		? ddgi_max_distance(cascade) * 0.05
		: 0.0;

	pos.xyz *= radius;
	pos.xyz += probePosition;
	pos = mul(GetCamera().view_projection, pos);
}
