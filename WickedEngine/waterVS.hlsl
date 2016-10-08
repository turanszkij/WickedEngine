#include "objectHF.hlsli"

PixelInputType main(Input input)
{
	PixelInputType Out = (PixelInputType)0;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input);
		
	float4 pos = input.pos;
	pos = mul( pos,WORLD );
		
	Out.pos = Out.pos2D = mul( pos, g_xCamera_VP );
	Out.pos3D = pos.xyz;
	Out.tex = input.tex.xy;
	Out.nor = mul(input.nor.xyz, (float3x3)WORLD);


	Out.ReflectionMapSamplingPos = mul(pos, g_xFrame_MainCamera_ReflVP);

	return Out;
}