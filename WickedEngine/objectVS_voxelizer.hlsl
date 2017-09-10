#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float3 nor : NORMAL;
	float2 tex : TEXCOORD;
	float3 instanceColor : COLOR;
};

VSOut main(Input_Object_ALL input, uint instanceID : SV_INSTANCEID)
{
	VSOut Out = (VSOut)0;

	float4x4 WORLD = MakeWorldMatrixFromInstance(input.instance);

	Out.pos = mul(float4(input.pos.xyz, 1), WORLD);
	Out.nor = normalize(mul(input.nor.xyz * 2 - 1, (float3x3)WORLD));
	Out.tex = input.tex.xy;
	Out.instanceColor = input.instance.color_dither.rgb;

	return Out;
}