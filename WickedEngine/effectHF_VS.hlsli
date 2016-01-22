#ifndef _EFFECTHF_VS_
#define _EFFECTHF_VS_

#include "effectInputLayoutHF.hlsli"
#include "windHF.hlsli"

inline float2x3 tangentBinormal(in float3 normal){
	float3 tangent; 
	float3 binormal; 
	
	float3 c1 = cross(normal, float3(0.0, 0.0, 1.0)); 
	float3 c2 = cross(normal, float3(0.0, 1.0, 0.0)); 
	
	if(length(c1)>length(c2))
	{
		tangent = c1;	
	}
	else
	{
		tangent = c2;	
	}
	
	tangent = normalize(tangent);
	
	binormal = cross(normal, tangent); 
	binormal = normalize(binormal);

	return float2x3(tangent,binormal);
}

#endif // _EFFECTHF_VS_
