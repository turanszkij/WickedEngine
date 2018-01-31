#include "envMapHF.hlsli"
#include "icosphere.hlsli"

VSOut_Sky_EnvmapRendering main(uint vid : SV_VERTEXID)
{
	VSOut_Sky_EnvmapRendering Out;
	Out.pos = float4(ICOSPHERE[vid].xyz,0);
	Out.nor = ICOSPHERE[vid].xyz;
	return Out;
}