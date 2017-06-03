#include "grassHF_GS.hlsli"

[maxvertexcount(8)]
void main(
	point VS_OUT input[1],
	inout TriangleStream< QGS_OUT > output
)
{
	float grassLength = input[0].nor.w;
	float3 root = mul(float4(input[0].pos.xyz,1), xWorld).xyz;

	//Sphere sphere = { mul(float4(root,1), g_xCamera_View).xyz, grassLength };
	//if (!SphereInsideFrustum(sphere, frustum, 1, LOD2))
	//{
	//	return;
	//}


	// create if not culled:

	float2 frame;
	texture_0.GetDimensions(frame.x, frame.y);
	frame.xy /= frame.y;
	frame.x *= 0.5f;

	uint rand = input[0].pos.w;
	//float3 viewDir = xView._m02_m12_m22;
	float4 pos = float4(input[0].pos.xyz, 1);
	float3 color = saturate(xColor.xyz + sin(pos.x - pos.y - pos.z)*0.013f)*0.5;
	float3 normal = /*normalize(*/input[0].nor.xyz/*-wind)*/;
	float3 wind = sin(g_xFrame_Time + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz*0.03*grassLength;
	float3 windPrev = sin(g_xFrame_Time + (pos.x + pos.y + pos.z))*g_xFrame_WindDirection.xyz*0.03*grassLength;

	frame.xy *= grassLength;
	pos.xyz -= normal*0.1*grassLength;

	QGS_OUT element = (QGS_OUT)0;
	element.nor = normal;

	float dist = distance(root, g_xCamera_CamPos);

	[branch]
	if (dist < LOD1)
	{
		float3 front = input[0].tan.xyz;
		float3 right = normalize(cross(front, normal));
		if (rand % 2) right *= -1;

		element.tex = float2(0, 0);
		element.pos = pos;
		element.pos.xyz += -right*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		float3 pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(1, 0);
		element.pos = pos;
		element.pos.xyz += right*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(0, 1);
		element.pos = pos;
		element.pos.xyz += -right*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(1, 1);
		element.pos = pos;
		element.pos.xyz += right*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		output.RestartStrip();

		//cross

		element.tex = float2(0, 0);
		element.pos = pos;
		element.pos.xyz += -front*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(1, 0);
		element.pos = pos;
		element.pos.xyz += front*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(0, 1);
		element.pos = pos;
		element.pos.xyz += -front*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(1, 1);
		element.pos = pos;
		element.pos.xyz += front*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

	}
	else
	{
		float3 front = g_xCamera_View._m02_m12_m22;
		float3 right = normalize(cross(front, normal));
#ifdef GRASS_FADE_DITHER
		static const float grassPopDistanceThreshold = 0.9f;
		element.fade = pow(saturate(distance(pos.xyz, g_xCamera_CamPos.xyz) / (LOD2*grassPopDistanceThreshold)), 10);
#endif

		element.tex = float2(rand % 2 ? 1 : 0, 0);
		element.pos = pos;
		element.pos.xyz += -right*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		float3 pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(rand % 2 ? 0 : 1, 0);
		element.pos = pos;
		element.pos.xyz += right*frame.x + normal*frame.y;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz + wind;
		pos3DPrev = element.pos.xyz + windPrev;
		element.pos = element.pos2D = mul(float4(element.pos3D, 1), g_xCamera_VP);
		element.pos2DPrev = mul(float4(pos3DPrev, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(rand % 2 ? 1 : 0, 1);
		element.pos = pos;
		element.pos.xyz += -right*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);

		element.tex = float2(rand % 2 ? 0 : 1, 1);
		element.pos = pos;
		element.pos.xyz += right*frame.x;
		element.pos = mul(element.pos, xWorld);
		element.pos3D = element.pos.xyz;
		element.pos = element.pos2D = mul(element.pos, g_xCamera_VP);
		element.pos2DPrev = mul(float4(element.pos3D, 1), g_xFrame_MainCamera_PrevVP);
		output.Append(element);
	}
}