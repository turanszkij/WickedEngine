#include "objectHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_1);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_2);

PixelInputType_Simple main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	Input_Object_POS_TEX input;
	input.pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));
	input.tex = asfloat(vertexBuffer_TEX.Load4(fetchAddress));
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