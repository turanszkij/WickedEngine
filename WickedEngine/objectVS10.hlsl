#include "objectHF.hlsli"



PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;


	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	float4x4 WORLDPREV = MakeWorldMatrixFromInstance(input.instancePrev);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;
	
	float4 pos = input.pos;
	float4 posPrev = input.pre;

	pos = mul( pos,WORLD );
	posPrev = mul(posPrev, WORLDPREV);


	Out.clip = dot(pos, g_xClipPlane);
		
	float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
	affectWind(pos.xyz,input.tex.w,input.id);



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