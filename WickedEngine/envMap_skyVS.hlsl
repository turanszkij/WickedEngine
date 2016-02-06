#include "envMapHF.hlsli"
#include "icosphere.hlsli"

VSOut_Sky main(uint vid : SV_VERTEXID)
{
	VSOut_Sky Out;
	Out.pos = float4(ICOSPHERE[vid],0);
	Out.nor = ICOSPHERE[vid];
	return Out;
}