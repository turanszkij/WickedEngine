#include "objectHF.hlsli"


// Vertex layout declaration:
RAWBUFFER(vertexBuffer_POS, VBSLOT_0);
RAWBUFFER(vertexBuffer_NOR, VBSLOT_1);
RAWBUFFER(vertexBuffer_TEX, VBSLOT_2);
RAWBUFFER(vertexBuffer_PRE, VBSLOT_3);
RAWBUFFER(instanceBuffer, VBSLOT_4);
RAWBUFFER(instanceBuffer_Prev, VBSLOT_5);


PixelInputType main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
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




	PixelInputType Out = (PixelInputType)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instancePrev);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;
	
	float4 pos = float4(input.pos.xyz, 1);
	float4 posPrev = float4(input.pre.xyz, 1);

	pos = mul( pos,WORLD );
	posPrev = mul(posPrev, WORLDPREV);


	Out.clip = dot(pos, g_xClipPlane);
		
	float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
	affectWind(pos.xyz, input.pos.w, g_xFrame_Time);
	affectWind(posPrev.xyz, input.pos.w, g_xFrame_TimePrev);


	//VERTEX OFFSET MOTION BLUR
	//if(xMotionBlur.x){
		//float offsetMod = dot(input.nor,vel);
		//pos = lerp(pos,posPrev,(offsetMod<0?((1-saturate(offsetMod))*(noiseTex.SampleLevel( sampler_linear_wrap,input.tex.xy,0 ).r)*0.6f):0));
	//}

	Out.pos = Out.pos2D = mul( pos, g_xCamera_VP );
	Out.pos2DPrev = mul(posPrev, g_xFrame_MainCamera_PrevVP);
	Out.pos3D = pos.xyz;
	Out.tex = input.tex.xy;
	Out.nor = normalize(normal);
	Out.nor2D = mul(Out.nor.xyz, (float3x3)g_xCamera_VP).xy;


	Out.ReflectionMapSamplingPos = mul(pos, g_xFrame_MainCamera_ReflVP);


	//Out.vel = mul( vel.xyz, g_xCamera_VP ).xyz;

	Out.ao = input.nor.w;


	return Out;
}