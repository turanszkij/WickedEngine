//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard 
//

#ifndef __PIXEL_PACKING_R11G11B10_HLSLI__
#define __PIXEL_PACKING_R11G11B10_HLSLI__

#include "ColorSpaceUtility.hlsli"

// The standard 32-bit HDR color format.  Each float has a 5-bit exponent and no sign bit.
uint Pack_R11G11B10_FLOAT( float3 rgb )
{
    // Clamp upper bound so that it doesn't accidentally round up to INF 
    // Exponent=15, Mantissa=1.11111
    rgb = min(rgb, asfloat(0x477C0000));  
    uint r = ((f32tof16(rgb.x) + 8) >> 4) & 0x000007FF;
    uint g = ((f32tof16(rgb.y) + 8) << 7) & 0x003FF800;
    uint b = ((f32tof16(rgb.z) + 16) << 17) & 0xFFC00000;
    return r | g | b;
}

float3 Unpack_R11G11B10_FLOAT( uint rgb )
{
    float r = f16tof32((rgb << 4 ) & 0x7FF0);
    float g = f16tof32((rgb >> 7 ) & 0x7FF0);
    float b = f16tof32((rgb >> 17) & 0x7FE0);
    return float3(r, g, b);
}

// An improvement to float is to store the mantissa in logarithmic form.  This causes a
// smooth and continuous change in precision rather than having jumps in precision every
// time the exponent increases by whole amounts.
uint Pack_R11G11B10_FLOAT_LOG( float3 rgb )
{
    float3 flat_mantissa = asfloat((asuint(rgb) & 0x7FFFFF) | 0x3F800000);
    float3 curved_mantissa = min(log2(flat_mantissa) + 1.0, asfloat(0x3FFFFFFF));
    rgb = asfloat((asuint(rgb) & 0xFF800000) | (asuint(curved_mantissa) & 0x7FFFFF));

    uint r = ((f32tof16(rgb.x) + 8) >>  4) & 0x000007FF;
    uint g = ((f32tof16(rgb.y) + 8) <<  7) & 0x003FF800;
    uint b = ((f32tof16(rgb.z) + 16) << 17) & 0xFFC00000;
    return r | g | b;
}

float3 Unpack_R11G11B10_FLOAT_LOG( uint p )
{
    float3 rgb = f16tof32(uint3(p << 4, p >> 7, p >> 17) & uint3(0x7FF0, 0x7FF0, 0x7FE0));
    float3 curved_mantissa = asfloat((asuint(rgb) & 0x7FFFFF) | 0x3F800000);
    float3 flat_mantissa = exp2(curved_mantissa - 1.0);
    return asfloat((asuint(rgb) & 0xFF800000) | (asuint(flat_mantissa) & 0x7FFFFF));
}

// As an alternative to floating point, we can store the log2 of a value in fixed point notation.
// The 11-bit fields store 5.6 fixed point notation for log2(x) with an exponent bias of 15.  The
// 10-bit field uses 5.5 fixed point.  The disadvantage here is we don't handle underflow.  Instead
// we use the extra two exponent values to extend the range down through two more exponents.
// Range = [2^-16, 2^16)
uint Pack_R11G11B10_FIXED_LOG(float3 rgb)
{
    uint3 p = clamp((log2(rgb) + 16.0) * float3(64, 64, 32) + 0.5, 0.0, float3(2047, 2047, 1023));
    return p.b << 22 | p.g << 11 | p.r;
}

float3 Unpack_R11G11B10_FIXED_LOG(uint p)
{
    return exp2((uint3(p, p >> 11, p >> 21) & uint3(2047, 2047, 2046)) / 64.0 - 16.0);
}

// These next two encodings are great for LDR data.  By knowing that our values are [0.0, 1.0]
// (or [0.0, 2.0), incidentally), we can reduce how many bits we need in the exponent.  We can
// immediately eliminate all postive exponents.  By giving more bits to the mantissa, we can
// improve precision at the expense of range.  The 8E3 format goes one bit further, quadrupling
// mantissa precision but increasing smallest exponent from -14 to -6.  The smallest value of 8E3
// is 2^-14, while the smallest value of 7E4 is 2^-21.  Both are smaller than the smallest 8-bit
// sRGB value, which is close to 2^-12.

// This is like R11G11B10_FLOAT except that it moves one bit from each exponent to each mantissa.
uint Pack_R11G11B10_E4_FLOAT( float3 rgb )
{
    // Clamp to [0.0, 2.0).  The magic number is 1.FFFFF x 2^0.  (We can't represent hex floats in HLSL.)
    // This trick works because clamping your exponent to 0 reduces the number of bits needed by 1.
    rgb = clamp( rgb, 0.0, asfloat(0x3FFFFFFF) );
    uint r = ((f32tof16(rgb.r) + 4) >> 3 ) & 0x000007FF;
    uint g = ((f32tof16(rgb.g) + 4) << 8 ) & 0x003FF800;
    uint b = ((f32tof16(rgb.b) + 8) << 18) & 0xFFC00000;
    return r | g | b;
}

float3 Unpack_R11G11B10_E4_FLOAT( uint rgb )
{
    float r = f16tof32((rgb << 3 ) & 0x3FF8);
    float g = f16tof32((rgb >> 8 ) & 0x3FF8);
    float b = f16tof32((rgb >> 18) & 0x3FF0);
    return float3(r, g, b);
}

// This is like R11G11B10_FLOAT except that it moves two bits from each exponent to each mantissa.
uint Pack_R11G11B10_E3_FLOAT( float3 rgb )
{
    // Clamp to [0.0, 2.0).  Divide by 256 to bias the exponent by -8.  This shifts it down to use one
    // fewer bit while still taking advantage of the denormalization hardware.  In half precision,
    // the exponent of 0 is 0xF.  Dividing by 256 makes the max exponent 0x7--one fewer bit.
    rgb = clamp( rgb, 0.0, asfloat(0x3FFFFFFF) ) / 256.0;
    uint r = ((f32tof16(rgb.r) + 2) >> 2 ) & 0x000007FF;
    uint g = ((f32tof16(rgb.g) + 2) << 9 ) & 0x003FF800;
    uint b = ((f32tof16(rgb.b) + 4) << 19) & 0xFFC00000;
    return r | g | b;
}

float3 Unpack_R11G11B10_E3_FLOAT( uint rgb )
{
    float r = f16tof32((rgb << 2 ) & 0x1FFC);
    float g = f16tof32((rgb >> 9 ) & 0x1FFC);
    float b = f16tof32((rgb >> 19) & 0x1FF8);
    return float3(r, g, b) * 256.0;
}

#endif // __PIXEL_PACKING_R11G11B10_HLSLI__
