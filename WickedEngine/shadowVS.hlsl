#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_1);


struct VertexOut
{
	float4 pos				: SV_POSITION;
};

VertexOut main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	Input_Shadow_POS input;
	input.pos = vertexBuffer_POS[vID];
	input.instance = instanceBuffer[instanceID];



	VertexOut Out = (VertexOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
		
	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	affectWind(Out.pos.xyz, input.pos.w, g_xFrame_Time);

	Out.pos = mul(Out.pos, g_xCamera_VP);


	return Out;
}