#include "objectInputLayoutHF.hlsli"
#include "envMapHF.hlsli"

VSOut main(Input input)
{
	VSOut Out = (VSOut)0;

	[branch]
	if ((uint)input.tex.z == g_xMat_matIndex)
	{

		float4x4 WORLD = MakeWorldMatrixFromInstance(input);
		Out.pos = mul(input.pos, WORLD);
		Out.nor = normalize(mul(normalize(input.nor.xyz), (float3x3)WORLD));
		Out.tex = input.tex.xy;
		Out.instanceColor = input.color_dither.rgb;
		Out.ao = input.nor.w;

	}

	return Out;
}