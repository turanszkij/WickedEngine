#include "envMapHF.hlsli"
#include "icosphere.hlsli"

PSIn_Sky_EnvmapRendering main(uint vid : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	PSIn_Sky_EnvmapRendering output;

	output.RTIndex = instanceID;
	output.pos = mul(xCubemapRenderCams[output.RTIndex].VP, float4(ICOSPHERE[vid].xyz,0));
	output.nor = ICOSPHERE[vid].xyz;

	return output;
}
