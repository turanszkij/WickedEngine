#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "icosphere.hlsli"

PixelInput main(uint vid : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	PixelInput output;

	output.RTIndex = instanceID;
	output.pos = mul(xCubemapRenderCams[output.RTIndex].VP, float4(ICOSPHERE[vid].xyz,0));
	output.nor = ICOSPHERE[vid].xyz;

	return output;
}
