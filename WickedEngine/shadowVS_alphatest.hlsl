#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "windHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_1);
RAWBUFFER(instanceBuffer, VBSLOT_2);


struct VertexOut
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

VertexOut main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	Input_Object_ALL input;
	input.pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));
	input.tex = asfloat(vertexBuffer_TEX.Load4(fetchAddress));
	const uint fetchAddress_Instance = instanceID * 64;
	input.instance.wi0 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 0));
	input.instance.wi1 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 16));
	input.instance.wi2 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 32));
	input.instance.color_dither = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 48));




	VertexOut Out = (VertexOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	affectWind(Out.pos.xyz, input.pos.w, g_xFrame_Time);

	Out.pos = mul(Out.pos, g_xCamera_VP);
	Out.tex = input.tex.xy;


	return Out;
}