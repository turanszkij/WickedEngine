#ifndef _TILEDLIGHTING_HF_
#define _TILEDLIGHTING_HF_

struct LightArrayType
{
	float3 positionVS; // View Space!
	float range;
	// --
	float4 color;
	// --
	float3 positionWS;
	float energy;
	// --
	float3 directionVS;
	float _pad0;
	// --
	float3 directionWS;
	uint type;
	// --
	float shadowBias;
	int shadowMap_index;
	float coneAngle;
	float coneAngleCos;
	// --
	float4x4 shadowMat[3];
};

#endif // _TILEDLIGHTING_HF_