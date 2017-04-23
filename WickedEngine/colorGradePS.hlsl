#include "postProcessHF.hlsli"

float4 sampleAs3DTexture(float3 uv, float width) {
    float sliceSize = 1.0 / width;              // space of 1 slice
    float slicePixelSize = sliceSize / width;           // space of 1 pixel
    float sliceInnerSize = slicePixelSize * (width - 1.0);  // space of width pixels
    float zSlice0 = min(floor(uv.z * width), width - 1.0);
    float zSlice1 = min(zSlice0 + 1.0, width - 1.0);
    float xOffset = slicePixelSize * 0.5 + uv.x * sliceInnerSize;
    float s0 = xOffset + (zSlice0 * sliceSize);
    float s1 = xOffset + (zSlice1 * sliceSize);
    float4 slice0Color = xMaskTex.SampleLevel(Sampler, float2(s0, uv.y),0);
    float4 slice1Color = xMaskTex.SampleLevel(Sampler, float2(s1, uv.y),0);
    float zOffset = (uv.z * width) % 1.0;
    float4 result = lerp(slice0Color, slice1Color, zOffset);
    return result;
}

float4 main(VertexToPixelPostProcess PSIn) : SV_TARGET
{
	float4 color = xTexture.SampleLevel(Sampler,PSIn.tex,0);
	
	float2 dim;
	xMaskTex.GetDimensions(dim.x,dim.y);
	return sampleAs3DTexture(color.rgb,dim.y);
}