#include "objectHF.hlsli"


// Vertex layout declaration:
TYPEDBUFFER(vertexBuffer_POS, float4, VBSLOT_0);
TYPEDBUFFER(vertexBuffer_NOR, float4, VBSLOT_1);
TYPEDBUFFER(vertexBuffer_TEX, float4, VBSLOT_2);
TYPEDBUFFER(vertexBuffer_PRE, float4, VBSLOT_3);
STRUCTUREDBUFFER(instanceBuffer, Input_Instance, VBSLOT_4);
STRUCTUREDBUFFER(instanceBuffer_Prev, Input_InstancePrev, VBSLOT_5);


PixelInputType main(uint vID : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	// Custom fetch vertex buffer:
	Input_Object_ALL input;
	input.pos = vertexBuffer_POS[vID];
	input.nor = vertexBuffer_NOR[vID];
	input.tex = vertexBuffer_TEX[vID];
	input.pre = vertexBuffer_PRE[vID];
	input.instance = instanceBuffer[instanceID];
	input.instancePrev = instanceBuffer_Prev[instanceID];




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