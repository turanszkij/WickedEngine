#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float  clip : SV_ClipDistance0;
};

VSOut main(Input_Object_POS input)
{
	VSOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	surface.position = mul(WORLD, surface.position);

	Out.clip = dot(surface.position, g_xCamera_ClipPlane);

	Out.pos = mul(g_xCamera_VP, surface.position);

	return Out;
}
