#define OBJECTSHADER_USE_NORMAL
#define OBJECTSHADER_USE_RENDERTARGETARRAYINDEX
#include "objectHF.hlsli"
#include "icosphere.hlsli"

PixelInput main(uint vid : SV_VERTEXID, uint instanceID : SV_INSTANCEID)
{
	PixelInput output;

	output.RTIndex = instanceID;
	output.pos = mul(GetCamera(output.RTIndex).view_projection, float4(ICOSPHERE[vid].xyz,0));
	output.nor = min16float3(ICOSPHERE[vid].xyz);

	return output;
}
