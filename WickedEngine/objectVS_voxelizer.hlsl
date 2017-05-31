#include "objectHF.hlsli"


// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
TYPEDBUFFER(vertexBuffer_NOR, float4, VBSLOT_1);
TYPEDBUFFER(vertexBuffer_TEX, float4, VBSLOT_2);
TYPEDBUFFER(vertexBuffer_PRE, float4, VBSLOT_3);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_4);
STRUCTUREDBUFFER(instanceBuffer_Prev, Input_InstancePrev, VBSLOT_5);

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 instanceColor : COLOR;
};

VSOut main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	Input_Object_ALL input;
	input.pos = vertexBuffer_POS[vID];
	input.nor = vertexBuffer_NOR[vID];
	input.tex = vertexBuffer_TEX[vID];
	input.pre = vertexBuffer_PRE[vID];
	input.instance = instanceBuffer[instanceID];
	input.instancePrev = instanceBuffer_Prev[instanceID];



	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	Out.nor = normalize(mul(input.nor.xyz, (float3x3)WORLD));
	Out.tex = input.tex.xy;
	Out.instanceColor = input.instance.color_dither.rgb;

	return Out;
}