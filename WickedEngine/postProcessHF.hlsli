#ifndef _POSTPROCESS_HF_
#define _POSTPROCESS_HF_

#include "imageHF.hlsli"
#include "packHF.hlsli"



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
	texture_lineardepth.GetDimensions(dim.x, dim.y);
	return texture_lineardepth.Load(int3(dim*texCoord, 0)).r;
}
float4 loadNormal(float2 texCoord)
{
	float2 dim;
	texture_gbuffer1.GetDimensions(dim.x, dim.y);
	return texture_gbuffer1.Load(int3(dim*texCoord, 0));
}
float4 loadVelocity(float2 texCoord)
{
	float2 dim;
	texture_gbuffer2.GetDimensions(dim.x, dim.y);
	return texture_gbuffer2.Load(int3(dim*texCoord, 0));
}
float4 loadMask(float2 texCoord)
{
	float2 dim;
	texture_1.GetDimensions(dim.x, dim.y);
	return texture_1.Load(int3(dim*texCoord, 0));
}
float4 loadScene(float2 texCoord)
{
	float2 dim;
	texture_0.GetDimensions(dim.x, dim.y);
	return texture_0.Load(int3(dim*texCoord, 0));
}

#endif // _POSTPROCESS_HF_
