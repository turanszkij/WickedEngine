#include "globals.hlsli"
#include "cone.hlsli"
#include "lightingHF.hlsli"

void main(uint vID : SV_VertexID, uint instanceID : SV_InstanceID, out float4 pos : SV_Position, out float4 col : COLOR, out uint rtindex : SV_RenderTargetArrayIndex)
{
	pos = CONE[vID];
	col = float4(xLightColor.rgb, 1) * saturate(1 - dot(pos.xyz, pos.xyz));

	pos = mul(xLightWorld, pos);
	pos = mul(GetCamera(instanceID).view_projection, pos);
	rtindex = instanceID;
}
