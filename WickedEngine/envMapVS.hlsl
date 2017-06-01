#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_NOR, VBSLOT_1);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_2);
RAWBUFFER(vertexBuffer_PRE, VBSLOT_3);
RAWBUFFER(instanceBuffer, VBSLOT_4);
RAWBUFFER(instanceBuffer_Prev, VBSLOT_5);


VSOut main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	const uint fetchAddress = vID * 16;
	Input_Object_ALL input;
	input.pos = asfloat(vertexBuffer_POS.Load4(fetchAddress));
	input.nor = asfloat(vertexBuffer_NOR.Load4(fetchAddress));
	input.tex = asfloat(vertexBuffer_TEX.Load4(fetchAddress));
	input.pre = asfloat(vertexBuffer_PRE.Load4(fetchAddress));
	const uint fetchAddress_Instance = instanceID * 64;
	input.instance.wi0 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 0));
	input.instance.wi1 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 16));
	input.instance.wi2 = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 32));
	input.instance.color_dither = asfloat(instanceBuffer.Load4(fetchAddress_Instance + 48));
	const uint fetchAddress_Instance_Prev = instanceID * 48;
	input.instancePrev.wiPrev0 = asfloat(instanceBuffer_Prev.Load4(fetchAddress_Instance_Prev + 0));
	input.instancePrev.wiPrev1 = asfloat(instanceBuffer_Prev.Load4(fetchAddress_Instance_Prev + 16));
	input.instancePrev.wiPrev2 = asfloat(instanceBuffer_Prev.Load4(fetchAddress_Instance_Prev + 32));




	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	Out.nor = normalize(mul(normalize(input.nor.xyz), (float3x3)WORLD));
	Out.tex = input.tex.xy;
	Out.instanceColor = input.instance.color_dither.rgb;
	Out.ao = input.nor.w;

	return Out;
}