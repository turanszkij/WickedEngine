//===============================================================================
// Copyright (c) 2021    Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
//===============================================================================

#ifndef BCN_COMMON_API_H_
#define BCN_COMMON_API_H_

//===================================================================
// NOTE: Do not use these API in production code, subject to changes 
//===================================================================

#ifndef ASPM_GPU
#pragma warning(disable : 4244)
#pragma warning(disable : 4201)
#endif

#include "common_def.h"

#define CMP_MAX_16BITFLOAT  65504.0f
#define CMP_FLT_MAX         3.402823466e+38F
#define BC1ConstructColour(r, g, b)  (((r) << 11) | ((g) << 5) | (b))


#ifdef ASPM_HLSL
#define fabs(x) abs(x)
#endif

CMP_STATIC CGU_FLOAT cmp_fabs(CMP_IN CGU_FLOAT x)
{
    return fabs(x);
}

CMP_STATIC CGU_FLOAT cmp_linearToSrgbf(CMP_IN CGU_FLOAT Color)
{
    if (Color <= 0.0f)
        return (0.0f);
    if (Color >= 1.0f)
        return (1.0f);
    // standard : 0.0031308f
    if (Color <= 0.00313066844250063)
        return (Color * 12.92f);
    return (pow(fabs(Color), 1.0f / 2.4f) * 1.055f - 0.055f);
}

CMP_STATIC CGU_Vec3f cmp_linearToSrgb(CMP_IN CGU_Vec3f Color)
{
    Color.x = cmp_linearToSrgbf(Color.x);
    Color.y = cmp_linearToSrgbf(Color.y);
    Color.z = cmp_linearToSrgbf(Color.z);
    return Color;
}

CMP_STATIC CGU_FLOAT cmp_srgbToLinearf(CMP_IN CGU_FLOAT Color)
{
    if (Color <= 0.0f)
        return (0.0f);
    if (Color >= 1.0f)
        return (1.0f);
    // standard 0.04045f
    if (Color <= 0.0404482362771082)
        return (Color / 12.92f);
    return pow((Color + 0.055f) / 1.055f, 2.4f);
}

CMP_STATIC CGU_Vec3f cmp_srgbToLinear(CMP_IN CGU_Vec3f Color)
{
    Color.x = cmp_srgbToLinearf(Color.x);
    Color.y = cmp_srgbToLinearf(Color.y);
    Color.z = cmp_srgbToLinearf(Color.z);
    return Color;
}

CMP_STATIC CGU_Vec3f cmp_565ToLinear(CMP_IN CGU_UINT32 n565)
{
    CGU_UINT32 r0;
    CGU_UINT32 g0;
    CGU_UINT32 b0;

    r0 = ((n565 & 0xf800) >> 8);
    g0 = ((n565 & 0x07e0) >> 3);
    b0 = ((n565 & 0x001f) << 3);

    // Apply the lower bit replication to give full dynamic range (5,6,5)
    r0 += (r0 >> 5);
    g0 += (g0 >> 6);
    b0 += (b0 >> 5);

    CGU_Vec3f LinearColor;
    LinearColor.x = (CGU_FLOAT)r0;
    LinearColor.y = (CGU_FLOAT)g0;
    LinearColor.z = (CGU_FLOAT)b0;

    return LinearColor;
}

CMP_STATIC CGU_UINT32 cmp_get2Bit32(CMP_IN CGU_UINT32 value, CMP_IN CGU_UINT32 indexPos)
{
    return (value >> (indexPos * 2)) & 0x3;
}

CMP_STATIC CGU_UINT32 cmp_set2Bit32(CMP_IN CGU_UINT32 value, CMP_IN CGU_UINT32 indexPos)
{
    return ((value & 0x3) << (indexPos * 2));
}

CMP_STATIC CGU_UINT32 cmp_constructColor(CMP_IN CGU_UINT32 R, CMP_IN CGU_UINT32 G, CMP_IN CGU_UINT32 B)
{
    return (((R & 0x000000F8) << 8) | ((G & 0x000000FC) << 3) | ((B & 0x000000F8) >> 3));
}

CMP_STATIC CGU_FLOAT cmp_clampf(CMP_IN CGU_FLOAT v, CMP_IN CGU_FLOAT a, CMP_IN CGU_FLOAT b)
{
    if (v < a)
        return a;
    else if (v > b)
        return b;
    return v;
}

CMP_STATIC CGU_Vec3f cmp_clampVec3f(CMP_IN CGU_Vec3f value, CMP_IN CGU_FLOAT minValue, CMP_IN CGU_FLOAT maxValue)
{
#ifdef ASPM_GPU
    return clamp(value, minValue, maxValue);
#else
    CGU_Vec3f revalue;
    revalue.x = cmp_clampf(value.x, minValue, maxValue);
    revalue.y = cmp_clampf(value.y, minValue, maxValue);
    revalue.z = cmp_clampf(value.z, minValue, maxValue);
    return revalue;
#endif
}

CMP_STATIC CGU_Vec3f cmp_saturate(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_HLSL
    return saturate(value);
#else
    return cmp_clampVec3f(value, 0.0f, 1.0f);
#endif
}

static CGU_Vec3f cmp_powVec3f(CGU_Vec3f color, CGU_FLOAT ex)
{
#ifdef ASPM_GPU
    return pow(color, ex);
#else
    CGU_Vec3f ColorSrgbPower;
    ColorSrgbPower.x = pow(color.x, ex);
    ColorSrgbPower.y = pow(color.y, ex);
    ColorSrgbPower.z = pow(color.z, ex);
    return ColorSrgbPower;
#endif
}

CMP_STATIC CGU_Vec3f cmp_minVec3f(CMP_IN CGU_Vec3f a, CMP_IN CGU_Vec3f b)
{
#ifdef ASPM_HLSL
    return min(a, b);
#endif
    CGU_Vec3f res;
    if (a.x < b.x)
        res.x = a.x;
    else
        res.x = b.x;
    if (a.y < b.y)
        res.y = a.y;
    else
        res.y = b.y;
    if (a.z < b.z)
        res.z = a.z;
    else
        res.z = b.z;
    return res;
}

CMP_STATIC CGU_Vec3f cmp_maxVec3f(CMP_IN CGU_Vec3f a, CMP_IN CGU_Vec3f b)
{
#ifdef ASPM_HLSL
    return max(a, b);
#endif
    CGU_Vec3f res;
    if (a.x > b.x)
        res.x = a.x;
    else
        res.x = b.x;
    if (a.y > b.y)
        res.y = a.y;
    else
        res.y = b.y;
    if (a.z > b.z)
        res.z = a.z;
    else
        res.z = b.z;
    return res;
}

inline CGU_Vec3f cmp_min3f(CMP_IN CGU_Vec3f value1, CMP_IN CGU_Vec3f value2)
{
#ifdef ASPM_GPU
    return min(value1, value2);
#else
    CGU_Vec3f res;
    res.x = CMP_MIN(value1.x, value2.x);
    res.y = CMP_MIN(value1.y, value2.y);
    res.z = CMP_MIN(value1.z, value2.z);
    return res;
#endif
}

inline CGU_Vec3f cmp_max3f(CMP_IN CGU_Vec3f value1, CMP_IN CGU_Vec3f value2)
{
#ifdef ASPM_GPU
    return max(value1, value2);
#else
    CGU_Vec3f res;
    res.x = CMP_MAX(value1.x, value2.x);
    res.y = CMP_MAX(value1.y, value2.y);
    res.z = CMP_MAX(value1.z, value2.z);
    return res;
#endif
}

CMP_STATIC CGU_FLOAT cmp_minf(CMP_IN CGU_FLOAT a, CMP_IN CGU_FLOAT b)
{
    return a < b ? a : b;
}

CMP_STATIC CGU_FLOAT cmp_maxf(CMP_IN CGU_FLOAT a, CMP_IN CGU_FLOAT b)
{
    return a > b ? a : b;
}

CMP_STATIC CGU_FLOAT cmp_floor(CMP_IN CGU_FLOAT value)
{
    return floor(value);
}

CMP_STATIC CGU_Vec3f cmp_floorVec3f(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_GPU
    return floor(value);
#else
    CGU_Vec3f revalue;
    revalue.x = floor(value.x);
    revalue.y = floor(value.y);
    revalue.z = floor(value.z);
    return revalue;
#endif
}

#ifndef ASPM_OPENCL

//=======================================================
// COMMON GPU & CPU API
//=======================================================

//======================
// implicit vector cast
//======================
CMP_STATIC CGU_Vec4i cmp_castimp(CGU_Vec4ui v1)
{
#ifdef ASPM_HLSL
    return (v1);
#else
    return (v1.x, v1.y, v1.z, v1.w);
#endif
}

CMP_STATIC CGU_Vec3i cmp_castimp(CGU_Vec3ui v1)
{
#ifdef ASPM_HLSL
    return (v1);
#else
    return (v1.x, v1.y, v1.z);
#endif
}

//======================
// Min / Max
//======================

CMP_STATIC CGU_UINT8 cmp_min8(CMP_IN CGU_UINT8 a, CMP_IN CGU_UINT8 b)
{
    return a < b ? a : b;
}

CMP_STATIC CGU_UINT8 cmp_max8(CMP_IN CGU_UINT8 a, CMP_IN CGU_UINT8 b)
{
    return a > b ? a : b;
}

CMP_STATIC CGU_UINT32 cmp_mini(CMP_IN CGU_UINT32 a, CMP_IN CGU_UINT32 b)
{
    return (a < b) ? a : b;
}

CMP_STATIC CGU_UINT32 cmp_maxi(CMP_IN CGU_UINT32 a, CMP_IN CGU_UINT32 b)
{
    return (a > b) ? a : b;
}

CMP_STATIC CGU_FLOAT cmp_max3(CMP_IN CGU_FLOAT i, CMP_IN CGU_FLOAT j, CMP_IN CGU_FLOAT k)
{
#ifdef ASPM_GLSL
    return max3(i, j, k);
#else
    CGU_FLOAT max = i;

    if (max < j)
        max = j;

    if (max < k)
        max = k;

    return (max);
#endif
}


CMP_STATIC CGU_Vec4ui cmp_minVec4ui(CMP_IN CGU_Vec4ui a, CMP_IN CGU_Vec4ui b)
{
    //#ifdef ASPM_HLSL
    //    return min(a, b);
    //#endif
    //#ifndef ASPM_GPU
    CGU_Vec4ui res;
    if (a.x < b.x)
        res.x = a.x;
    else
        res.x = b.x;
    if (a.y < b.y)
        res.y = a.y;
    else
        res.y = b.y;
    if (a.z < b.z)
        res.z = a.z;
    else
        res.z = b.z;
    if (a.w < b.w)
        res.w = a.w;
    else
        res.w = b.w;
    return res;
    //#endif
}

CMP_STATIC CGU_Vec4ui cmp_maxVec4ui(CMP_IN CGU_Vec4ui a, CMP_IN CGU_Vec4ui b)
{
    //#ifdef ASPM_HLSL
    //    return max(a, b);
    //#endif
    //#ifndef ASPM_GPU
    CGU_Vec4ui res;
    if (a.x > b.x)
        res.x = a.x;
    else
        res.x = b.x;
    if (a.y > b.y)
        res.y = a.y;
    else
        res.y = b.y;
    if (a.z > b.z)
        res.z = a.z;
    else
        res.z = b.z;
    if (a.w > b.w)
        res.w = a.w;
    else
        res.w = b.w;
    return res;
    //#endif
}

//======================
// Clamps
//======================

CMP_STATIC CGU_UINT32 cmp_clampui32(CMP_IN CGU_UINT32 v, CMP_IN CGU_UINT32 a, CMP_IN CGU_UINT32 b)
{
    if (v < a)
        return a;
    else if (v > b)
        return b;
    return v;
}


// Test Ref:https://en.wikipedia.org/wiki/Half-precision_floating-point_format
// Half (in Hex)        Float               Comment
// ---------------------------------------------------------------------------
// 0001    (approx) = 0.000000059604645     smallest positive subnormal number
// 03ff    (approx) = 0.000060975552        largest subnormal number
// 0400    (approx) = 0.00006103515625      smallest positive normal number
// 7bff    (approx) = 65504                 largest normal number
// 3bff    (approx) = 0.99951172            largest number less than one
// 3c00    (approx) = 1.00097656            smallest number larger than one
// 3555             = 0.33325195            the rounding of 1/3 to nearest
// c000             = ?2
// 8000             = -0
// 0000             = 0
// 7c00             = infinity
// fc00             = infinity
// Half Float Math

CMP_STATIC CGU_FLOAT HalfToFloat(CGU_UINT32 h)
{
#if defined(ASPM_GPU)
    CGU_FLOAT f = min16float((float)(h));
    return f;
#else
    union FP32
    {
        CGU_UINT32 u;
        CGU_FLOAT  f;
    };

    const FP32 magic      = {(254 - 15) << 23};
    const FP32 was_infnan = {(127 + 16) << 23};

    FP32 o;
    o.u = (h & 0x7fff) << 13;  // exponent/mantissa bits
    o.f *= magic.f;            // exponent adjust
    if (o.f >= was_infnan.f)   // check Inf/NaN
        o.u |= 255 << 23;
    o.u |= (h & 0x8000) << 16;  // sign bit
    return o.f;
#endif
}

// From BC6HEcode.hlsl

CMP_STATIC CGU_FLOAT cmp_half2float1(CGU_UINT32 Value)
{
    CGU_UINT32 Mantissa = (CGU_UINT32)(Value & 0x03FF);

    CGU_UINT32 Exponent;
    if ((Value & 0x7C00) != 0)  // The value is normalized
    {
        Exponent = (CGU_UINT32)((Value >> 10) & 0x1F);
    }
    else if (Mantissa != 0)  // The value is denormalized
    {
        // Normalize the value in the resulting float
        Exponent = 1;

        do
        {
            Exponent--;
            Mantissa <<= 1;
        } while ((Mantissa & 0x0400) == 0);

        Mantissa &= 0x03FF;
    }
    else  // The value is zero
    {
        Exponent = (CGU_UINT32)(-112);
    }

    CGU_UINT32 Result = ((Value & 0x8000) << 16) |  // Sign
                        ((Exponent + 112) << 23) |  // Exponent
                        (Mantissa << 13);           // Mantissa

    return CGU_FLOAT(Result);
}

CMP_STATIC CGU_Vec3f cmp_half2floatVec3(CGU_Vec3ui color_h)
{
    //uint3 sign = color_h & 0x8000;
    //uint3 expo = color_h & 0x7C00;
    //uint3 base = color_h & 0x03FF;
    //return ( expo == 0 ) ? asfloat( ( sign << 16 ) | asuint( float3(base) / 16777216 ) ) //16777216 = 2^24
    //    : asfloat( ( sign << 16 ) | ( ( ( expo + 0x1C000 ) | base ) << 13 ) ); //0x1C000 = 0x1FC00 - 0x3C00

    return CGU_Vec3f(cmp_half2float1(color_h.x), cmp_half2float1(color_h.y), cmp_half2float1(color_h.z));
}

CMP_STATIC CGU_UINT16 FloatToHalf(CGU_FLOAT value)
{
#if defined(ASPM_GPU)
    return 0;
#else
    union FP32
    {
        CGU_UINT16 u;
        float      f;
        struct
        {
            CGU_UINT32 Mantissa : 23;
            CGU_UINT32 Exponent : 8;
            CGU_UINT32 Sign : 1;
        };
    };

    union FP16
    {
        CGU_UINT16 u;
        struct
        {
            CGU_UINT32 Mantissa : 10;
            CGU_UINT32 Exponent : 5;
            CGU_UINT32 Sign : 1;
        };
    };

    FP16 o = {0};
    FP32 f;
    f.f = value;

    // Based on ISPC reference code (with minor modifications)
    if (f.Exponent == 0)  // Signed zero/denormal (which will underflow)
        o.Exponent = 0;
    else if (f.Exponent == 255)  // Inf or NaN (all exponent bits set)
    {
        o.Exponent = 31;
        o.Mantissa = f.Mantissa ? 0x200 : 0;  // NaN->qNaN and Inf->Inf
    }
    else  // Normalized number
    {
        // Exponent unbias the single, then bias the halfp
        int newexp = f.Exponent - 127 + 15;
        if (newexp >= 31)  // Overflow, return signed infinity
            o.Exponent = 31;
        else if (newexp <= 0)  // Underflow
        {
            if ((14 - newexp) <= 24)  // Mantissa might be non-zero
            {
                CGU_UINT32 mant = f.Mantissa | 0x800000;  // Hidden 1 bit
                o.Mantissa      = mant >> (14 - newexp);
                if ((mant >> (13 - newexp)) & 1)  // Check for rounding
                    o.u++;                        // Round, might overflow into exp bit, but this is OK
            }
        }
        else
        {
            o.Exponent = newexp;
            o.Mantissa = f.Mantissa >> 13;
            if (f.Mantissa & 0x1000)  // Check for rounding
                o.u++;                // Round, might overflow to inf, this is OK
        }
    }

    o.Sign = f.Sign;
    return o.u;
#endif
}

CMP_STATIC CGU_UINT32 cmp_float2halfui(CGU_FLOAT f)
{
    CGU_UINT32 Result;

    CGU_UINT32 IValue = CGU_UINT32(f);
    CGU_UINT32 Sign   = (IValue & 0x80000000U) >> 16U;
    IValue            = IValue & 0x7FFFFFFFU;

    if (IValue > 0x47FFEFFFU)
    {
        // The number is too large to be represented as a half.  Saturate to infinity.
        Result = 0x7FFFU;
    }
    else
    {
        if (IValue < 0x38800000U)
        {
            // The number is too small to be represented as a normalized half.
            // Convert it to a denormalized value.
            CGU_UINT32 Shift = 113U - (IValue >> 23U);
            IValue           = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized half.
            IValue += 0xC8000000U;
        }

        Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
    }
    return (Result | Sign);
}

CMP_STATIC CGU_Vec3ui cmp_float2half(CGU_Vec3f endPoint_f)
{
    return CGU_Vec3ui(cmp_float2halfui(endPoint_f.x), cmp_float2halfui(endPoint_f.y), cmp_float2halfui(endPoint_f.z));
}

CMP_STATIC CGU_UINT32 cmp_float2half1(CGU_FLOAT f)
{
    CGU_UINT32 Result;

    CGU_UINT32 IValue = CGU_UINT32(f);  //asuint(f);
    CGU_UINT32 Sign   = (IValue & 0x80000000U) >> 16U;
    IValue            = IValue & 0x7FFFFFFFU;

    if (IValue > 0x47FFEFFFU)
    {
        // The number is too large to be represented as a half.  Saturate to infinity.
        Result = 0x7FFFU;
    }
    else
    {
        if (IValue < 0x38800000U)
        {
            // The number is too small to be represented as a normalized half.
            // Convert it to a denormalized value.
            CGU_UINT32 Shift = 113U - (IValue >> 23U);
            IValue           = (0x800000U | (IValue & 0x7FFFFFU)) >> Shift;
        }
        else
        {
            // Rebias the exponent to represent the value as a normalized half.
            IValue += 0xC8000000U;
        }

        Result = ((IValue + 0x0FFFU + ((IValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
    }
    return (Result | Sign);
}

CMP_STATIC CGU_Vec3ui cmp_float2halfVec3(CGU_Vec3f endPoint_f)
{
    return CGU_Vec3ui(cmp_float2half1(endPoint_f.x), cmp_float2half1(endPoint_f.y), cmp_float2half1(endPoint_f.z));
}

CMP_STATIC CGU_FLOAT cmp_f32tof16(CMP_IN CGU_FLOAT value)
{
#ifdef ASPM_GLSL
    return packHalf2x16(CGU_Vec2f(value.x, 0.0));
#endif
#ifdef ASPM_HLSL
    return f32tof16(value);
#endif
#ifndef ASPM_GPU
    return FloatToHalf(value);
#endif
}

CMP_STATIC CGU_Vec3f cmp_f32tof16(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_GLSL
    return CGU_Vec3f(packHalf2x16(CGU_Vec2f(value.x, 0.0)), packHalf2x16(CGU_Vec2f(value.y, 0.0)), packHalf2x16(CGU_Vec2f(value.z, 0.0)));
#endif
#ifdef ASPM_HLSL
    return f32tof16(value);
#endif
#ifndef ASPM_GPU
    CGU_Vec3f res;
    res.x = FloatToHalf(value.x);
    res.y = FloatToHalf(value.y);
    res.z = FloatToHalf(value.z);
    return res;
#endif
}

CMP_STATIC CGU_FLOAT cmp_f16tof32(CGU_UINT32 value)
{
#ifdef ASPM_GLSL
    return unpackHalf2x16(value).x;
#endif
#ifdef ASPM_HLSL
    return f16tof32(value);
#endif
#ifndef ASPM_GPU
    return HalfToFloat(value);
#endif
}

CMP_STATIC CGU_Vec3f cmp_f16tof32(CGU_Vec3ui value)
{
#ifdef ASPM_GLSL
    return CGU_Vec3f(unpackHalf2x16(value.x).x, unpackHalf2x16(value.y).x, unpackHalf2x16(value.z).x);
#endif
#ifdef ASPM_HLSL
    return f16tof32(value);
#endif
#ifndef ASPM_GPU
    CGU_Vec3f res;
    res.x = HalfToFloat(value.x);
    res.y = HalfToFloat(value.y);
    res.z = HalfToFloat(value.z);
    return res;
#endif
}

CMP_STATIC CGU_Vec3f cmp_f16tof32(CGU_Vec3f value)
{
#ifdef ASPM_GLSL
    return CGU_Vec3f(unpackHalf2x16(value.x).x, unpackHalf2x16(value.y).x, unpackHalf2x16(value.z).x);
#endif
#ifdef ASPM_HLSL
    return f16tof32(value);
#endif
#ifndef ASPM_GPU
    CGU_Vec3f res;
    res.x = HalfToFloat((CGU_UINT32)value.x);
    res.y = HalfToFloat((CGU_UINT32)value.y);
    res.z = HalfToFloat((CGU_UINT32)value.z);
    return res;
#endif
}

CMP_STATIC void cmp_swap(CMP_INOUT CGU_Vec3f CMP_REFINOUT a, CMP_INOUT CGU_Vec3f CMP_REFINOUT b)
{
    CGU_Vec3f tmp = a;
    a             = b;
    b             = tmp;
}

CMP_STATIC void cmp_swap(CMP_INOUT CGU_FLOAT CMP_REFINOUT a, CMP_INOUT CGU_FLOAT CMP_REFINOUT b)
{
    CGU_FLOAT tmp = a;
    a             = b;
    b             = tmp;
}

CMP_STATIC void cmp_swap(CMP_INOUT CGU_Vec3i CMP_REFINOUT lhs, CMP_INOUT CGU_Vec3i CMP_REFINOUT rhs)  // valided with msc code
{
    CGU_Vec3i tmp = lhs;
    lhs           = rhs;
    rhs           = tmp;
}

CMP_STATIC CGU_INT cmp_dotVec2i(CMP_IN CGU_Vec2i value1, CMP_IN CGU_Vec2i value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y);
#endif
}

CMP_STATIC CGU_FLOAT cmp_dotVec3f(CMP_IN CGU_Vec3f value1, CMP_IN CGU_Vec3f value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y) + (value1.z * value2.z);
#endif
}

CMP_STATIC CGU_UINT32 cmp_dotVec3ui(CMP_IN CGU_Vec3ui value1, CMP_IN CGU_Vec3ui value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y) + (value1.z * value2.z);
#endif
}

CMP_STATIC CGU_UINT32 cmp_dotVec4i(CMP_IN CGU_Vec4i value1, CMP_IN CGU_Vec4i value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y) + (value1.z * value2.z) + (value1.w * value2.w);
#endif
}

CMP_STATIC CGU_UINT32 cmp_dotVec4ui(CMP_IN CGU_Vec4ui value1, CMP_IN CGU_Vec4ui value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y) + (value1.z * value2.z) + (value1.w * value2.w);
#endif
}

CMP_STATIC CGU_Vec3f cmp_clampVec3fi(CMP_IN CGU_Vec3f value, CMP_IN CGU_INT minValue, CMP_IN CGU_INT maxValue)
{
#ifdef ASPM_GPU
    return clamp(value, minValue, maxValue);
#else
    CGU_Vec3f revalue;
    revalue.x = cmp_clampf(value.x, (CGU_FLOAT)minValue, (CGU_FLOAT)maxValue);
    revalue.y = cmp_clampf(value.y, (CGU_FLOAT)minValue, (CGU_FLOAT)maxValue);
    revalue.z = cmp_clampf(value.z, (CGU_FLOAT)minValue, (CGU_FLOAT)maxValue);
    return revalue;
#endif
}

CMP_STATIC CGU_Vec4ui cmp_clampVec4ui(CMP_IN CGU_Vec4ui value, CMP_IN CGU_UINT32 minValue, CMP_IN CGU_UINT32 maxValue)
{
#ifdef ASPM_GPU
    return clamp(value, minValue, maxValue);
#else
    CGU_Vec4ui revalue;
    revalue.x = cmp_clampui32(value.x, minValue, maxValue);
    revalue.y = cmp_clampui32(value.y, minValue, maxValue);
    revalue.z = cmp_clampui32(value.z, minValue, maxValue);
    revalue.w = cmp_clampui32(value.w, minValue, maxValue);
    return revalue;
#endif
}

CMP_STATIC CGU_Vec4f cmp_clampVec4f(CMP_IN CGU_Vec4f value, CMP_IN CGU_FLOAT minValue, CMP_IN CGU_FLOAT maxValue)
{
#ifdef ASPM_GPU
    return clamp(value, minValue, maxValue);
#else
    CGU_Vec4f revalue;
    revalue.x = cmp_clampf(value.x, minValue, maxValue);
    revalue.y = cmp_clampf(value.y, minValue, maxValue);
    revalue.z = cmp_clampf(value.z, minValue, maxValue);
    revalue.w = cmp_clampf(value.w, minValue, maxValue);
    return revalue;
#endif
}

CMP_STATIC CGU_Vec3f cmp_clamp3Vec3f(CMP_IN CGU_Vec3f value, CMP_IN CGU_Vec3f minValue, CMP_IN CGU_Vec3f maxValue)
{
#ifdef ASPM_GPU
    return clamp(value, minValue, maxValue);
#else
    CGU_Vec3f revalue;
    revalue.x = cmp_clampf(value.x, minValue.x, maxValue.x);
    revalue.y = cmp_clampf(value.y, minValue.y, maxValue.y);
    revalue.z = cmp_clampf(value.z, minValue.z, maxValue.z);
    return revalue;
#endif
}

CMP_STATIC CGU_Vec3f cmp_exp2(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_GPU
    return exp2(value);
#else
    CGU_Vec3f revalue;
    revalue.x = exp2(value.x);
    revalue.y = exp2(value.y);
    revalue.z = exp2(value.z);
    return revalue;
#endif
}


CMP_STATIC CGU_Vec3f cmp_roundVec3f(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_HLSL
    return round(value);
#endif
#ifndef ASPM_HLSL
    CGU_Vec3f res;
    res.x = round(value.x);
    res.y = round(value.y);
    res.z = round(value.z);
    return res;
#endif
}

CMP_STATIC CGU_Vec3f cmp_log2Vec3f(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_GPU
    return log2(value);
#else
    CGU_Vec3f res;
    res.x = log2(value.x);
    res.y = log2(value.y);
    res.z = log2(value.z);
    return res;
#endif
}

// used in BC1 LowQuality code
CMP_STATIC CGU_FLOAT cmp_saturate(CMP_IN CGU_FLOAT value)
{
#ifdef ASPM_HLSL
    return saturate(value);
#else
    return cmp_clampf(value, 0.0f, 1.0f);
#endif
}

CMP_STATIC CGU_FLOAT cmp_rcp(CMP_IN CGU_FLOAT det)
{
#ifdef ASPM_HLSL
    return rcp(det);
#else
    if (det > 0.0f)
        return (1 / det);
    else
        return 0.0f;
#endif
}

CMP_STATIC CGU_UINT32 cmp_Get4BitIndexPos(CMP_IN CGU_FLOAT indexPos, CMP_IN CGU_FLOAT endPoint0Pos, CMP_IN CGU_FLOAT endPoint1Pos)
{
    CGU_FLOAT r = (indexPos - endPoint0Pos) / (endPoint1Pos - endPoint0Pos);
    return cmp_clampui32(CGU_UINT32(r * 14.93333f + 0.03333f + 0.5f), 0, 15);
}

// Calculate Mean Square Least Error (MSLE) for 2 Vectors
CMP_STATIC CGU_FLOAT cmp_CalcMSLE(CMP_IN CGU_Vec3f a, CMP_IN CGU_Vec3f b)
{
    CGU_Vec3f err = cmp_log2Vec3f((b + 1.0f) / (a + 1.0f));
    err           = err * err;
    return err.x + err.y + err.z;
}



// Compute Endpoints (min/max) bounding box
CMP_STATIC void cmp_GetTexelMinMax(CMP_IN CGU_Vec3f texels[16], CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMin, CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMax)
{
    blockMin = texels[0];
    blockMax = texels[0];
    for (CGU_UINT32 i = 1u; i < 16u; ++i)
    {
        blockMin = cmp_minVec3f(blockMin, texels[i]);
        blockMax = cmp_maxVec3f(blockMax, texels[i]);
    }
}

// Refine Endpoints (min/max) by insetting bounding box in log2 RGB space
CMP_STATIC void cmp_RefineMinMaxAsLog2(CMP_IN CGU_Vec3f texels[16], CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMin, CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMax)
{
    CGU_Vec3f refinedBlockMin = blockMax;
    CGU_Vec3f refinedBlockMax = blockMin;
    for (CGU_UINT32 i = 0u; i < 16u; ++i)
    {
        refinedBlockMin = cmp_minVec3f(refinedBlockMin, texels[i] == blockMin ? refinedBlockMin : texels[i]);
        refinedBlockMax = cmp_maxVec3f(refinedBlockMax, texels[i] == blockMax ? refinedBlockMax : texels[i]);
    }

    CGU_Vec3f logBlockMax = cmp_log2Vec3f(blockMax + 1.0f);
    CGU_Vec3f logBlockMin = cmp_log2Vec3f(blockMin + 1.0f);

    CGU_Vec3f logRefinedBlockMax = cmp_log2Vec3f(refinedBlockMax + 1.0f);
    CGU_Vec3f logRefinedBlockMin = cmp_log2Vec3f(refinedBlockMin + 1.0f);
    CGU_Vec3f logBlockMaxExt     = (logBlockMax - logBlockMin) * (1.0f / 32.0f);

    logBlockMin += cmp_minVec3f(logRefinedBlockMin - logBlockMin, logBlockMaxExt);
    logBlockMax -= cmp_minVec3f(logBlockMax - logRefinedBlockMax, logBlockMaxExt);

    blockMin = cmp_exp2(logBlockMin) - 1.0f;
    blockMax = cmp_exp2(logBlockMax) - 1.0f;
}

// Refine Endpoints (min/max) by Least Squares Optimization
CMP_STATIC void cmp_RefineMinMaxAs16BitLeastSquares(CMP_IN CGU_Vec3f texels[16],
                                                    CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMin,
                                                    CMP_INOUT CGU_Vec3f CMP_REFINOUT blockMax)
{
    CGU_Vec3f blockDir = blockMax - blockMin;
    blockDir           = blockDir / (blockDir.x + blockDir.y + blockDir.z);

    CGU_FLOAT endPoint0Pos = cmp_f32tof16(cmp_dotVec3f(blockMin, blockDir));
    CGU_FLOAT endPoint1Pos = cmp_f32tof16(cmp_dotVec3f(blockMax, blockDir));

    CGU_Vec3f alphaTexelSum = 0.0f;
    CGU_Vec3f betaTexelSum  = 0.0f;
    CGU_FLOAT alphaBetaSum  = 0.0f;
    CGU_FLOAT alphaSqSum    = 0.0f;
    CGU_FLOAT betaSqSum     = 0.0f;

    for (CGU_UINT32 i = 0; i < 16; i++)
    {
        CGU_FLOAT  texelPos   = cmp_f32tof16(cmp_dotVec3f(texels[i], blockDir));
        CGU_UINT32 texelIndex = cmp_Get4BitIndexPos(texelPos, endPoint0Pos, endPoint1Pos);

        CGU_FLOAT beta  = cmp_saturate(texelIndex / 15.0f);
        CGU_FLOAT alpha = 1.0f - beta;

        CGU_Vec3f texelF16;
        texelF16.x = cmp_f32tof16(texels[i].x);
        texelF16.y = cmp_f32tof16(texels[i].y);
        texelF16.z = cmp_f32tof16(texels[i].z);

        alphaTexelSum += texelF16 * alpha;
        betaTexelSum += texelF16 * beta;

        alphaBetaSum += alpha * beta;

        alphaSqSum += alpha * alpha;
        betaSqSum += beta * beta;
    }

    CGU_FLOAT det = alphaSqSum * betaSqSum - alphaBetaSum * alphaBetaSum;

    if (abs(det) > 0.00001f)
    {
        CGU_FLOAT detRcp = cmp_rcp(det);
        blockMin         = cmp_clampVec3f((alphaTexelSum * betaSqSum - betaTexelSum * alphaBetaSum) * detRcp, 0.0f, CMP_MAX_16BITFLOAT);
        blockMax         = cmp_clampVec3f((betaTexelSum * alphaSqSum - alphaTexelSum * alphaBetaSum) * detRcp, 0.0f, CMP_MAX_16BITFLOAT);
        blockMin         = cmp_f16tof32(blockMin);
        blockMax         = cmp_f16tof32(blockMax);
    }
}

//=============================================================================================

CMP_STATIC CGU_Vec3f cmp_fabsVec3f(CGU_Vec3f value)
{
#ifdef ASPM_HLSL
    return abs(value);
#else
    CGU_Vec3f res;
    res.x = abs(value.x);
    res.y = abs(value.y);
    res.z = abs(value.z);
    return res;
#endif
}


CMP_STATIC CGU_UINT32 cmp_constructColor(CMP_IN CGU_Vec3ui EndPoints)
{
    return (((EndPoints.r & 0x000000F8) << 8) | ((EndPoints.g & 0x000000FC) << 3) | ((EndPoints.b & 0x000000F8) >> 3));
}

CMP_STATIC CGU_UINT32 cmp_constructColorBGR(CMP_IN CGU_Vec3f EndPoints)
{
    return (((CGU_UINT32(EndPoints.b) & 0x000000F8) << 8) | ((CGU_UINT32(EndPoints.g) & 0x000000FC) << 3) | ((CGU_UINT32(EndPoints.r) & 0x000000F8) >> 3));
}

CMP_STATIC CGU_FLOAT cmp_mod(CMP_IN CGU_FLOAT value, CMP_IN CGU_FLOAT modval)
{
#ifdef ASPM_GLSL
    return mod(value, modval);
#endif
    return fmod(value, modval);
}

CMP_STATIC CGU_Vec3f cmp_truncVec3f(CMP_IN CGU_Vec3f value)
{
#ifdef ASPM_HLSL
    return trunc(value);
#else
    CGU_Vec3f res;
    res.x = trunc(value.x);
    res.y = trunc(value.y);
    res.z = trunc(value.z);
    return res;
#endif
}

CMP_STATIC CGU_Vec3f cmp_ceilVec3f(CMP_IN CGU_Vec3f value)
{
    CGU_Vec3f res;
    res.x = ceil(value.x);
    res.y = ceil(value.y);
    res.z = ceil(value.z);
    return res;
}

CMP_STATIC CGU_FLOAT cmp_sqrt(CGU_FLOAT value)
{
    return sqrt(value);
}

// Computes inverse square root over an implementation-defined range. The maximum error is implementation-defined.
CMP_STATIC CGV_FLOAT cmp_rsqrt(CGV_FLOAT f)
{
    CGV_FLOAT sf = sqrt(f);
    if (sf != 0)
        return 1 / sqrt(f);
    else
        return 0.0f;
}

// Common to BC7 API ------------------------------------------------------------------------------------------------------------------------

// valid bit range is 0..8 for mode 1
CMP_STATIC INLINE CGU_UINT32 cmp_shift_right_uint32(CMP_IN CGU_UINT32 v, CMP_IN CGU_INT bits)
{
    return v >> bits;  // (perf warning expected)
}

CMP_STATIC INLINE CGU_INT cmp_clampi(CMP_IN CGU_INT value, CMP_IN CGU_INT low, CMP_IN CGU_INT high)
{
    if (value < low)
        return low;
    else if (value > high)
        return high;
    return value;
}

CMP_STATIC INLINE CGU_INT32 cmp_clampi32(CMP_IN CGU_INT32 value, CMP_IN CGU_INT32 low, CMP_IN CGU_INT32 high)
{
    if (value < low)
        value = low;
    else if (value > high)
        value = high;
    return value;
}

CMP_STATIC CGV_FLOAT cmp_dot4f(CMP_IN CGV_Vec4f value1, CMP_IN CGV_Vec4f value2)
{
#ifdef ASPM_GPU
    return dot(value1, value2);
#else
    return (value1.x * value2.x) + (value1.y * value2.y) + (value1.z * value2.z) + (value1.w * value2.w);
#endif
}

CMP_STATIC INLINE void cmp_set_vec4f(CMP_INOUT CGU_Vec4f CMP_REFINOUT pV, CMP_IN CGU_FLOAT x, CMP_IN CGU_FLOAT y, CMP_IN CGU_FLOAT z, CMP_IN CGU_FLOAT w)
{
    pV[0] = x;
    pV[1] = y;
    pV[2] = z;
    pV[3] = w;
}

CMP_STATIC INLINE void cmp_set_vec4ui(CGU_Vec4ui CMP_REFINOUT pV, CMP_IN CGU_UINT8 x, CMP_IN CGU_UINT8 y, CMP_IN CGU_UINT8 z, CMP_IN CGU_UINT8 w)
{
    pV[0] = x;
    pV[1] = y;
    pV[2] = z;
    pV[3] = w;
}

CMP_STATIC inline void cmp_set_vec4ui_clamped(CGU_Vec4ui CMP_REFINOUT pRes, CMP_IN CGU_INT32 r, CMP_IN CGU_INT32 g, CMP_IN CGU_INT32 b, CMP_IN CGU_INT32 a)
{
    pRes[0] = (CGU_UINT8)cmp_clampi32(r, 0, 255);
    pRes[1] = (CGU_UINT8)cmp_clampi32(g, 0, 255);
    pRes[2] = (CGU_UINT8)cmp_clampi32(b, 0, 255);
    pRes[3] = (CGU_UINT8)cmp_clampi32(a, 0, 255);
}

CMP_STATIC inline CGU_Vec4f cmp_clampNorm4f(CMP_IN CGU_Vec4f pV)
{
    CGU_Vec4f res;
    res[0] = cmp_clampf(pV[0], 0.0f, 1.0f);
    res[1] = cmp_clampf(pV[1], 0.0f, 1.0f);
    res[2] = cmp_clampf(pV[2], 0.0f, 1.0f);
    res[3] = cmp_clampf(pV[3], 0.0f, 1.0f);
    return res;
}

CMP_STATIC INLINE CGU_Vec4f cmp_vec4ui_to_vec4f(CMP_IN CGU_Vec4ui pC)
{
    CGU_Vec4f res;
    cmp_set_vec4f(res, (CGU_FLOAT)pC[0], (CGU_FLOAT)pC[1], (CGU_FLOAT)pC[2], (CGU_FLOAT)pC[3]);
    return res;
}

CMP_STATIC INLINE void cmp_normalize(CGU_Vec4f CMP_REFINOUT pV)
{
    CGU_FLOAT s = cmp_dot4f(pV, pV);
    if (s != 0.0f)
    {
        s = 1.0f / cmp_sqrt(s);
        pV *= s;
    }
}

CMP_STATIC INLINE CGV_FLOAT cmp_squaref(CMP_IN CGV_FLOAT v)
{
    return v * v;
}

CMP_STATIC INLINE CGU_INT cmp_squarei(CMP_IN CGU_INT i)
{
    return i * i;
}

CMP_STATIC CGU_UINT8 cmp_clampui8(CMP_IN CGU_UINT8 v, CMP_IN CGU_UINT8 a, CMP_IN CGU_UINT8 b)
{
    if (v < a)
        return a;
    else if (v > b)
        return b;
    return v;
}

CMP_STATIC CGU_INT32 cmp_abs32(CMP_IN CGU_INT32 v)
{
    CGU_UINT32 msk = v >> 31;
    return (v ^ msk) - msk;
}

CMP_STATIC void cmp_swap32(CMP_INOUT CGU_UINT32 CMP_REFINOUT a, CMP_INOUT CGU_UINT32 CMP_REFINOUT b)
{
    CGU_UINT32 t = a;
    a            = b;
    b            = t;
}

// Computes inverse square root over an implementation-defined range. The maximum error is implementation-defined.
CMP_STATIC CGV_FLOAT cmp_Image_rsqrt(CMP_IN CGV_FLOAT f)
{
    CGV_FLOAT sf = sqrt(f);
    if (sf != 0)
        return 1 / sqrt(f);
    else
        return 0.0f;
}

CMP_STATIC void cmp_pack4bitindex32(CMP_INOUT CGU_UINT32 packed_index[2], CMP_IN CGU_UINT32 src_index[16])
{
    // Converts from unpacked index to packed index
    packed_index[0]  = 0x0000;
    packed_index[1]  = 0x0000;
    CGU_UINT32 shift = 0;  // was CGU_UINT8
    for (CGU_INT k = 0; k < 8; k++)
    {
        packed_index[0] |= (CGU_UINT32)(src_index[k] & 0x0F) << shift;
        packed_index[1] |= (CGU_UINT32)(src_index[k + 8] & 0x0F) << shift;
        shift += 4;
    }
}

CMP_STATIC void cmp_pack4bitindex(CMP_INOUT CGU_UINT32 packed_index[2], CMP_IN CGU_UINT8 src_index[16])
{
    // Converts from unpacked index to packed index
    packed_index[0]  = 0x0000;
    packed_index[1]  = 0x0000;
    CGU_UINT32 shift = 0;  // was CGU_UINT8
    for (CGU_INT k = 0; k < 8; k++)
    {
        packed_index[0] |= (CGU_UINT32)(src_index[k] & 0x0F) << shift;
        packed_index[1] |= (CGU_UINT32)(src_index[k + 8] & 0x0F) << shift;
        shift += 4;
    }
}

CMP_STATIC INLINE CGU_INT cmp_expandbits(CMP_IN CGU_INT v, CMP_IN CGU_INT bits)
{
    CGU_INT vv = v << (8 - bits);
    return vv + cmp_shift_right_uint32(vv, bits);
}

// This code need further improvements and investigation
CMP_STATIC INLINE CGU_UINT8 cmp_ep_find_floor2(CMP_IN CGV_FLOAT v, CMP_IN CGU_UINT8 bits, CMP_IN CGU_UINT8 use_par, CMP_IN CGU_UINT8 odd)
{
    CGU_UINT8 i1 = 0;
    CGU_UINT8 i2 = 1 << (bits - use_par);
    odd          = use_par ? odd : 0;
    while (i2 - i1 > 1)
    {
        CGU_UINT8 j    = (CGU_UINT8)((i1 + i2) * 0.5f);
        CGV_FLOAT ep_d = (CGV_FLOAT)cmp_expandbits((j << use_par) + odd, bits);
        if (v >= ep_d)
            i1 = j;
        else
            i2 = j;
    }

    return (i1 << use_par) + odd;
}

CMP_STATIC CGV_FLOAT cmp_absf(CMP_IN CGV_FLOAT a)
{
    return a > 0.0F ? a : -a;
}

CMP_STATIC INLINE CGU_UINT32 cmp_pow2Packed(CMP_IN CGU_INT x)
{
    return 1 << x;
}

CMP_STATIC INLINE CGU_UINT8 cmp_clampIndex(CMP_IN CGU_UINT8 v, CMP_IN CGU_UINT8 a, CMP_IN CGU_UINT8 b)
{
    if (v < a)
        return a;
    else if (v > b)
        return b;
    return v;
}

CMP_STATIC INLINE CGU_UINT8 shift_right_uint82(CMP_IN CGU_UINT8 v, CMP_IN CGU_UINT8 bits)
{
    return v >> bits;  // (perf warning expected)
}

#endif

CMP_STATIC CGU_INT cmp_QuantizeToBitSize(CMP_IN CGU_INT value, CMP_IN CGU_INT prec, CMP_IN CGU_BOOL signedfloat16)
{
    if (prec <= 1)
        return 0;
    CGU_BOOL negvalue = false;

    // move data to use extra bits for processing
    CGU_INT ivalue = value;

    if (signedfloat16)
    {
        if (value < 0)
        {
            negvalue = true;
            value    = -value;
        }
        prec--;
    }
    else
    {
        // clamp -ve
        if (value < 0)
            value = 0;
    }

    CGU_INT iQuantized;
    CGU_INT bias = (prec > 10 && prec != 16) ? ((1 << (prec - 11)) - 1) : 0;
    bias         = (prec == 16) ? 15 : bias;

    iQuantized = ((ivalue << prec) + bias) / (0x7bff + 1);  // 16 bit Float Max 0x7bff

    return (negvalue ? -iQuantized : iQuantized);
}

//=======================================================
// CPU GPU Macro API
//=======================================================

#ifdef ASPM_GPU
#define cmp_min(a, b) min(a, b)
#define cmp_max(a, b) max(a, b)
#else
#ifndef cmp_min
#define cmp_min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef cmp_max
#define cmp_max(a, b) ((a) > (b) ? (a) : (b))
#endif
#endif

//=======================================================
// CPU Template API
//=======================================================

#ifndef ASPM_GPU
#ifndef TEMPLATE_API_INTERFACED
#define TEMPLATE_API_INTERFACED

template <typename T>
T clamp(T& v, const T& lo, const T& hi)
{
    if (v < lo)
        return lo;
    else if (v > hi)
        return hi;
    return v;
}

template <typename T>
Vec4T<T> clamp(Vec4T<T>& v, const T& lo, const T& hi)
{
    Vec4T<T> res = v;
    if (v.x < lo)
        res.x = lo;
    else if (v.x > hi)
        res.x = hi;
    if (v.y < lo)
        res.y = lo;
    else if (v.y > hi)
        res.y = hi;
    if (v.z < lo)
        res.z = lo;
    else if (v.w > hi)
        res.w = hi;
    if (v.w < lo)
        res.w = lo;
    else if (v.z > hi)
        res.z = hi;
    return res;
}


template <typename T,typename T2>
T dot(T& v1, T2& v2)
{
    return (v1 * v2);
}

template <typename T>
T dot(Vec4T<T>& v1, Vec4T<T>& v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
}

template <typename T, typename T2>
T dot(Vec4T<T>& v1, Vec4T<T2>& v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w);
}
template <typename T>
T dot(Vec3T<T>& v1, Vec3T<T>& v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

template <typename T, typename T2>
T dot(Vec3T<T>& v1, Vec3T<T2>& v2)
{
    return (v1.x * v2.x + v1.y * v2.y + v1.z * v2.z);
}

#endif  //  API_INTERFACED
#endif  //  ASPM_GPU

#endif //
