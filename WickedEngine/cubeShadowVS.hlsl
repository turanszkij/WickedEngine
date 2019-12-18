#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"


struct GS_CUBEMAP_IN
{
	float4 Pos		: SV_POSITION;    // World position
	uint faceIndex	: FACEINDEX;
};

GS_CUBEMAP_IN main(Input_Object_POS input)
{
	GS_CUBEMAP_IN Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.Pos = mul(WORLD, surface.position);
	Out.faceIndex = input.inst.userdata.y;

	return Out;
}