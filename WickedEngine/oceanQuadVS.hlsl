#include "oceanHF.hlsli"

VS_QUAD_OUTPUT main(float4 vPos : POSITION)
{
	VS_QUAD_OUTPUT Output;

	Output.Position = vPos;
	Output.TexCoord.x = 0.5f + vPos.x * 0.5f;
	Output.TexCoord.y = 0.5f - vPos.y * 0.5f;

	return Output;
}
