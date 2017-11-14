#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

#define EXTRUDE_SCREENSPACE 2

#define g_texDisplacement	texture_0 // FFT wave displacement map

static const float3 QUAD[] = {
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(1, 0, 0),
	float3(1, 1, 0),
	float3(0, 1, 0),
};

#define infinite g_xFrame_MainCamera_ZFarP
float3 intersectPlane(in float3 source, in float3 dir, in float3 normal, float height)
{
	// Compute the distance between the source and the surface, following a ray, then return the intersection
	// http://www.cs.rpi.edu/~cutler/classes/advancedgraphics/S09/lectures/11_ray_tracing.pdf
	float distance = (-height - dot(normal, source)) / dot(normal, dir);
	if (distance < 0.0)
		return source + dir * distance;
	else
		return -(float3(source.x, height, source.z) + float3(dir.x, height, dir.z) * infinite);
}

PSIn main(uint fakeIndex : SV_VERTEXID)
{
	PSIn Out;

	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;

	float2 dim = xOceanScreenSpaceParams.xy;
	float2 invdim = xOceanScreenSpaceParams.zw;

	Out.pos = float4(QUAD[vertexID], 1);

	Out.pos.xy *= invdim;
	Out.pos.xy += (float2)unflatten2D(instanceID, dim.xy) * invdim;
	Out.pos.xy = Out.pos.xy * 2 - 1;
	Out.pos.xy *= EXTRUDE_SCREENSPACE;

	float3 o = g_xFrame_MainCamera_CamPos;
	float4 r = mul(float4(Out.pos.xy, 0, 1), g_xFrame_MainCamera_InvVP);
	r.xyz /= r.w;
	float3 d = normalize(o.xyz - r.xyz);

	float3 planeOrigin = float3(0, 0, 0);
	float3 planeNormal = float3(0, 1, 0);

	float3 worldPos = intersectPlane(o, d, planeNormal, planeOrigin.y);

	float2 uv = worldPos.xz * xOceanTexMulAdd.xy + xOceanTexMulAdd.zw;

	float3 displacement = g_texDisplacement.SampleLevel(sampler_point_wrap, uv, 0).xyz;
	worldPos.xzy += displacement.xyz;

	Out.pos = mul(float4(worldPos, 1), g_xFrame_MainCamera_VP);
	Out.pos2D = Out.pos;
	Out.pos3D = worldPos;
	Out.uv = uv;
	Out.ReflectionMapSamplingPos = mul(float4(worldPos, 1), g_xFrame_MainCamera_ReflVP);

	return Out;
}
