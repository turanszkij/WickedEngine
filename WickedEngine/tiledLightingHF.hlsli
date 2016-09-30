#ifndef _TILEDLIGHTING_HF_
#define _TILEDLIGHTING_HF_

struct LightArrayType
{
	float3 PositionVS; // View Space!
	float range;
	float4 color;
	float3 PositionWS;
	float energy;
	float3 direction;
	uint type;
	float shadowBias;
	float _pad0;
	float _pad1;
	float _pad2;
};

#endif // _TILEDLIGHTING_HF_