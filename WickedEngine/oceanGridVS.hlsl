#include "globals.hlsli"

static const float3 QUAD[] = {
	float3(0, 0, 0),	// 0
	float3(1, 0, 0),	// 1
	float3(0, 1, 0),	// 2
	float3(0, 1, 0),	// 3
	float3(1, 0, 0),	// 4
	float3(1, 1, 0),	// 5
};


struct VSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float a : TEXCOOD0;
};

VSOut main(uint fakeIndex : SV_VERTEXID)
{
	VSOut Out;

	uint vertexID = fakeIndex % 6;
	uint instanceID = fakeIndex / 6;
	
	Out.pos = float4(QUAD[vertexID], 1);

	float2 dim = g_xColor.xy;
	float2 invdim = g_xColor.zw;

	Out.pos.xy *= invdim;
	Out.pos.xy += (float2)unflatten2D(instanceID, dim.xy) * invdim;
	Out.pos.xy = Out.pos.xy * 2 - 1;

	float4 o = mul(float4(Out.pos.xy, 0, 1), g_xFrame_MainCamera_InvVP);
	o.xyz /= o.w;
	float4 r = mul(float4(Out.pos.xy, 1, 1), g_xFrame_MainCamera_InvVP);
	r.xyz /= r.w;
	float3 d = normalize(r.xyz - o.xyz);
	
	float3 planeOrigin = float3(0, 0, 0);
	float3 planeNormal = float3(0, 1, 0);

	float a = dot(planeNormal, d); 
	float t = dot(planeNormal, (planeOrigin - o.xyz) / a);
	float3 pos = o.xyz + d.xyz * t;

	Out.pos = mul(float4(pos, 1), g_xFrame_MainCamera_VP);
	Out.color = float4(pos, 1);
	Out.a = a;

	return Out;
}