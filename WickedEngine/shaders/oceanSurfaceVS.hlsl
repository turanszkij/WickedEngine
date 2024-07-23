#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

Texture2D<float4> texture_displacementmap : register(t0);

PSIn main(uint vertexID : SV_VertexID)
{
	PSIn Out;
	
	float2 dim = xOceanScreenSpaceParams.xy;
	float2 invdim = xOceanScreenSpaceParams.zw;
	uint2 grid_coord = unflatten2D(vertexID, dim.xy);
	
	// Assemble screen space grid:
	Out.pos = float4(grid_coord / float2(dim - 1), 1, 1);
	Out.pos.xy = uv_to_clipspace(Out.pos.xy);
	float4 originalpos = Out.pos;
	Out.pos.xy *= max(1, xOceanSurfaceDisplacementTolerance); // extrude screen space grid to tolerate displacement

	// Perform ray tracing of screen grid and plane surface to unproject to world space:
	const float3 o = GetCamera().position;
	float4 unproj = mul(GetCamera().inverse_view_projection, float4(Out.pos.xy, 1, 1));
	unproj.xyz /= unproj.w;
	const float3 d = normalize(o.xyz - unproj.xyz);

	float3 worldPos = intersectPlaneClampInfinite(o, d, float3(0, 1, 0), xOceanWaterHeight);
	
	float2 uv = worldPos.xz * xOceanPatchSizeRecip;
	if (grid_coord.x > 0 && grid_coord.x < dim.x - 1 && grid_coord.y > 0 && grid_coord.y < dim.y - 1) // don't displace the side edges, to avoid holes
	{
		// Displace surface:
		float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, uv, 0).xzy;
		float dist = length(worldPos - GetCamera().position);
		displacement *= saturate(1 - saturate(dist / GetCamera().z_far - 0.8) * 5.0); // fade will be on edge and inwards 20%
		worldPos += displacement;
	}

	// Reproject displaced surface and output:
	Out.pos = mul(GetCamera().view_projection, float4(worldPos, 1));
	Out.pos3D = worldPos;
	Out.uv = uv;

	return Out;
}
