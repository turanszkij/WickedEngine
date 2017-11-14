#include "globals.hlsli"

#define EXTRUDE_SCREENSPACE 2

static const float3 QUAD[] = {
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(1, 1, 0),
};

struct GSOut
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
};

[maxvertexcount(4)]
void main(
	point float4 input[1] : SV_POSITION, uint vID : SV_PRIMITIVEID,
	inout TriangleStream< GSOut > output
)
{
	GSOut Out[4] = {
		(GSOut)0,
		(GSOut)0,
		(GSOut)0,
		(GSOut)0,
	};

	float2 dim = g_xColor.xy;
	float2 invdim = g_xColor.zw;

	float3 o = g_xFrame_MainCamera_CamPos;

	for (uint i = 0; i < 4; ++i)
	{
		Out[i].pos = float4(QUAD[i], 1);

		Out[i].pos.xy *= invdim;
		Out[i].pos.xy += (float2)unflatten2D(vID, dim.xy) * invdim;
		Out[i].pos.xy = Out[i].pos.xy * 2 - 1;
		Out[i].pos.xy *= EXTRUDE_SCREENSPACE;

		float4 r = mul(float4(Out[i].pos.xy, 0, 1), g_xFrame_MainCamera_InvVP);
		r.xyz /= r.w;
		float3 d = normalize(r.xyz - o.xyz);

		float3 planeOrigin = float3(0, 0, 0);
		float3 planeNormal = float3(0, 1, 0);

		float planeDot = dot(planeNormal, d);

		if (planeDot > 0)
		{
			return;
		}

		float t = dot(planeNormal, (planeOrigin - o.xyz) / planeDot);
		float3 worldPos = o.xyz + d.xyz * t;

		float3 displacement = texture_0.SampleLevel(sampler_point_wrap, worldPos.xz * 0.01f, 0).xyz;
		worldPos.xyz += displacement.xyz;
		//worldPos.y += displacement.y;

		Out[i].pos = mul(float4(worldPos, 1), g_xFrame_MainCamera_VP);
		Out[i].color = float4(1, 1, 1, 1);
	}

	for (uint j = 0; j < 4; ++j)
	{
		output.Append(Out[j]);
	}
}