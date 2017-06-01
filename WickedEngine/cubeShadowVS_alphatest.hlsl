#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_1);
RAWBUFFER(instanceBuffer, VBSLOT_2);

struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;    // World position 
	float2 Tex		: TEXCOORD0;      // Texture coord
};

GS_CUBEMAP_IN main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	Input_Shadow_POS_TEX input;
	input.pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));
	input.tex = asfloat(vertexBuffer_TEX.Load4(fetchAddress));
	const uint fetchAddress_Instance = instanceID * 64;
	input.instance.wi0 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 0));
	input.instance.wi1 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 16));
	input.instance.wi2 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 32));
	input.instance.color_dither = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 48));




	GS_CUBEMAP_IN Out = (GS_CUBEMAP_IN)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.Pos = mul(float4(input.pos.xyz, 1), WORLD);
	Out.Tex = input.tex.xy;


	return Out;
}