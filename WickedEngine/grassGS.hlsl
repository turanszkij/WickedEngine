#include "grassHF_GS.hlsli"

[maxvertexcount(42)]
void main(
	point VS_OUT input[1],
	inout TriangleStream< GS_OUT > output
)
{
	float grassLength = input[0].nor.w;
	float3 root = mul(float4(input[0].pos.xyz, 1), xWorld).xyz;

	Sphere sphere = { mul(float4(root,1), g_xCamera_View).xyz, grassLength };
	if (!SphereInsideFrustum(sphere, frustum, 1, LOD2))
	{
		return;
	}


	uint rand = input[0].pos.w;
	float4 pos = float4(input[0].pos.xyz, 1);
	float3 color = saturate(xColor.xyz + sin(pos.x - pos.y - pos.z)*0.013f)*0.5;
	float3 wind = sin(g_xFrame_Time + (pos.x + pos.y + pos.z)*0.1f)*g_xFrame_WindDirection.xyz*0.1;
	if (rand % (uint)g_xFrame_WindRandomness) wind = -wind;
	float3 normal = input[0].nor.xyz;

	float3 right = normalize(cross(input[0].tan.xyz, normal))*0.3;
	if (rand % 2) right *= -1;

	float dist = distance(root, g_xCamera_CamPos);

	[branch]
	if (dist < LOD0)
	{
		for (uint i = 0; i<6; ++i) {
			float4 mod = pos + float4(cross(MOD[i], normal), 0);
			genBlade(output, mod, normal, grassLength, 1, right, color, wind, 0);
		}
	}
	else if (dist < LOD1)
	{
		for (uint i = 0; i<3; ++i) {
			float4 mod = pos + float4(cross(MOD[i], normal), 0);
			genBlade(output, mod, normal, grassLength, 2, right, color, wind, 0);
		}
	}
	else
	{
		[unroll]
		for (uint i = 0; i<1; ++i) {
			float4 mod = pos + float4(cross(MOD[i], normal), 0);
#ifdef GRASS_FADE_DITHER
			static const float grassPopDistanceThreshold = 0.9f;
			const float fade = pow(saturate(dist / (LOD2*grassPopDistanceThreshold)), 10);
#else
			static const float fade = 0;
#endif
			genBlade(output, mod, normal, grassLength, 4, right, color, wind, fade);
		}
	}



}