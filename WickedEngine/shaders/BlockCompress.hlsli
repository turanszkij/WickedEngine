//--------------------------------------------------------------------------------------
// BlockCompress.hlsli
//
// Helper functions for block compression
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------------------

#define BlockCompressRS \
    "RootFlags ( DENY_VERTEX_SHADER_ROOT_ACCESS |" \
    "            DENY_DOMAIN_SHADER_ROOT_ACCESS |" \
    "            DENY_GEOMETRY_SHADER_ROOT_ACCESS |" \
    "            DENY_HULL_SHADER_ROOT_ACCESS )," \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(SRV(t0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u1, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u2, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u3, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u4, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "StaticSampler(s0, " \
    "              filter = FILTER_MIN_MAG_MIP_POINT," \
    "              addressU = TEXTURE_ADDRESS_CLAMP," \
    "              addressV = TEXTURE_ADDRESS_CLAMP," \
    "              addressW = TEXTURE_ADDRESS_CLAMP," \
    "              visibility = SHADER_VISIBILITY_ALL)"

#define COMPRESS_ONE_MIP_THREADGROUP_WIDTH 8
#define COMPRESS_TWO_MIPS_THREADGROUP_WIDTH 16

#define MIP1_BLOCKS_PER_ROW 8

// Constant buffer for block compression shaders
cbuffer BlockCompressCB : register(b0)
{
    float g_oneOverTextureWidth;
}

// CUSTOMBUILD : warning X4714: sum of temp registers and indexable temp registers times 256 threads exceeds the recommended total 16384.  Performance may be reduced
//  This warning shows up in Debug mode due to the complexity of the unoptimized shaders, but it's harmless aside from the fact that the shaders will be slow in Debug
#pragma warning(disable: 4714)

//--------------------------------------------------------------------------------------
// Name: ColorTo565
// Desc: Pack a 3-component color into a uint
//--------------------------------------------------------------------------------------
uint ColorTo565(float3 color)
{
    uint3 rgb = round(color * float3(31.0f, 63.0f, 31.0f));
    return (rgb.r << 11) | (rgb.g << 5) | rgb.b;
}


//--------------------------------------------------------------------------------------
// Name: TexelToUV
// Desc: Convert from a texel to the UV coordinates used in a Gather call
//--------------------------------------------------------------------------------------
float2 TexelToUV(float2 texel, float oneOverTextureWidth)
{
    // We Gather from the bottom-right corner of the texel
    return (texel + 1.0f) * oneOverTextureWidth;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGB
// Desc: Load the 16 RGB texels that form a block
//--------------------------------------------------------------------------------------
void LoadTexelsRGB(Texture2D tex, SamplerState samp, float oneOverTextureWidth, uint2 threadIDWithinDispatch, out float3 block[16])
{
    float2 uv = TexelToUV(float2(threadIDWithinDispatch * 4), oneOverTextureWidth);

    float4 red = tex.GatherRed(samp, uv, int2(0, 0));
    float4 green = tex.GatherGreen(samp, uv, int2(0, 0));
    float4 blue = tex.GatherBlue(samp, uv, int2(0, 0));
    block[0] = float3(red[3], green[3], blue[3]);
    block[1] = float3(red[2], green[2], blue[2]);
    block[4] = float3(red[0], green[0], blue[0]);
    block[5] = float3(red[1], green[1], blue[1]);

    red = tex.GatherRed(samp, uv, int2(2, 0));
    green = tex.GatherGreen(samp, uv, int2(2, 0));
    blue = tex.GatherBlue(samp, uv, int2(2, 0));
    block[2] = float3(red[3], green[3], blue[3]);
    block[3] = float3(red[2], green[2], blue[2]);
    block[6] = float3(red[0], green[0], blue[0]);
    block[7] = float3(red[1], green[1], blue[1]);

    red = tex.GatherRed(samp, uv, int2(0, 2));
    green = tex.GatherGreen(samp, uv, int2(0, 2));
    blue = tex.GatherBlue(samp, uv, int2(0, 2));
    block[8] = float3(red[3], green[3], blue[3]);
    block[9] = float3(red[2], green[2], blue[2]);
    block[12] = float3(red[0], green[0], blue[0]);
    block[13] = float3(red[1], green[1], blue[1]);

    red = tex.GatherRed(samp, uv, int2(2, 2));
    green = tex.GatherGreen(samp, uv, int2(2, 2));
    blue = tex.GatherBlue(samp, uv, int2(2, 2));
    block[10] = float3(red[3], green[3], blue[3]);
    block[11] = float3(red[2], green[2], blue[2]);
    block[14] = float3(red[0], green[0], blue[0]);
    block[15] = float3(red[1], green[1], blue[1]);
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBBias
// Desc: Load the 16 RGB texels that form a block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsRGBBias(Texture2D tex, SamplerState samp, float oneOverTextureSize, uint2 threadIDWithinDispatch, uint mipBias, out float3 block[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample can clamp
    float2 location = float2(threadIDWithinDispatch * 4) * oneOverTextureSize;
    block[0] = tex.SampleLevel(samp, location, mipBias, int2(0, 0)).rgb;
    block[1] = tex.SampleLevel(samp, location, mipBias, int2(1, 0)).rgb;
    block[2] = tex.SampleLevel(samp, location, mipBias, int2(2, 0)).rgb;
    block[3] = tex.SampleLevel(samp, location, mipBias, int2(3, 0)).rgb;
    block[4] = tex.SampleLevel(samp, location, mipBias, int2(0, 1)).rgb;
    block[5] = tex.SampleLevel(samp, location, mipBias, int2(1, 1)).rgb;
    block[6] = tex.SampleLevel(samp, location, mipBias, int2(2, 1)).rgb;
    block[7] = tex.SampleLevel(samp, location, mipBias, int2(3, 1)).rgb;
    block[8] = tex.SampleLevel(samp, location, mipBias, int2(0, 2)).rgb;
    block[9] = tex.SampleLevel(samp, location, mipBias, int2(1, 2)).rgb;
    block[10] = tex.SampleLevel(samp, location, mipBias, int2(2, 2)).rgb;
    block[11] = tex.SampleLevel(samp, location, mipBias, int2(3, 2)).rgb;
    block[12] = tex.SampleLevel(samp, location, mipBias, int2(0, 3)).rgb;
    block[13] = tex.SampleLevel(samp, location, mipBias, int2(1, 3)).rgb;
    block[14] = tex.SampleLevel(samp, location, mipBias, int2(2, 3)).rgb;
    block[15] = tex.SampleLevel(samp, location, mipBias, int2(3, 3)).rgb;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBA
// Desc: Load the 16 RGBA texels that form a block
//--------------------------------------------------------------------------------------
void LoadTexelsRGBA(Texture2D tex, uint2 threadIDWithinDispatch, out float3 blockRGB[16], out float blockA[16])
{
    float4 rgba;
    int3 location = int3(threadIDWithinDispatch * 4, 0);
    rgba = tex.Load(location, int2(0, 0)); blockRGB[0] = rgba.rgb; blockA[0] = rgba.a;
    rgba = tex.Load(location, int2(1, 0)); blockRGB[1] = rgba.rgb; blockA[1] = rgba.a;
    rgba = tex.Load(location, int2(2, 0)); blockRGB[2] = rgba.rgb; blockA[2] = rgba.a;
    rgba = tex.Load(location, int2(3, 0)); blockRGB[3] = rgba.rgb; blockA[3] = rgba.a;
    rgba = tex.Load(location, int2(0, 1)); blockRGB[4] = rgba.rgb; blockA[4] = rgba.a;
    rgba = tex.Load(location, int2(1, 1)); blockRGB[5] = rgba.rgb; blockA[5] = rgba.a;
    rgba = tex.Load(location, int2(2, 1)); blockRGB[6] = rgba.rgb; blockA[6] = rgba.a;
    rgba = tex.Load(location, int2(3, 1)); blockRGB[7] = rgba.rgb; blockA[7] = rgba.a;
    rgba = tex.Load(location, int2(0, 2)); blockRGB[8] = rgba.rgb; blockA[8] = rgba.a;
    rgba = tex.Load(location, int2(1, 2)); blockRGB[9] = rgba.rgb; blockA[9] = rgba.a;
    rgba = tex.Load(location, int2(2, 2)); blockRGB[10] = rgba.rgb; blockA[10] = rgba.a;
    rgba = tex.Load(location, int2(3, 2)); blockRGB[11] = rgba.rgb; blockA[11] = rgba.a;
    rgba = tex.Load(location, int2(0, 3)); blockRGB[12] = rgba.rgb; blockA[12] = rgba.a;
    rgba = tex.Load(location, int2(1, 3)); blockRGB[13] = rgba.rgb; blockA[13] = rgba.a;
    rgba = tex.Load(location, int2(2, 3)); blockRGB[14] = rgba.rgb; blockA[14] = rgba.a;
    rgba = tex.Load(location, int2(3, 3)); blockRGB[15] = rgba.rgb; blockA[15] = rgba.a;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsRGBABias
// Desc: Load the 16 RGBA texels that form a block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsRGBABias(Texture2D tex, SamplerState samp, float oneOverTextureSize, uint2 threadIDWithinDispatch, uint mipBias, out float3 blockRGB[16], out float blockA[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample will clamp
    float4 rgba;
    float2 location = float2(threadIDWithinDispatch * 4) * oneOverTextureSize;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 0)); blockRGB[0] = rgba.rgb; blockA[0] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 0)); blockRGB[1] = rgba.rgb; blockA[1] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 0)); blockRGB[2] = rgba.rgb; blockA[2] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 0)); blockRGB[3] = rgba.rgb; blockA[3] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 1)); blockRGB[4] = rgba.rgb; blockA[4] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 1)); blockRGB[5] = rgba.rgb; blockA[5] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 1)); blockRGB[6] = rgba.rgb; blockA[6] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 1)); blockRGB[7] = rgba.rgb; blockA[7] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 2)); blockRGB[8] = rgba.rgb; blockA[8] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 2)); blockRGB[9] = rgba.rgb; blockA[9] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 2)); blockRGB[10] = rgba.rgb; blockA[10] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 2)); blockRGB[11] = rgba.rgb; blockA[11] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 3)); blockRGB[12] = rgba.rgb; blockA[12] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 3)); blockRGB[13] = rgba.rgb; blockA[13] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 3)); blockRGB[14] = rgba.rgb; blockA[14] = rgba.a;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 3)); blockRGB[15] = rgba.rgb; blockA[15] = rgba.a;
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsUV
// Desc: Load the 16 UV texels that form a block
//--------------------------------------------------------------------------------------
void LoadTexelsUV(Texture2D tex, SamplerState samp, float oneOverTextureWidth, uint2 threadIDWithinDispatch, out float blockU[16], out float blockV[16])
{
    float2 uv = TexelToUV(float2(threadIDWithinDispatch * 4), oneOverTextureWidth);

    float4 red = tex.GatherRed(samp, uv, int2(0, 0));
    float4 green = tex.GatherGreen(samp, uv, int2(0, 0));
    blockU[0] = red[3]; blockV[0] = green[3];
    blockU[1] = red[2]; blockV[1] = green[2];
    blockU[4] = red[0]; blockV[4] = green[0];
    blockU[5] = red[1]; blockV[5] = green[1];

    red = tex.GatherRed(samp, uv, int2(2, 0));
    green = tex.GatherGreen(samp, uv, int2(2, 0));
    blockU[2] = red[3]; blockV[2] = green[3];
    blockU[3] = red[2]; blockV[3] = green[2];
    blockU[6] = red[0]; blockV[6] = green[0];
    blockU[7] = red[1]; blockV[7] = green[1];

    red = tex.GatherRed(samp, uv, int2(0, 2));
    green = tex.GatherGreen(samp, uv, int2(0, 2));
    blockU[8] = red[3]; blockV[8] = green[3];
    blockU[9] = red[2]; blockV[9] = green[2];
    blockU[12] = red[0]; blockV[12] = green[0];
    blockU[13] = red[1]; blockV[13] = green[1];

    red = tex.GatherRed(samp, uv, int2(2, 2));
    green = tex.GatherGreen(samp, uv, int2(2, 2));
    blockU[10] = red[3]; blockV[10] = green[3];
    blockU[11] = red[2]; blockV[11] = green[2];
    blockU[14] = red[0]; blockV[14] = green[0];
    blockU[15] = red[1]; blockV[15] = green[1];
}


//--------------------------------------------------------------------------------------
// Name: LoadTexelsUVBias
// Desc: Load the 16 UV texels that form a block, with a mip bias
//--------------------------------------------------------------------------------------
void LoadTexelsUVBias(Texture2D tex, SamplerState samp, float oneOverTextureSize, uint2 threadIDWithinDispatch, uint mipBias, out float blockU[16], out float blockV[16])
{
    // We need to use Sample rather than Gather/Load for the Bias functions, because low mips will read outside
    //  the texture boundary. When reading outside the boundary, Gather/Load return 0, but Sample will clamp
    float4 rgba;
    float2 location = float2(threadIDWithinDispatch * 4) * oneOverTextureSize;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 0)); blockU[0] = rgba.r; blockV[0] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 0)); blockU[1] = rgba.r; blockV[1] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 0)); blockU[2] = rgba.r; blockV[2] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 0)); blockU[3] = rgba.r; blockV[3] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 1)); blockU[4] = rgba.r; blockV[4] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 1)); blockU[5] = rgba.r; blockV[5] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 1)); blockU[6] = rgba.r; blockV[6] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 1)); blockU[7] = rgba.r; blockV[7] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 2)); blockU[8] = rgba.r; blockV[8] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 2)); blockU[9] = rgba.r; blockV[9] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 2)); blockU[10] = rgba.r; blockV[10] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 2)); blockU[11] = rgba.r; blockV[11] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(0, 3)); blockU[12] = rgba.r; blockV[12] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(1, 3)); blockU[13] = rgba.r; blockV[13] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(2, 3)); blockU[14] = rgba.r; blockV[14] = rgba.g;
    rgba = tex.SampleLevel(samp, location, mipBias, int2(3, 3)); blockU[15] = rgba.r; blockV[15] = rgba.g;
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxChannel
// Desc: Get the min and max of a single channel
//--------------------------------------------------------------------------------------
void GetMinMaxChannel(float block[16], out float minC, out float maxC)
{
    minC = block[0];
    maxC = block[0];

    for (int i = 1; i < 16; ++i)
    {
        minC = min(minC, block[i]);
        maxC = max(maxC, block[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxUV
// Desc: Get the min and max of two channels (UV)
//--------------------------------------------------------------------------------------
void GetMinMaxUV(float blockU[16], float blockV[16], out float minU, out float maxU, out float minV, out float maxV)
{
    minU = blockU[0];
    maxU = blockU[0];
    minV = blockV[0];
    maxV = blockV[0];

    for (int i = 1; i < 16; ++i)
    {
        minU = min(minU, blockU[i]);
        maxU = max(maxU, blockU[i]);
        minV = min(minV, blockV[i]);
        maxV = max(maxV, blockV[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: GetMinMaxRGB
// Desc: Get the min and max of three channels (RGB)
//--------------------------------------------------------------------------------------
void GetMinMaxRGB(float3 colorBlock[16], out float3 minColor, out float3 maxColor)
{
    minColor = colorBlock[0];
    maxColor = colorBlock[0];

    for (int i = 1; i < 16; ++i)
    {
        minColor = min(minColor, colorBlock[i]);
        maxColor = max(maxColor, colorBlock[i]);
    }
}


//--------------------------------------------------------------------------------------
// Name: InsetMinMaxRGB
// Desc: Slightly inset the min and max color values to reduce RMS error.
//      This is recommended by van Waveren & Castano, "Real-Time YCoCg-DXT Compression"
//      http://www.nvidia.com/object/real-time-ycocg-dxt-compression.html
//--------------------------------------------------------------------------------------
void InsetMinMaxRGB(inout float3 minColor, inout float3 maxColor, float colorScale)
{
    // Since we have four points, (1/16) * (max-min) will give us half the distance between
    //  two points on the line in color space
    float3 offset = (1.0f / 16.0f) * (maxColor - minColor);

    // After applying the offset, we want to round up or down to the next integral color value (0 to 255)
    colorScale *= 255.0f;
    maxColor = ceil((maxColor - offset) * colorScale) / colorScale;
    minColor = floor((minColor + offset) * colorScale) / colorScale;
}


//--------------------------------------------------------------------------------------
// Name: GetIndicesRGB
// Desc: Calculate the BC block indices for each color in the block
//--------------------------------------------------------------------------------------
uint GetIndicesRGB(float3 block[16], float3 minColor, float3 maxColor)
{
    uint indices = 0;

    // For each input color, we need to select between one of the following output colors:
    //  0: maxColor
    //  1: (2/3)*maxColor + (1/3)*minColor
    //  2: (1/3)*maxColor + (2/3)*minColor
    //  3: minColor  
    //
    // We essentially just project (block[i] - maxColor) onto (minColor - maxColor), but we pull out
    //  a few constant terms.
    float3 diag = minColor - maxColor;
    float stepInc = 3.0f / dot(diag, diag); // Scale up by 3, because our indices are between 0 and 3
    diag *= stepInc;
    float c = stepInc * (dot(maxColor, maxColor) - dot(maxColor, minColor));

    for (int i = 15; i >= 0; --i)
    {
        // Compute the index for this block element
        uint index = round(dot(block[i], diag) + c);

        // Now we need to convert our index into the somewhat unintuivive BC1 indexing scheme:
        //  0: maxColor
        //  1: minColor
        //  2: (2/3)*maxColor + (1/3)*minColor
        //  3: (1/3)*maxColor + (2/3)*minColor
        //
        // The mapping is:
        //  0 -> 0
        //  1 -> 2
        //  2 -> 3
        //  3 -> 1
        //
        // We can perform this mapping using bitwise operations, which is faster
        //  than predication or branching as long as it doesn't increase our register
        //  count too much. The mapping in binary looks like:
        //  00 -> 00
        //  01 -> 10
        //  10 -> 11
        //  11 -> 01
        //
        // Splitting it up by bit, the output looks like:
        //  bit1_out = bit0_in XOR bit1_in
        //  bit0_out = bit1_in 
        uint bit0_in = index & 1;
        uint bit1_in = index >> 1;
        indices |= ((bit0_in^bit1_in) << 1) | bit1_in;

        if (i != 0)
        {
            indices <<= 2;
        }
    }

    return indices;
}


//--------------------------------------------------------------------------------------
// Name: GetIndicesAlpha
// Desc: Calculate the BC block indices for an alpha channel
//--------------------------------------------------------------------------------------
void GetIndicesAlpha(float block[16], float minA, float maxA, inout uint2 _packed)
{
    float d = minA - maxA;
    float stepInc = 7.0f / d;

    // Both _packed.x and _packed.y contain index values, so we need two loops

    uint index = 0;
	uint _shift = 16;
	int i = 0;
    for (i = 0; i < 6; ++i)
    {
        // For each input alpha value, we need to select between one of eight output values
        //  0: maxA
        //  1: (6/7)*maxA + (1/7)*minA
        //  ...
        //  6: (1/7)*maxA + (6/3)*minA
        //  7: minA  
        index = round(stepInc * (block[i] - maxA));

        // Now we need to convert our index into the BC indexing scheme:
        //  0: maxA
        //  1: minA
        //  2: (6/7)*maxA + (1/7)*minA
        //  ...
        //  7: (1/7)*maxA + (6/3)*minA
        index += (index > 0) - (7 * (index == 7));

		_packed.x |= (index << _shift);
		_shift += 3;
	}

    // The 6th index straddles the two uints
	_packed.y |= (index >> 1);

	_shift = 2;
    for (i = 6; i < 16; ++i)
    {
        index = round((block[i] - maxA) * stepInc);
        index += (index > 0) - (7 * (index == 7));

		_packed.y |= (index << _shift);
		_shift += 3;
	}
}


//--------------------------------------------------------------------------------------
// Name: CompressBC1Block
// Desc: Compress a BC1 block. colorScale is a scale value to be applied to the input 
//          colors; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, colorScale is always 1.0
//--------------------------------------------------------------------------------------
uint2 CompressBC1Block(float3 block[16], float colorScale = 1.0f)
{
    float3 minColor, maxColor;
    GetMinMaxRGB(block, minColor, maxColor);

    // Inset the min and max values
    InsetMinMaxRGB(minColor, maxColor, colorScale);

    // Pack our colors into uints
    uint minColor565 = ColorTo565(colorScale * minColor);
    uint maxColor565 = ColorTo565(colorScale * maxColor);

    uint indices = 0;
    if (minColor565 < maxColor565)
    {
        indices = GetIndicesRGB(block, minColor, maxColor);
    }

    return uint2((minColor565 << 16) | maxColor565, indices);
}


//--------------------------------------------------------------------------------------
// Name: CompressBC3Block
// Desc: Compress a BC3 block. valueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, valueScale is always 1.0
//--------------------------------------------------------------------------------------
uint4 CompressBC3Block(float3 blockRGB[16], float blockA[16], float valueScale = 1.0f)
{
    float3 minColor, maxColor;
    float minA, maxA;
    GetMinMaxRGB(blockRGB, minColor, maxColor);
    GetMinMaxChannel(blockA, minA, maxA);

    // Inset the min and max color values. We don't inset the alpha values
    //  because, while it may reduce the RMS error, it has a tendency to turn
    //  fully opaque texels partially transparent, which is probably not desirable.
    InsetMinMaxRGB(minColor, maxColor, valueScale);

    // Pack our colors and alpha values into uints
    uint minColor565 = ColorTo565(valueScale * minColor);
    uint maxColor565 = ColorTo565(valueScale * maxColor);
    uint minAPacked = round(minA * valueScale * 255.0f);
    uint maxAPacked = round(maxA * valueScale * 255.0f);

    uint indices = 0;
    if (minColor565 < maxColor565)
    {
        indices = GetIndicesRGB(blockRGB, minColor, maxColor);
    }

    uint2 outA = uint2((minAPacked << 8) | maxAPacked, 0);
    if (minAPacked < maxAPacked)
    {
        GetIndicesAlpha(blockA, minA, maxA, outA);
    }

    return uint4(outA.x, outA.y, (minColor565 << 16) | maxColor565, indices);
}


//--------------------------------------------------------------------------------------
// Name: CompressBC5Block
// Desc: Compress a BC5 block. valueScale is a scale value to be applied to the input 
//          values; this used as an optimization when compressing two mips at a time.
//          When compressing only a single mip, valueScale is always 1.0
//--------------------------------------------------------------------------------------
uint4 CompressBC5Block(float blockU[16], float blockV[16], float valueScale = 1.0f)
{
    float minU, maxU, minV, maxV;
    GetMinMaxUV(blockU, blockV, minU, maxU, minV, maxV);

    // Pack our min and max uv values
    uint minUPacked = round(minU * valueScale * 255.0f);
    uint maxUPacked = round(maxU * valueScale * 255.0f);
    uint minVPacked = round(minV * valueScale * 255.0f);
    uint maxVPacked = round(maxV * valueScale * 255.0f);

    uint2 outU = uint2((minUPacked << 8) | maxUPacked, 0);
    uint2 outV = uint2((minVPacked << 8) | maxVPacked, 0);

    if (minUPacked < maxUPacked)
    {
        GetIndicesAlpha(blockU, minU, maxU, outU);
    }

    if (minVPacked < maxVPacked)
    {
        GetIndicesAlpha(blockV, minV, maxV, outV);
    }

    return uint4(outU.x, outU.y, outV.x, outV.y);
}


//--------------------------------------------------------------------------------------
// Name: CalcTailMipsParams
// Desc: Calculate parameters used in the "compress tail mips" shaders
//--------------------------------------------------------------------------------------
void CalcTailMipsParams(uint2 threadIDWithinDispatch, out float oneOverTextureSize, out uint2 blockID, out uint mipBias)
{
    blockID = threadIDWithinDispatch;
    mipBias = 0;
    oneOverTextureSize = 1;

    // When compressing our tail mips, we only dispatch one 8x8 threadgroup. Different threads
    //  are selected to compress different mip levels based on the position of thr thread in
    //  the threadgroup.
    if (blockID.x < 4)
    {
        if (blockID.y < 4)
        {
            // 16x16 mip
            oneOverTextureSize = 1.0f / 16.0f;
        }
        else
        {
            // 1x1 mip
            mipBias = 4;
            blockID.y -= 4;
        }
    }
    else if (blockID.x < 6)
    {
        // 8x8 mip
        mipBias = 1;
        blockID -= float2(4, 4);
        oneOverTextureSize = 1.0f / 8.0f;
    }
    else if (blockID.x < 7)
    {
        // 4x4 mip
        mipBias = 2;
        blockID -= float2(6, 6);
        oneOverTextureSize = 1.0f / 4.0f;
    }
    else if (blockID.x < 8)
    {
        // 2x2 mip
        mipBias = 3;
        blockID -= float2(7, 7);
        oneOverTextureSize = 1.0f / 2.0f;
    }
}
