#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;    // World position 
	float2 Tex		: TEXCOORD0;      // Texture coord
};

GS_CUBEMAP_IN main(Input_Object_POS_TEX input)
{
	GS_CUBEMAP_IN Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.Pos = mul(surface.position, WORLD);
	Out.Tex = surface.uv;

	return Out;
}