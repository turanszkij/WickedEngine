#include "effectHF_PSVS.hlsli"
#include "effectHF_VS.hlsli"

Texture2D noiseTex:register(t0);

PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;


	[branch]if((uint)input.tex.z == g_xMat_matIndex)
	{

		float4x4 WORLD = MakeWorldMatrixFromInstance(input);

		Out.instanceColor = input.color_dither.rgb;
		Out.dither = input.color_dither.a;
	
		float4 pos = input.pos;
		float4 posPrev = input.pre;
		//float4 vel = float4(0,0,0,1);
		

		pos = mul( pos,WORLD );


		Out.clip = dot(pos, g_xClipPlane);
		
		float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
		affectWind(pos.xyz,input.tex.w,input.id);


		[branch]
		if (posPrev.w){
			posPrev = mul(posPrev, WORLD);
			//vel = pos - posPrev;
		}
		else
			posPrev = pos;

		//VERTEX OFFSET MOTION BLUR
		//if(xMotionBlur.x){
			//float offsetMod = dot(input.nor,vel);
			//pos = lerp(pos,posPrev,(offsetMod<0?((1-saturate(offsetMod))*(noiseTex.SampleLevel( sampler_linear_wrap,input.tex.xy,0 ).r)*0.6f):0));
		//}

		Out.pos = Out.pos2D = mul( pos, g_xCamera_VP );
		Out.pos2DPrev = mul(posPrev, g_xCamera_PrevVP);
		Out.pos3D = pos.xyz;
		Out.tex = input.tex.xy;
		Out.nor = normalize(normal);
		Out.nor2D = mul(Out.nor.xyz, (float3x3)g_xCamera_VP);


		Out.ReflectionMapSamplingPos = mul(pos, g_xCamera_ReflVP);


		//Out.vel = mul( vel.xyz, g_xCamera_VP ).xyz;

		Out.ao = input.nor.w;

	}

	return Out;
}