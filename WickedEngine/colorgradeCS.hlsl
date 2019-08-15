#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);
TEXTURE2D(lookuptable, float4, TEXSLOT_ONDEMAND1);

RWTEXTURE2D(output, unorm float4, 0);

float4 sampleAs3DTexture(in float3 uv, in float width) 
{
    float sliceSize = 1.0 / width;              // space of 1 slice
    float slicePixelSize = sliceSize / width;           // space of 1 pixel
    float sliceInnerSize = slicePixelSize * (width - 1.0);  // space of width pixels
    float zSlice0 = min(floor(uv.z * width), width - 1.0);
    float zSlice1 = min(zSlice0 + 1.0, width - 1.0);
    float xOffset = slicePixelSize * 0.5 + uv.x * sliceInnerSize;
    float s0 = xOffset + (zSlice0 * sliceSize);
    float s1 = xOffset + (zSlice1 * sliceSize);
    float4 slice0Color = lookuptable.SampleLevel(sampler_linear_clamp, float2(s0, uv.y),0);
    float4 slice1Color = lookuptable.SampleLevel(sampler_linear_clamp, float2(s1, uv.y),0);
    float zOffset = (uv.z * width) % 1.0;
    float4 result = lerp(slice0Color, slice1Color, zOffset);
    return result;
}

[numthreads(POSTPROCESS_BLOCKSIZE, POSTPROCESS_BLOCKSIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	const float2 uv = (DTid.xy + 0.5f) * xPPResolution_rcp;

	float4 color = input.SampleLevel(sampler_linear_clamp, uv, 0);
	
	float2 dim;
	lookuptable.GetDimensions(dim.x, dim.y);
	output[DTid.xy] = sampleAs3DTexture(color.rgb, dim.y);
}
