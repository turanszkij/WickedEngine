#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"



// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_1);


struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;    // World position
};

GS_CUBEMAP_IN main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	Input_Shadow_POS input;
	input.pos = vertexBuffer_POS[vID];
	input.instance = instanceBuffer[instanceID];



	GS_CUBEMAP_IN Out = (GS_CUBEMAP_IN)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.Pos = mul(float4(input.pos.xyz, 1), WORLD);


	return Out;
}