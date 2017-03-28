#include "postProcessHF.hlsli"

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	//float4 neighborhood[9];
	//neighborhood[0] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(-1, -1), 0));
	//neighborhood[1] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(0, -1), 0));
	//neighborhood[2] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(1, -1), 0));
	//neighborhood[3] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(-1, 0), 0));
	//neighborhood[4] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(0, 0), 0)); // center
	//neighborhood[5] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(1, 0), 0));
	//neighborhood[6] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(-1, 1), 0));
	//neighborhood[7] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(0, 1), 0));
	//neighborhood[8] = xMaskTex.Load(uint3(PSIn.pos.xy + uint2(1, 1), 0));
	//float4 neighborhoodMin = neighborhood[0];
	//float4 neighborhoodMax = neighborhood[0];
	//[unroll]
	//for (uint i = 1; i < 9; ++i)
	//{
	//	neighborhoodMin = min(neighborhoodMin, neighborhood[i]);
	//	neighborhoodMax = max(neighborhoodMax, neighborhood[i]);
	//}

	//float4 history = clamp(neighborhood[4], neighborhoodMin, neighborhoodMax);
	float4 history = xMaskTex.Load(uint3(PSIn.pos.xy, 0));

	float4 current = xTexture.Load(uint3(PSIn.pos.xy,0));

	return lerp(history, current, 0.05f);
}