#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 instanceColor : COLOR;
};

VSOut main(Input input, uint instanceID : SV_INSTANCEID)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input);

	Out.pos = mul(input.pos, WORLD);
	Out.nor = normalize(mul(input.nor.xyz, (float3x3)WORLD));
	Out.tex = input.tex.xy;
	Out.instanceColor = input.color_dither.rgb;

	return Out;
}