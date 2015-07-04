//#include "ViewProp.h"
#include "viewProp.hlsli"
#include "imageHF.hlsli"
#include "globalsHF.hlsli"

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

float loadDepth(float2 texCoord)
{
	float2 dim;
	xSceneDepthMap.GetDimensions(dim.x, dim.y);
	return xSceneDepthMap.Load(int4(dim*texCoord, 0, 0)).r;
}
float4 loadNormal(float2 texCoord)
{
	float2 dim;
	xNormalMap.GetDimensions(dim.x, dim.y);
	return xNormalMap.Load(int4(dim*texCoord, 0, 0));
}
float4 loadVelocity(float2 texCoord)
{
	float2 dim;
	xSceneVelocityMap.GetDimensions(dim.x, dim.y);
	return xSceneVelocityMap.Load(int4(dim*texCoord, 0, 0));
}
float4 loadMask(float2 texCoord)
{
	float2 dim;
	xMaskTex.GetDimensions(dim.x, dim.y);
	return xMaskTex.Load(int4(dim*texCoord, 0, 0));
}
float4 loadScene(float2 texCoord)
{
	float2 dim;
	xTexture.GetDimensions(dim.x, dim.y);
	return xTexture.Load(int4(dim*texCoord, 0, 0));
}