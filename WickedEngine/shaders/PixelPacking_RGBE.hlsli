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

#ifndef __PIXEL_PACKING_RGBE_HLSLI__
#define __PIXEL_PACKING_RGBE_HLSLI__

#include "ColorSpaceUtility.hlsli"

// RGBE, aka R9G9B9E5_SHAREDEXP, is an unsigned float HDR pixel format where red, green,
// and blue all share the same exponent.  The color channels store a 9-bit value ranging
// from [0/512, 511/512] which multiplies by 2^Exp and Exp ranges from [-15, 16].
// Floating point specials are not encoded.
uint PackRGBE(float3 rgb)
{
    // To determine the shared exponent, we must clamp the channels to an expressible range
    const float kMaxVal = asfloat(0x477F8000); // 1.FF x 2^+15
    const float kMinVal = asfloat(0x37800000); // 1.00 x 2^-16

    // Non-negative and <= kMaxVal
    rgb = clamp(rgb, 0, kMaxVal);

    // From the maximum channel we will determine the exponent.  We clamp to a min value
    // so that the exponent is within the valid 5-bit range.
    float MaxChannel = max(max(kMinVal, rgb.r), max(rgb.g, rgb.b));

    // 'Bias' has to have the biggest exponent plus 15 (and nothing in the mantissa).  When
    // added to the three channels, it shifts the explicit '1' and the 8 most significant
    // mantissa bits into the low 9 bits.  IEEE rules of float addition will round rather
    // than truncate the discarded bits.  Channels with smaller natural exponents will be
    // shifted further to the right (discarding more bits).
    float Bias = asfloat((asuint(MaxChannel) + 0x07804000) & 0x7F800000);

    // Shift bits into the right places
    uint3 RGB = asuint(rgb + Bias);
    uint E = (asuint(Bias) << 4) + 0x10000000;
    return E | RGB.b << 18 | RGB.g << 9 | (RGB.r & 0x1FF);
}

float3 UnpackRGBE(uint p)
{
    float3 rgb = uint3(p, p >> 9, p >> 18) & 0x1FF;
    return ldexp(rgb, (int)(p >> 27) - 24);
}

// This non-standard variant applies a non-linear ramp to the mantissa to get better precision
// with bright and saturated colors.  These colors tend to have one or two channels that prop
// up the shared exponent, leaving little to no information in the dark channels.
uint PackRGBE_sqrt(float3 rgb)
{
    // To determine the shared exponent, we must clamp the channels to an expressible range
    const float kMaxVal = asfloat(0x477FFFFF); // 1.FFFFFF x 2^+15
    const float kMinVal = asfloat(0x37800000); // 1.000000 x 2^-16

    rgb = clamp(rgb, 0, kMaxVal);

    float MaxChannel = max(max(kMinVal, rgb.r), max(rgb.g, rgb.b));

    // Scaling the maximum channel puts it into the range [0, 1).  It does this by negating
    // and subtracting one from the max exponent.
    float Scale = asfloat((0x7EFFFFFF - asuint(MaxChannel)) & 0x7F800000);
    uint3 RGB = sqrt(rgb * Scale) * 511.0 + 0.5;
    uint E = (0x47000000 - asuint(Scale)) << 4;
    return E | RGB.b << 18 | RGB.g << 9 | RGB.r;
}

float3 UnpackRGBE_sqrt(uint p)
{
    float3 rgb = (uint3(p, p >> 9, p >> 18) & 0x1FF) / 511.0;
    return ldexp(rgb * rgb, (int)(p >> 27) - 15);
}

#endif // __PIXEL_PACKING_RGBE_HLSLI__
