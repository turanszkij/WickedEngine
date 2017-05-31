#include "objectHF.hlsli"


// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
TYPEDBUFFER(vertexBuffer_TEX, float4, VBSLOT_1);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_2);

PixelInputType_Simple main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	Input_Object_POS_TEX input;
	input.pos = vertexBuffer_POS[vID];
	input.tex = vertexBuffer_TEX[vID];
	input.instance = instanceBuffer[instanceID];



	PixelInputType_Simple Out = (PixelInputType_Simple)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	float4 pos = float4(input.pos.xyz, 1);

	pos = mul(pos, WORLD);

	Out.clip = dot(pos, g_xClipPlane);

	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);


	Out.pos = mul(pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}