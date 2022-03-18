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

	// Assemble screen space grid:
	Out.pos = float4(unflatten2D(vertexID, dim.xy) * invdim, 0, 1);
	Out.pos.xy = Out.pos.xy * 2 - 1;
	Out.pos.xy *= max(1, xOceanSurfaceDisplacementTolerance); // extrude screen space grid to tolerate displacement

	// Perform ray tracing of screen grid and plane surface to unproject to world space:
	float3 o = GetCamera().position;
	float4 r = mul(GetCamera().inverse_view_projection, float4(Out.pos.xy, 0, 1));
	r.xyz /= r.w;
	float3 d = normalize(o.xyz - r.xyz);

	float3 worldPos = intersectPlaneClampInfinite(o, d, float3(0, 1, 0), xOceanWaterHeight);

	// Displace surface:
	float2 uv = worldPos.xz * xOceanPatchSizeRecip;
	float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, uv + xOceanMapHalfTexel, 0).xzy;
	displacement *= 1 - saturate(distance(GetCamera().position, worldPos) * 0.0025f);
	worldPos += displacement;

	// Reproject displaced surface and output:
	Out.pos = mul(GetCamera().view_projection, float4(worldPos, 1));
	Out.pos3D = worldPos;
	Out.uv = uv;

	return Out;
}
