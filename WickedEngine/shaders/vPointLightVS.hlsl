#include "globals.hlsli"
#include "circle.hlsli"
#include "lightingHF.hlsli"

void main(float4 pos : POSITION, uint instanceID : SV_InstanceID, out float4 outpos : SV_Position, out float4 col : COLOR, out uint rtindex : SV_RenderTargetArrayIndex)
{
	col = float4(xLightColor.rgb, 1);
	outpos = mul(GetCamera(instanceID).view_projection, pos);
	rtindex = instanceID;
}
