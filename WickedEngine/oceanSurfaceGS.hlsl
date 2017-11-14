#include "globals.hlsli"
#include "oceanSurfaceHF.hlsli"

#define EXTRUDE_SCREENSPACE 2

#define g_texDisplacement	texture_0 // FFT wave displacement map

struct VSOut
{
	// void
};

static const float3 QUAD[] = {
	float3(0, 0, 0),
	float3(0, 1, 0),
	float3(1, 0, 0),
	float3(1, 1, 0),
};


[maxvertexcount(4)]
void main(
	point VSOut input[1], uint vID : SV_PRIMITIVEID,
	inout TriangleStream< GSOut > output
)
{
	GSOut Out[4] = {
		(GSOut)0,
		(GSOut)0,
		(GSOut)0,
		(GSOut)0,
	};

	float2 dim = xOceanScreenSpaceParams.xy;
	float2 invdim = xOceanScreenSpaceParams.zw;

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

		float2 uv = worldPos.xz * xOceanTexMulAdd.xy + xOceanTexMulAdd.zw;

		float3 displacement = g_texDisplacement.SampleLevel(sampler_point_wrap, uv, 0).xyz;
		worldPos.xyz += displacement.xyz;

		Out[i].pos = mul(float4(worldPos, 1), g_xFrame_MainCamera_VP);
		Out[i].pos2D = Out[i].pos;
		Out[i].pos3D = worldPos;
		Out[i].uv = uv;
		Out[i].ReflectionMapSamplingPos = mul(float4(worldPos, 1), g_xFrame_MainCamera_ReflVP);
	}

	for (uint j = 0; j < 4; ++j)
	{
		output.Append(Out[j]);
	}
}


//// Nvidia:
//VS_OUTPUT main(float2 vPos : POSITION)
//{
//	VS_OUTPUT Output;
//
//	// Local position
//	float4 pos_local = mul(float4(vPos, 0, 1), g_matLocal);
//	// UV
//	float2 uv_local = pos_local.xy * g_UVScale + g_UVOffset;
//
//	// Blend displacement to avoid tiling artifact
//	float3 eye_vec = pos_local.xyz - g_LocalEye;
//	float dist_2d = length(eye_vec.xy);
//	float blend_factor = (PATCH_BLEND_END - dist_2d) / (PATCH_BLEND_END - PATCH_BLEND_BEGIN);
//	blend_factor = clamp(blend_factor, 0, 1);
//
//	// Add perlin noise to distant patches
//	float perlin = 0;
//	if (blend_factor < 1)
//	{
//		float2 perlin_tc = uv_local * g_PerlinSize + g_UVBase;
//		float perlin_0 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.x + g_PerlinMovement, 0).w;
//		float perlin_1 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.y + g_PerlinMovement, 0).w;
//		float perlin_2 = g_texPerlin.SampleLevel(sampler_aniso_wrap, perlin_tc * g_PerlinOctave.z + g_PerlinMovement, 0).w;
//
//		perlin = perlin_0 * g_PerlinAmplitude.x + perlin_1 * g_PerlinAmplitude.y + perlin_2 * g_PerlinAmplitude.z;
//	}
//
//	// Displacement map
//	float3 displacement = 0;
//	if (blend_factor > 0)
//		displacement = g_texDisplacement.SampleLevel(sampler_point_wrap, uv_local, 0).xyz;
//	displacement = lerp(float3(0, 0, perlin), displacement, blend_factor);
//	pos_local.xyz += displacement;
//
//	// Transform
//	Output.Position = mul(pos_local, g_matWorldViewProj);
//	Output.LocalPos = pos_local.xyz;
//
//	// Pass thru texture coordinate
//	Output.TexCoord = uv_local;
//
//	return Output;
//}

