#include "effectHF_PSVS.hlsli"
#include "effectHF_VS.hlsli"

Texture2D noiseTex:register(t0);
SamplerState texSampler:register(s0);

PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;


	[branch]if((uint)input.tex.z == matIndex)
	{

		float4x4 WORLD = float4x4(
				float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
				,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
				,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
				,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
			);

		Out.dither = input.dither;
	
		float4 pos = input.pos;
		float4 posPrev = input.pre;
		float4 vel = float4(0,0,0,1);
		

		pos = mul( pos,WORLD );

		if(posPrev.w){
			posPrev = mul( posPrev,WORLD );
			vel = pos - posPrev;
		}

		Out.clip = dot(pos, xClipPlane);
		
		float3 normal = mul(normalize(input.nor.xyz), (float3x3)WORLD);
		affectWind(pos.xyz,xWind,time,input.tex.w,input.id,windRandomness,windWaveSize);

		//VERTEX OFFSET MOTION BLUR
		//if(xMotionBlur.x){
			//float offsetMod = dot(input.nor,vel);
			//pos = lerp(pos,posPrev,(offsetMod<0?((1-saturate(offsetMod))*(noiseTex.SampleLevel( texSampler,input.tex.xy,0 ).r)*0.6f):0));
		//}

		Out.pos = Out.pos2D = mul( pos, xViewProjection );
		Out.pos3D = pos.xyz;
		Out.cam = xCamPos.xyz;
		Out.tex = input.tex.xy;
		Out.nor = normalize(normal);


		Out.ReflectionMapSamplingPos = mul(pos, xRefViewProjection );


		Out.vel = mul( mul(vel.xyz,WORLD), xViewProjection ).xyz;

		Out.ao = input.nor.w;

	}

	return Out;
}