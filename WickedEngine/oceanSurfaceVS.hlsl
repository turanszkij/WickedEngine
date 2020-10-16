#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

TEXTURE2D(texture_displacementmap, float4, TEXSLOT_ONDEMAND0);

static const float3 QUAD[] = {
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(0, 1, 0),
};

#define infinite g_xCamera_ZFarP
float3 intersectPlaneClampInfinite(in float3 rayOrigin, in float3 rayDirection, in float3 planeNormal, float planeHeight)
{
	float dist = (planeHeight - dot(planeNormal, rayOrigin)) / dot(planeNormal, rayDirection);
	if (dist < 0.0)
		return rayOrigin + rayDirection * dist;
	else
		return float3(rayOrigin.x, planeHeight, rayOrigin.z) - normalize(float3(rayDirection.x, 0, rayDirection.z)) * infinite;
}

PSIn main(uint fakeIndex : SV_VERTEXID)
{
	PSIn Out;

	// fake instancing of quads:
	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;

	// Retrieve grid dimensions and 1/gridDimensions:
	float2 dim = xOceanScreenSpaceParams.xy;
	float2 invdim = xOceanScreenSpaceParams.zw;

	// Assemble screen space grid:
	Out.pos = float4(QUAD[vertexID], 1);
	Out.pos.xy *= invdim;
	Out.pos.xy += (float2)unflatten2D(instanceID, dim.xy) * invdim;
	Out.pos.xy = Out.pos.xy * 2 - 1;
	Out.pos.xy *= max(1, xOceanSurfaceDisplacementTolerance); // extrude screen space grid to tolerate displacement

	// Perform ray tracing of screen grid and plane surface to unproject to world space:
	float3 o = g_xCamera_CamPos;
	float4 r = mul(g_xCamera_InvVP, float4(Out.pos.xy, 0, 1));
	r.xyz /= r.w;
	float3 d = normalize(o.xyz - r.xyz);

	float3 worldPos = intersectPlaneClampInfinite(o, d, float3(0, 1, 0), xOceanWaterHeight);

	// Displace surface:
	float2 uv = worldPos.xz * xOceanPatchSizeRecip;
	float3 displacement = texture_displacementmap.SampleLevel(sampler_linear_wrap, uv + xOceanMapHalfTexel, 0).xzy;
	displacement *= 1 - saturate(distance(g_xCamera_CamPos, worldPos) * 0.005f);
	worldPos += displacement;

	// Reproject displaced surface and output:
	Out.pos = mul(g_xCamera_VP, float4(worldPos, 1));
	Out.pos3D = worldPos;
	Out.uv = uv;
	Out.ReflectionMapSamplingPos = mul(g_xFrame_MainCamera_ReflVP, float4(worldPos, 1));

	return Out;
}
