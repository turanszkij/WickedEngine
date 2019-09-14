#include "globals.hlsli"

struct PSIn
{
	float4 pos : SV_POSITION;
	float4 pos3D : TEXCOORD0;
	float4 pos2D : TEXCOORD1;
};

float4 main(PSIn input) : SV_TARGET
{
	uint forceFieldID = g_xFrame_ForceFieldArrayOffset + (uint)g_xColor.w;
	ShaderEntity forceField = EntityArray[forceFieldID];

	float4 color = forceField.energy < 0 ? float4(0, 0, 1, 1) : float4(1, 0, 0, 1);

	if (forceField.GetType() == ENTITY_TYPE_FORCEFIELD_POINT)
	{
		// point-like forcefield:
		float3 centerToPos = normalize(input.pos3D.xyz - forceField.positionWS.xyz);
		float3 eyeToCenter = normalize(forceField.positionWS.xyz - g_xColor.xyz);
		color.a *= pow(saturate(dot(centerToPos, eyeToCenter)), 1.0f / max(0.0001f, abs(forceField.energy)));
	}
	else
	{
		// planar forcefield:
		float3 dir = forceField.positionWS - input.pos3D.xyz;
		float dist = dot(forceField.directionWS, dir);
		color.a *= pow(1 - saturate(dist * forceField.range), 1.0f / max(0.0001f, abs(forceField.energy)));
	}

	float2 pTex = input.pos2D.xy / input.pos2D.w * float2(0.5f, -0.5f) + 0.5f;
	float4 depthScene = texture_lineardepth.GatherRed(sampler_linear_clamp, pTex) * g_xCamera_ZFarP;
	float depthFragment = input.pos2D.w;
	float fade = saturate(1.0 / forceField.coneAngleCos * abs(forceField.energy) * (max(max(depthScene.x, depthScene.y), max(depthScene.z, depthScene.w)) - depthFragment));
	color.a *= fade;

	color.a = saturate(color.a * 0.78f);

	return color;
}