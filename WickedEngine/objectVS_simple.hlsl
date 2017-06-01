#include "objectHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_1);
RAWBUFFER(instanceBuffer, VBSLOT_2);

PixelInputType_Simple main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	Input_Object_POS_TEX input;
	input.pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));
	input.tex = asfloat(vertexBuffer_TEX.Load4(fetchAddress));
	const uint fetchAddress_Instance = instanceID * 64;
	input.instance.wi0 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 0));
	input.instance.wi1 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 16));
	input.instance.wi2 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 32));
	input.instance.color_dither = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 48));



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