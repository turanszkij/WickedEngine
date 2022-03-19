#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

Texture2D<float4> texture_displacementmap : register(t0);

#define infinite GetCamera().z_far
float3 intersectPlaneClampInfinite(in float3 rayOrigin, in float3 rayDirection, in float3 planeNormal, float planeHeight)
{
	float dist = (planeHeight - dot(planeNormal, rayOrigin)) / dot(planeNormal, rayDirection);
	if (dist < 0.0)
		return rayOrigin + rayDirection * dist;
	else
		return float3(rayOrigin.x, planeHeight, rayOrigin.z) - normalize(float3(rayDirection.x, 0, rayDirection.z)) * infinite;
}

PSIn main(uint vertexID : SV_VertexID)
{
	PSIn Out;

	// Retrieve grid dimensions and 1/gridDimensions:
	float2 dim = xOceanScreenSpaceParams.xy;
	float2 invdim = xOceanScreenSpaceParams.zw;
	uint2 grid_coord = unflatten2D(vertexID, dim.xy);

	// Assemble screen space grid:
	Out.pos = float4(grid_coord * invdim, 0, 1);
	Out.pos.xy = Out.pos.xy * 2 - 1;
	Out.pos.xy *= max(1, xOceanSurfaceDisplacementTolerance); // extrude screen space grid to tolerate displacement

	// Perform ray tracing of screen grid and plane surface to unproject to world space:
	float3 o = GetCamera().position;
	float4 unproj = mul(GetCamera().inverse_view_projection, float4(Out.pos.xy, 1, 1));
	unproj.xyz /= unproj.w;
	float3 d = normalize(o.xyz - unproj.xyz);

	float3 worldPos = intersectPlaneClampInfinite(o, d, float3(0, 1, 0), xOceanWaterHeight);

	float2 uv = worldPos.xz * xOceanPatchSizeRecip;
	if (grid_coord.x > 0 && grid_coord.x < dim.x - 1 && grid_coord.y > 0 && grid_coord.y < dim.x - 1) // don't displace the side edges, to avoid holes
	{
		// Displace surface:
		float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, uv, 0).xzy;
		worldPos += displacement;
	}

	// Reproject displaced surface and output:
	Out.pos = mul(GetCamera().view_projection, float4(worldPos, 1));
	Out.pos3D = worldPos;
	Out.uv = uv;

	return Out;
}
