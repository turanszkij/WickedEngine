#include "globals.hlsli"
#include "uvsphere.hlsli"

struct PSIn
{
	float4 pos : SV_POSITION;
	float4 pos3D : TEXCOORD0;
	float4 pos2D : TEXCOORD1;
};

PSIn main(uint vID : SV_VERTEXID)
{
	PSIn Out;

	Out.pos = UVSPHERE[vID];


	uint forceFieldID = g_xFrame_ForceFieldArrayOffset + (uint)g_xColor.w;
	ShaderEntity forceField = EntityArray[forceFieldID];

	Out.pos.xyz *= forceField.coneAngleCos; // range...
	Out.pos.xyz += forceField.positionWS;

	Out.pos3D = Out.pos;

	Out.pos = mul(g_xTransform, Out.pos);
	Out.pos2D = Out.pos;

	return Out;
}