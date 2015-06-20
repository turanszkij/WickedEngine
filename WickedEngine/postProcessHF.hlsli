//#include "ViewProp.h"
#include "viewProp.hlsli"
#include "imageHF.hlsli"

cbuffer processBuffer:register(b1){
	float4 xPostProcess; //motion blur|outline|DOF|ssss
	float4 xBloom;
};
cbuffer psbuffer:register(b2){
	float4 xMaskFadOpaDis;
	float4 xDimension;
};



struct VertextoPixel
{
	float4 pos				: SV_POSITION;
	float2 tex				: TEXCOORD0;
};

#include "depthConvertHF.hlsli"

/*inline float getDepth(float4 c)
{
	float z_b = c.x;
    float z_n = 2.0 * z_b - 1.0;
    float lin = 2.0 * zNearP * zFarP / (zFarP + zNearP - z_n * (zFarP - zNearP));
	return lin*0.01f;
}*/