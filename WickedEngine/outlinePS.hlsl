#include "postProcessHF.hlsli"


static const float OUTLINETHRESHOLDDEPTH = 0.0011f;
static const float OUTLINEWIDTHDEPTH = 1.0f;
static const float OUTLINETHRESHOLDNORMAL = 0.6f;
static const float OUTLINEWIDTHNORMAL = 2.0f;
inline float edgeValueDepth(float2 texCo, float2 texDim, float Thickness, float Threshold)
{
	float2 QuadScreenSize = float2(texDim.x,texDim.y);
	float result = 1;

		
	float2 uv = texCo.xy;
	float midDepth = xSceneDepthMap.SampleLevel(Sampler,uv,0);

	//[branch]if(abs(midDepth-zFarP)>0.01)
	{
	
		//Thickness/=midDepth;
		Threshold*=midDepth;
		
		float2 ox = float2(Thickness/QuadScreenSize.x,0.0);
		float2 oy = float2(0.0,Thickness/QuadScreenSize.y);
		float2 PP = uv - oy;

		float CC = xSceneDepthMap.SampleLevel(Sampler,(PP-ox),0); float g00 = (CC)*0.01f;
		CC = xSceneDepthMap.SampleLevel(Sampler,PP,0);    float g01 = (CC)*0.01f;
		CC = xSceneDepthMap.SampleLevel(Sampler,(PP+ox),0); float g02 = (CC)*0.01f;
		PP = uv;
		CC = xSceneDepthMap.SampleLevel(Sampler,(PP-ox),0); float g10 = (CC)*0.01f;
		CC = midDepth;    float g11 = (CC)*0.01f;
		CC = xSceneDepthMap.SampleLevel(Sampler,(PP+ox),0); float g12 = (CC)*0.01f;
		PP = uv + oy;			   
		CC = xSceneDepthMap.SampleLevel(Sampler,(PP-ox),0); float g20 = (CC)*0.01f;
		CC = xSceneDepthMap.SampleLevel(Sampler,PP,0);    float g21 = (CC)*0.01f;
		CC = xSceneDepthMap.SampleLevel(Sampler,(PP+ox),0); float g22 = (CC)*0.01f;
		float K00 = -1;
		float K01 = -2;
		float K02 = -1;
		float K10 = 0;
		float K11 = 0;
		float K12 = 0;
		float K20 = 1;
		float K21 = 2;
		float K22 = 1;
		float sx = 0;
		float sy = 0;
		sx += g00 * K00;
		sx += g01 * K01;
		sx += g02 * K02;
		sx += g10 * K10;
		sx += g11 * K11;
		sx += g12 * K12;
		sx += g20 * K20;
		sx += g21 * K21;
		sx += g22 * K22; 
		sy += g00 * K00;
		sy += g01 * K10;
		sy += g02 * K20;
		sy += g10 * K01;
		sy += g11 * K11;
		sy += g12 * K21;
		sy += g20 * K02;
		sy += g21 * K12;
		sy += g22 * K22; 
		float dist = sqrt(sx*sx+sy*sy);
		if (dist>Threshold) { result = 0.0f; }

	}
	
	return result;
}
inline float edgeValueNormal(float2 texCo, float2 texDim, float Thickness, float Threshold)
{
	float2 screen = float2(texDim.x,texDim.y)/Thickness;
	float3 baseNor = xNormalMap.Sample(Sampler,texCo).rgb;
	float4 sum = float4(0,0,0,0);
	sum.x = abs(dot(baseNor,xNormalMap.Sample(Sampler,texCo+float2(-1,-1)/screen).xyz));
	if(sum.x){
		sum.y = abs(dot(baseNor,xNormalMap.Sample(Sampler,texCo+float2(1,-1)/screen).xyz));
		sum.z = abs(dot(baseNor,xNormalMap.Sample(Sampler,texCo+float2(-1,1)/screen).xyz));
		sum.w = abs(dot(baseNor,xNormalMap.Sample(Sampler,texCo+float2(1,1)/screen).xyz));
		return step(Threshold.xxxx,sum).x;
	}
	return 1;
}

float4 main(VertextoPixel PSIn) : SV_TARGET
{
	float4 color = float4(0,0,0,1);
	float numSampling = 0.0f;

	color += xTexture.Sample(Sampler,PSIn.tex);
	numSampling++;

	/*float2 depthMapSize;
	xSceneDepthMap.GetDimensions(depthMapSize.x,depthMapSize.y);
	color = xSceneDepthMap.Load(int3(depthMapSize.xy*PSIn.tex,0));*/
	
	color.rgb*=edgeValueDepth(PSIn.tex,xDimension.xy,OUTLINEWIDTHDEPTH,OUTLINETHRESHOLDDEPTH);
	//color.rgb*=edgeValueNormal(PSIn.tex,xDimension.xy,OUTLINEWIDTHNORMAL,OUTLINETHRESHOLDNORMAL);

	return color/numSampling;
}