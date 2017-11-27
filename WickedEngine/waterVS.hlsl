#include "objectHF.hlsli"

PixelInputType main(Input_Object_ALL input)
{
	PixelInputType Out = (PixelInputType)0;
	
	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);
		
	surface.position = mul(surface.position, WORLD);
		
	Out.pos3D = surface.position.xyz;
	Out.pos = Out.pos2D = mul(surface.position, g_xCamera_VP);
	Out.tex = surface.uv;
	Out.nor = normalize(mul(surface.normal, (float3x3)WORLD));
	Out.nor2D = mul(Out.nor.xyz, (float3x3)g_xCamera_VP).xy;

	Out.ReflectionMapSamplingPos = mul(surface.position, g_xFrame_MainCamera_ReflVP);

	Out.instanceColor = input.instance.color_dither.rgb;
	Out.dither = input.instance.color_dither.a;

	return Out;
}