#include "effectInputLayoutHF.hlsli"
#include "windHF.hlsli"

cbuffer staticBuffer:register(b0){
	float4x4 xViewProjection;
	float4x4 xRefViewProjection;
	float4x4 xPrevViewProjection;
	float4	 xCamPos;
	float4	 xClipPlane;
	float3	 xWind; float time;
	float	 windRandomness;
	float	 windWaveSize;
}

//#include "skinningHF.hlsli"

cbuffer constantBuffer:register(b3){
	float4	 xDisplace;
};

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
