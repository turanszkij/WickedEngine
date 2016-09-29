#ifndef _TILEDLIGHTING_HF_
#define _TILEDLIGHTING_HF_

struct LightArrayType
{
	float3 PositionVS; // View Space!
	float range;
	float4 color;
	float3 PositionWS;
	float energy;
	uint type;
	float shadowBias;
	float _pad0;
	float _pad1;
};

#endif // _TILEDLIGHTING_HF_