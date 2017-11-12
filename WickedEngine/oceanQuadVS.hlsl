#include "oceanWaveGenHF.hlsli"
#include "fullScreenTriangleHF.hlsli"

VS_QUAD_OUTPUT main(uint vI : SV_VertexID)
{
	VS_QUAD_OUTPUT Out;

	FullScreenTriangle(vI, Out.Position, Out.TexCoord);

	return Out;
}
