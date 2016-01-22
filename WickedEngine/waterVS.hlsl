#include "effectHF_PSVS.hlsli"
#include "effectHF_VS.hlsli"

PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;
	

	[branch]if((uint)input.tex.z == g_xMat_matIndex){
		float4x4 WORLD = float4x4(
				float4(input.wi0.x,input.wi1.x,input.wi2.x,0)
				,float4(input.wi0.y,input.wi1.y,input.wi2.y,0)
				,float4(input.wi0.z,input.wi1.z,input.wi2.z,0)
				,float4(input.wi0.w,input.wi1.w,input.wi2.w,1)
			);
		
		float4 pos = input.pos;
		pos = mul( pos,WORLD );
		
		Out.pos = Out.pos2D = mul( pos, g_xCamera_VP );
		Out.pos3D = pos.xyz;
		Out.tex = input.tex.xy;
		Out.nor = mul(input.nor.xyz, (float3x3)WORLD);
		/*float2x3 tanbin = tangentBinormal(Out.nor);
		Out.tan=tanbin[0];
		Out.bin=tanbin[1];*/


		Out.ReflectionMapSamplingPos = mul(pos, g_xCamera_ReflVP);
		
		//Out.EyeVec = normalize(xCamPos.xyz - pos.xyz);
	}

	return Out;
}