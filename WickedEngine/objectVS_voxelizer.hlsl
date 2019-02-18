#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 instanceColor : COLOR;
};

VSOut main(Input_Object_POS_TEX input, uint instanceID : SV_INSTANCEID)
{
	VSOut Out;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.inst);
	VertexSurface surface = MakeVertexSurfaceFromInput(input);

	Out.pos = mul(surface.position, WORLD);
	Out.nor = normalize(mul(surface.normal, (float3x3)WORLD));
	Out.tex = surface.uv;
	Out.instanceColor = input.inst.color_dither.rgb;

	return Out;
}