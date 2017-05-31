#include "objectHF.hlsli"


// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_1);

float4 main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID) : SV_POSITION
{
	// Custom fetch vertex buffer:
	Input_Object_POS input;
	input.pos = vertexBuffer_POS[vID];
	input.instance = instanceBuffer[instanceID];


	PixelInputType_Simple Out = (PixelInputType_Simple)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	float4 pos = float4(input.pos.xyz, 1);

	pos = mul(pos, WORLD);

	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);


	return mul(pos, g_xCamera_VP);
}
