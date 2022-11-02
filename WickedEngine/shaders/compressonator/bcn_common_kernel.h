//=============================================================================
// Copyright (c) 2018-2021    Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
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
//=====================================================================
//=====================================================================
// Block-compression (BC) functionality ref
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//=====================================================================
//************************************************************************************
// ** NOTE **
// Content and data types may change, use CMP_Core.h for interface to your application
//************************************************************************************

#ifndef _BCN_COMMON_KERNEL_H
#define _BCN_COMMON_KERNEL_H

#pragma warning(disable : 4505)  // disable warnings on unreferenced local function has been removed

#include "common_def.h"
#include "bcn_common_api.h"

//-----------------------------------------------------------------------
// When build is for CPU, we have some missing API calls common to GPU
// Use CPU CMP_Core replacements
//-----------------------------------------------------------------------
// used in BC1 HiQuaity
#if defined(ASPM_GPU) || defined(ASPM_HLSL) || defined(ASPM_OPENCL)
#define ALIGN_16
#else
#include INC_cmp_math_func
#if defined(WIN32) || defined(_WIN64)
#define ALIGN_16 __declspec(align(16))
#else  // !WIN32 && !_WIN64
#define ALIGN_16
#endif  // !WIN32 && !_WIN64
#endif



#define DXTC_OFFSET_ALPHA 0
#define DXTC_OFFSET_RGB 2

#define BC1CompBlockSize 8

#define RC 2
#define GC 1
#define BC 0
#define AC 3

/*
Channel Bits
*/
#define RGBA8888_CHANNEL_A 3
#define RGBA8888_CHANNEL_R 2
#define RGBA8888_CHANNEL_G 1
#define RGBA8888_CHANNEL_B 0
#define RGBA8888_OFFSET_A (RGBA8888_CHANNEL_A * 8)
#define RGBA8888_OFFSET_R (RGBA8888_CHANNEL_R * 8)
#define RGBA8888_OFFSET_G (RGBA8888_CHANNEL_G * 8)
#define RGBA8888_OFFSET_B (RGBA8888_CHANNEL_B * 8)

#ifndef MAX_ERROR
#define MAX_ERROR 128000.f
#endif

#define MAX_BLOCK 64
#define MAX_POINTS 16
#define BLOCK_SIZE      MAX_BLOCK
#define NUM_CHANNELS    4
#define NUM_ENDPOINTS   2
#define BLOCK_SIZE_4X4  16
#define CMP_ALPHA_RAMP  8  // Number of Ramp Points used for Alpha Channels in BC5

#define ConstructColour(r, g, b) (((r) << 11) | ((g) << 5) | (b)) // same as BC1ConstructColour in common_api

#define BYTEPP 4
#define CMP_QUALITY0 0.25f
#define CMP_QUALITY1 0.50f
#define CMP_QUALITY2 0.75f
#define POS(x, y) (pos_on_axis[(x) + (y)*4])

// Find the first approximation of the line
// Assume there is a linear relation
//   Z = a * X_In
//   Z = b * Y_In
// Find a,b to minimize MSE between Z and Z_In
#define EPS (2.f / 255.f) * (2.f / 255.f)
#define EPS2 3.f * (2.f / 255.f) * (2.f / 255.f)

// Grid precision
#define PIX_GRID 8

#define BYTE_MASK 0x00ff

#define SCH_STPS 3  // number of search steps to make at each end of interval
static CMP_CONSTANT CGU_FLOAT sMvF[] = {0.f, -1.f, 1.f, -2.f, 2.f, -3.f, 3.f, -4.f, 4.f, -5.f, 5.f, -6.f, 6.f, -7.f, 7.f, -8.f, 8.f};

#ifndef GBL_SCH_STEP
#define GBL_SCH_STEP_MXS 0.018f
#define GBL_SCH_EXT_MXS 0.1f
#define LCL_SCH_STEP_MXS 0.6f
#define GBL_SCH_STEP_MXQ 0.0175f
#define GBL_SCH_EXT_MXQ 0.154f
#define LCL_SCH_STEP_MXQ 0.45f

#define GBL_SCH_STEP GBL_SCH_STEP_MXS
#define GBL_SCH_EXT GBL_SCH_EXT_MXS
#define LCL_SCH_STEP LCL_SCH_STEP_MXS
#endif

#ifndef ASPM_GPU

typedef union
{
    struct colorblock64U
    {
        CGU_UINT8 col0;
        CGU_UINT8 col1;
        CGU_UINT8 indices[6];
    };
    CGU_INT8   cmp_data8[8];
    CGU_INT32  cmp_data32[2];
    CGU_UINT64 cmp_data64;
} CMP_BLOCK64_UNORM;

typedef union
{
    struct colorblock64S
    {
        CGU_UINT8 col0;
        CGU_UINT8 col1;
        CGU_UINT8 indices[6];
    };
    CGU_INT8   cmp_data8[8];
    CGU_INT32  cmp_data32[2];
    CGU_UINT64 cmp_data64[2];
} CMP_BLOCK64_SNORM;

typedef union
{
    CGU_INT8   cmp_data8[16];
    CGU_INT32  cmp_data32[4];
    CGU_UINT64 cmp_data64[2];
} CMP_BLOCK128_UNORM;

#endif

typedef struct
{
    CGU_UINT32 data;
    CGU_UINT32 index;
} CMP_di;

typedef struct
{
    CGU_FLOAT  data;
    CGU_UINT32 index;
} CMP_df;

typedef struct
{
    // user setable
    CGU_FLOAT  m_fquality;
    CGU_FLOAT  m_fChannelWeights[3];
    CGU_BOOL   m_bUseChannelWeighting;
    CGU_BOOL   m_bUseAdaptiveWeighting;
    CGU_BOOL   m_bUseFloat;
    CGU_BOOL   m_b3DRefinement;
    CGU_BOOL   m_bUseAlpha;
    CGU_BOOL   m_bIsSRGB;  // Use Linear to SRGB color conversion used in BC1, default is false
    CGU_BOOL   m_bIsSNORM;
    CGU_BOOL   m_sintsrc;  // source data pointer is signed data
    CGU_UINT32 m_nRefinementSteps;
    CGU_UINT32 m_nAlphaThreshold;
    CGU_BOOL   m_mapDecodeRGBA;
    CGU_UINT32 m_src_width;
    CGU_UINT32 m_src_height;
} CMP_BC15Options;

typedef struct 
{
    CGU_Vec3i end_point0;
    CGU_Vec3i end_point1;
    CGU_UINT8 indices[16];
    CGU_BOOL  m_3color;
} CMP_BC1_Encode_Results;

// used in BC1 LowQuality code
typedef struct
{
    CGU_Vec3f Color0;
    CGU_Vec3f Color1;
} CMP_EndPoints;

// Common data info used between encoders
// Defines properties of current 4x4 pixel block
typedef struct
{
    CGU_UINT32 grayscale_flag;
    CGU_UINT32 any_black_pixels;
    CGU_BOOL   all_colors_equal;
    CGU_Vec3i  min;
    CGU_Vec3i  max;
    CGU_Vec3i  total;
    CGU_Vec3i  avg;
} CMP_EncodeData;

typedef struct CMP_BC1_Block_t
{
    // Union struct not supported on GPU
    // 8 Bytes Total
    #ifndef ASPM_GPU
    union {
        struct { // 2 x 32bit 
            CGU_UINT32 colors;
            CGU_UINT32 indices;
        };
        struct { // 8 x 8bit
            CGU_UINT8 m_low_color[2];
            CGU_UINT8 m_high_color[2];
            CGU_UINT8 m_selectors[4];
        };
    }; 

    inline void set_low_color(CGU_UINT16 c)
    {
        m_low_color[0] = static_cast<CGU_UINT8>(c & 0xFF);
        m_low_color[1] = static_cast<CGU_UINT8>((c >> 8) & 0xFF);
    }
    inline void set_high_color(CGU_UINT16 c)
    {
        m_high_color[0] = static_cast<CGU_UINT8>(c & 0xFF);
        m_high_color[1] = static_cast<CGU_UINT8>((c >> 8) & 0xFF);
    }
    #else
            CGU_UINT32 colors;
            CGU_UINT32 indices;
    #endif
} CMP_BC1_Block;

// Helper functions to cut precision of floats
// Prec is a power of 10 value from 1,10,100,...,10000... INT MAX power 10
static CGU_BOOL cmp_compareprecision(CGU_FLOAT f1, CGU_FLOAT f2, CGU_INT Prec)
{
    CGU_INT scale1 = (CGU_INT)(f1 * Prec);
    CGU_INT scale2 = (CGU_INT)(f2 * Prec);
    return (scale1 == scale2);
}

// Helper function to compare floats to a set precision
static CGU_FLOAT cmp_getfloatprecision(CGU_FLOAT f1, CGU_INT Prec)
{
    CGU_INT scale1 = (CGU_INT)(f1 * Prec);
    return ((CGU_FLOAT)(scale1) / Prec);
}

static CGU_FLOAT cmp_getIndicesRGB(CMP_INOUT CGU_UINT32 CMP_PTRINOUT cmpindex,
                                   const CGU_Vec3f                   block[16],
                                   CGU_Vec3f                         minColor,
                                   CGU_Vec3f                         maxColor,
                                   CGU_BOOL                          getErr)
{
    CGU_UINT32 PackedIndices = 0;
    CGU_FLOAT  err           = 0.0f;
    CGU_Vec3f  cn[4];
    CGU_FLOAT  minDistance;

    if (getErr)
    {
        // remap to BC1 spec for decoding offsets,
        // where cn[0] > cn[1] Max Color = index 0, 2/3 offset =index 2, 1/3 offset = index 3, Min Color = index 1
        cn[0] = maxColor;
        cn[1] = minColor;
        cn[2] = cn[0] * 2.0f / 3.0f + cn[1] * 1.0f / 3.0f;
        cn[3] = cn[0] * 1.0f / 3.0f + cn[1] * 2.0f / 3.0f;
    }

    CGU_FLOAT  Scale       = 3.f / dot(minColor - maxColor, minColor - maxColor);
    CGU_Vec3f  ScaledRange = (minColor - maxColor) * Scale;
    CGU_FLOAT  Bias        = (dot(maxColor, maxColor) - dot(maxColor, minColor)) * Scale;
    CGU_INT    indexMap[4] = {0, 2, 3, 1};  // mapping based on BC1 Spec for color0 > color1
    CGU_UINT32 index;
    CGU_FLOAT  diff;

    for (CGU_UINT32 i = 0; i < 16; i++)
    {
        // Get offset from base scale
        diff  = dot(block[i], ScaledRange) + Bias;
        index = ((CGU_UINT32)round(diff)) & 0x3;

        // remap linear offset to spec offset
        index = indexMap[index];

        // use err calc for use in higher quality code
        if (getErr)
        {
            minDistance = dot(block[i] - cn[index], block[i] - cn[index]);
            err += minDistance;
        }

        // Map the 2 bit index into compress 32 bit block
        if (index)
            PackedIndices |= (index << (2 * i));
    }

    if (getErr)
        err = err * 0.0208333f;

    CMP_PTRINOUT cmpindex = PackedIndices;
    return err;
}

//---------------------------------------- BCn Common Utility Code -------------------------------------------------------

#ifndef ASPM_GPU
static void SetDefaultBC15Options(CMP_BC15Options* BC15Options)
{
    if (BC15Options)
    {
        BC15Options->m_fquality              = 1.0f;
        BC15Options->m_bUseChannelWeighting  = false;
        BC15Options->m_bUseAdaptiveWeighting = false;
        BC15Options->m_fChannelWeights[0]    = 0.3086f;
        BC15Options->m_fChannelWeights[1]    = 0.6094f;
        BC15Options->m_fChannelWeights[2]    = 0.0820f;
        BC15Options->m_nAlphaThreshold       = 128;
        BC15Options->m_bUseFloat             = false;
        BC15Options->m_b3DRefinement         = false;
        BC15Options->m_bUseAlpha             = false;
        BC15Options->m_bIsSNORM              = false;
        BC15Options->m_bIsSRGB               = false;
        BC15Options->m_nRefinementSteps      = 0;
        BC15Options->m_src_width             = 4;
        BC15Options->m_src_height            = 4;
#ifdef CMP_SET_BC13_DECODER_RGBA
        BC15Options->m_mapDecodeRGBA         = true;
#else
        BC15Options->m_mapDecodeRGBA         = false;
#endif
    }
}
#endif

static CMP_BC15Options CalculateColourWeightings(CGU_Vec4f rgbaBlock[BLOCK_SIZE_4X4], CMP_BC15Options BC15options)
{
    CGU_FLOAT fBaseChannelWeights[3] = {0.3086f, 0.6094f, 0.0820f};

    if (!BC15options.m_bUseChannelWeighting)
    {
        BC15options.m_fChannelWeights[0] = 1.0F;
        BC15options.m_fChannelWeights[1] = 1.0F;
        BC15options.m_fChannelWeights[2] = 1.0F;
        return BC15options;
    }

    if (BC15options.m_bUseAdaptiveWeighting)
    {
        float medianR = 0.0f, medianG = 0.0f, medianB = 0.0f;

        for (CGU_UINT32 k = 0; k < BLOCK_SIZE_4X4; k++)
        {
            medianR += rgbaBlock[k].x;
            medianG += rgbaBlock[k].y;
            medianB += rgbaBlock[k].z;
        }

        medianR /= BLOCK_SIZE_4X4;
        medianG /= BLOCK_SIZE_4X4;
        medianB /= BLOCK_SIZE_4X4;

        // Now skew the colour weightings based on the gravity center of the block
        float largest = max(max(medianR, medianG), medianB);

        if (largest > 0)
        {
            medianR /= largest;
            medianG /= largest;
            medianB /= largest;
        }
        else
            medianR = medianG = medianB = 1.0f;

        // Scale weightings back up to 1.0f
        CGU_FLOAT fWeightScale = 1.0f / (fBaseChannelWeights[0] + fBaseChannelWeights[1] + fBaseChannelWeights[2]);

        BC15options.m_fChannelWeights[0] = fBaseChannelWeights[0] * fWeightScale;
        BC15options.m_fChannelWeights[1] = fBaseChannelWeights[1] * fWeightScale;
        BC15options.m_fChannelWeights[2] = fBaseChannelWeights[2] * fWeightScale;

        BC15options.m_fChannelWeights[0] = ((BC15options.m_fChannelWeights[0] * 3 * medianR) + BC15options.m_fChannelWeights[0]) * 0.25f;
        BC15options.m_fChannelWeights[1] = ((BC15options.m_fChannelWeights[1] * 3 * medianG) + BC15options.m_fChannelWeights[1]) * 0.25f;
        BC15options.m_fChannelWeights[2] = ((BC15options.m_fChannelWeights[2] * 3 * medianB) + BC15options.m_fChannelWeights[2]) * 0.25f;

        fWeightScale = 1.0f / (BC15options.m_fChannelWeights[0] + BC15options.m_fChannelWeights[1] + BC15options.m_fChannelWeights[2]);

        BC15options.m_fChannelWeights[0] *= fWeightScale;
        BC15options.m_fChannelWeights[1] *= fWeightScale;
        BC15options.m_fChannelWeights[2] *= fWeightScale;
    }
    else
    {
        BC15options.m_fChannelWeights[0] = fBaseChannelWeights[0];
        BC15options.m_fChannelWeights[1] = fBaseChannelWeights[1];
        BC15options.m_fChannelWeights[2] = fBaseChannelWeights[2];
    }

    return BC15options;
}

static CMP_BC15Options CalculateColourWeightings3f(CGU_Vec3f rgbBlock[BLOCK_SIZE_4X4], CMP_BC15Options BC15options)
{
    CGU_FLOAT fBaseChannelWeights[3] = {0.3086f, 0.6094f, 0.0820f};

    if (!BC15options.m_bUseChannelWeighting)
    {
        BC15options.m_fChannelWeights[0] = 1.0F;
        BC15options.m_fChannelWeights[1] = 1.0F;
        BC15options.m_fChannelWeights[2] = 1.0F;
        return BC15options;
    }

    if (BC15options.m_bUseAdaptiveWeighting)
    {
        float medianR = 0.0f, medianG = 0.0f, medianB = 0.0f;

        for (CGU_UINT32 k = 0; k < BLOCK_SIZE_4X4; k++)
        {
            medianR += rgbBlock[k].x;
            medianG += rgbBlock[k].y;
            medianB += rgbBlock[k].z;
        }

        medianR /= BLOCK_SIZE_4X4;
        medianG /= BLOCK_SIZE_4X4;
        medianB /= BLOCK_SIZE_4X4;

        // Now skew the colour weightings based on the gravity center of the block
        float largest = max(max(medianR, medianG), medianB);

        if (largest > 0)
        {
            medianR /= largest;
            medianG /= largest;
            medianB /= largest;
        }
        else
            medianR = medianG = medianB = 1.0f;

        // Scale weightings back up to 1.0f
        CGU_FLOAT fWeightScale = 1.0f / (fBaseChannelWeights[0] + fBaseChannelWeights[1] + fBaseChannelWeights[2]);

        BC15options.m_fChannelWeights[0] = fBaseChannelWeights[0] * fWeightScale;
        BC15options.m_fChannelWeights[1] = fBaseChannelWeights[1] * fWeightScale;
        BC15options.m_fChannelWeights[2] = fBaseChannelWeights[2] * fWeightScale;

        BC15options.m_fChannelWeights[0] = ((BC15options.m_fChannelWeights[0] * 3 * medianR) + BC15options.m_fChannelWeights[0]) * 0.25f;
        BC15options.m_fChannelWeights[1] = ((BC15options.m_fChannelWeights[1] * 3 * medianG) + BC15options.m_fChannelWeights[1]) * 0.25f;
        BC15options.m_fChannelWeights[2] = ((BC15options.m_fChannelWeights[2] * 3 * medianB) + BC15options.m_fChannelWeights[2]) * 0.25f;

        fWeightScale = 1.0f / (BC15options.m_fChannelWeights[0] + BC15options.m_fChannelWeights[1] + BC15options.m_fChannelWeights[2]);

        BC15options.m_fChannelWeights[0] *= fWeightScale;
        BC15options.m_fChannelWeights[1] *= fWeightScale;
        BC15options.m_fChannelWeights[2] *= fWeightScale;
    }
    else
    {
        BC15options.m_fChannelWeights[0] = fBaseChannelWeights[0];
        BC15options.m_fChannelWeights[1] = fBaseChannelWeights[1];
        BC15options.m_fChannelWeights[2] = fBaseChannelWeights[2];
    }

    return BC15options;
}

static CGU_FLOAT cmp_getRampErr(CGU_FLOAT  Prj[BLOCK_SIZE_4X4],
                                CGU_FLOAT  PrjErr[BLOCK_SIZE_4X4],
                                CGU_FLOAT  PreMRep[BLOCK_SIZE_4X4],
                                CGU_FLOAT  StepErr,
                                CGU_FLOAT  lowPosStep,
                                CGU_FLOAT  highPosStep,
                                CGU_UINT32 dwUniqueColors)
{
    CGU_FLOAT error  = 0;
    CGU_FLOAT step   = (highPosStep - lowPosStep) / 3;  // using (dwNumChannels=4 - 1);
    CGU_FLOAT step_h = step * (CGU_FLOAT)0.5;
    CGU_FLOAT rstep  = (CGU_FLOAT)1.0f / step;

    for (CGU_UINT32 i = 0; i < dwUniqueColors; i++)
    {
        CGU_FLOAT v;
        // Work out which value in the block this select
        CGU_FLOAT del;

        if ((del = Prj[i] - lowPosStep) <= 0)
            v = lowPosStep;
        else if (Prj[i] - highPosStep >= 0)
            v = highPosStep;
        else
            v = floor((del + step_h) * rstep) * step + lowPosStep;

        // And accumulate the error
        CGU_FLOAT d = (Prj[i] - v);
        d *= d;
        CGU_FLOAT err = PreMRep[i] * d + PrjErr[i];
        error += err;
        if (StepErr < error)
        {
            error = StepErr;
            break;
        }
    }
    return error;
}

static CGU_Vec2ui cmp_compressExplicitAlphaBlock(const CGU_FLOAT AlphaBlockUV[16])
{
    CGU_Vec2ui compBlock = {0, 0};
    CGU_UINT8  i;
    for (i = 0; i < 16; i++)
    {
        CGU_UINT8 v = (CGU_UINT8)(AlphaBlockUV[i] * 255.0F);
        v           = (v + 7 - (v >> 4));
        v >>= 4;

        if (v < 0)
            v = 0;
        else if (v > 0xf)
            v = 0xf;

        if (i < 8)
            compBlock.x |= v << (4 * i);
        else
            compBlock.y |= v << (4 * (i - 8));
    }
    return compBlock;
}

static CGU_FLOAT cmp_getRampError(CGU_FLOAT _Blk[BLOCK_SIZE_4X4],
                                  CGU_FLOAT _Rpt[BLOCK_SIZE_4X4],
                                  CGU_FLOAT _maxerror,
                                  CGU_FLOAT _min_ex,
                                  CGU_FLOAT _max_ex,
                                  CGU_INT   _NmbrClrs)
{  // Max 16
    CGU_INT         i;
    CGU_FLOAT       error  = 0;
    const CGU_FLOAT step   = (_max_ex - _min_ex) / 7;  // (CGU_FLOAT)(dwNumPoints - 1);
    const CGU_FLOAT step_h = step * 0.5f;
    const CGU_FLOAT rstep  = 1.0f / step;

    for (i = 0; i < _NmbrClrs; i++)
    {
        CGU_FLOAT v;
        // Work out which value in the block this select
        CGU_FLOAT del;

        if ((del = _Blk[i] - _min_ex) <= 0)
            v = _min_ex;
        else if (_Blk[i] - _max_ex >= 0)
            v = _max_ex;
        else
            v = (floor((del + step_h) * rstep) * step) + _min_ex;

        // And accumulate the error
        CGU_FLOAT del2 = (_Blk[i] - v);
        error += del2 * del2 * _Rpt[i];

        // if we've already lost to the previous step bail out
        if (_maxerror < error)
        {
            error = _maxerror;
            break;
        }
    }
    return error;
}

static CGU_FLOAT cmp_linearBlockRefine(CGU_FLOAT _Blk[BLOCK_SIZE_4X4],
                                       CGU_FLOAT _Rpt[BLOCK_SIZE_4X4],
                                       CGU_FLOAT _MaxError,
                                       CMP_INOUT CGU_FLOAT CMP_PTRINOUT _min_ex,
                                       CMP_INOUT CGU_FLOAT CMP_PTRINOUT _max_ex,
                                       CGU_FLOAT                        _m_step,
                                       CGU_FLOAT                        _min_bnd,
                                       CGU_FLOAT                        _max_bnd,
                                       CGU_INT                          _NmbrClrs)
{
    // Start out assuming our endpoints are the min and max values we've
    // determined

    // Attempt a (simple) progressive refinement step to reduce noise in the
    // output image by trying to find a better overall match for the endpoints.

    CGU_FLOAT maxerror = _MaxError;
    CGU_FLOAT min_ex   = CMP_PTRINOUT _min_ex;
    CGU_FLOAT max_ex   = CMP_PTRINOUT _max_ex;

    CGU_INT mode, bestmode;

    do
    {
        CGU_FLOAT cr_min0 = min_ex;
        CGU_FLOAT cr_max0 = max_ex;
        for (bestmode = -1, mode = 0; mode < SCH_STPS * SCH_STPS; mode++)
        {
            // check each move (see sStep for direction)
            CGU_FLOAT cr_min = min_ex + _m_step * sMvF[mode / SCH_STPS];
            CGU_FLOAT cr_max = max_ex + _m_step * sMvF[mode % SCH_STPS];

            cr_min = max(cr_min, _min_bnd);
            cr_max = min(cr_max, _max_bnd);

            CGU_FLOAT error;
            error = cmp_getRampError(_Blk, _Rpt, maxerror, cr_min, cr_max, _NmbrClrs);

            if (error < maxerror)
            {
                maxerror = error;
                bestmode = mode;
                cr_min0  = cr_min;
                cr_max0  = cr_max;
            }
        }

        if (bestmode != -1)
        {
            // make move (see sStep for direction)
            min_ex = cr_min0;
            max_ex = cr_max0;
        }
    } while (bestmode != -1);

    CMP_PTRINOUT _min_ex = min_ex;
    CMP_PTRINOUT _max_ex = max_ex;

    return maxerror;
}

static CGU_Vec2f cmp_getLinearEndPoints(CGU_FLOAT _Blk[BLOCK_SIZE_4X4], CMP_IN CGU_FLOAT fquality, CMP_IN CGU_BOOL isSigned)
{
    CGU_UINT32 i;
    CGU_Vec2f  cmpMinMax;

    //================================================================
    // Bounding Box
    // lowest quality calculation to get min and max value to use
    //================================================================
    if (fquality < CMP_QUALITY2)
    {
        cmpMinMax.x = _Blk[0];
        cmpMinMax.y = _Blk[0];
        for (i = 1; i < BLOCK_SIZE_4X4; ++i)
        {
            cmpMinMax.x = min(cmpMinMax.x, _Blk[i]);
            cmpMinMax.y = max(cmpMinMax.y, _Blk[i]);
        }
        return cmpMinMax;
    }

    //================================================================
    // Do more calculations to get the best min and max values to use
    //================================================================
    CGU_FLOAT Ramp[2];

    // Result defaults for SNORM or UNORM
    Ramp[0] = isSigned ? -1.0f : 0.0f;
    Ramp[1] = 1.0f;

    ALIGN_16 CGU_FLOAT afUniqueValues[BLOCK_SIZE_4X4];
    ALIGN_16 CGU_FLOAT afValueRepeats[BLOCK_SIZE_4X4];
    for (i = 0; i < BLOCK_SIZE_4X4; i++)
        afUniqueValues[i] = afValueRepeats[i] = 0.f;

    // For each unique value we compute the number of it appearances.
    CGU_FLOAT fBlk[BLOCK_SIZE_4X4];

// sort the input
#ifndef ASPM_GPU
    memcpy(fBlk, _Blk, BLOCK_SIZE_4X4 * sizeof(CGU_FLOAT));
    qsort((void*)fBlk, (size_t)BLOCK_SIZE_4X4, sizeof(CGU_FLOAT), QSortFCmp);
#else
    CGU_UINT32 j;

    for (i = 0; i < BLOCK_SIZE_4X4; i++)
    {
        fBlk[i] = _Blk[i];
    }

    CMP_df what[BLOCK_SIZE];

    for (i = 0; i < BLOCK_SIZE_4X4; i++)
    {
        what[i].index = i;
        what[i].data  = fBlk[i];
    }

    CGU_UINT32 tmp_index;
    CGU_FLOAT  tmp_data;

    for (i = 1; i < BLOCK_SIZE_4X4; i++)
    {
        for (j = i; j > 0; j--)
        {
            if (what[j - 1].data > what[j].data)
            {
                tmp_index         = what[j].index;
                tmp_data          = what[j].data;
                what[j].index     = what[j - 1].index;
                what[j].data      = what[j - 1].data;
                what[j - 1].index = tmp_index;
                what[j - 1].data  = tmp_data;
            }
        }
    }

    for (i = 0; i < BLOCK_SIZE_4X4; i++)
        fBlk[i] = what[i].data;
#endif

    CGU_FLOAT new_p = -2.0f;

    CGU_UINT32 dwUniqueValues    = 0;
    afUniqueValues[0]            = 0.0f;
    CGU_BOOL requiresCalculation = true;

    {
        // Ramp not fixed
        for (i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            if (new_p != fBlk[i])
            {
                afUniqueValues[dwUniqueValues] = new_p = fBlk[i];
                afValueRepeats[dwUniqueValues]         = 1.f;
                dwUniqueValues++;
            }
            else if (dwUniqueValues)
                afValueRepeats[dwUniqueValues - 1] += 1.f;
        }

        // if number of unique colors is less or eq 2, we've done
        if (dwUniqueValues <= 2)
        {
            Ramp[0] = floor(afUniqueValues[0] * 255.0f + 0.5f);
            if (dwUniqueValues == 1)
                Ramp[1] = Ramp[0] + 1.f;
            else
                Ramp[1] = floor(afUniqueValues[1] * 255.0f + 0.5f);
            requiresCalculation = false;
        }
    }  // Ramp not fixed

    if (requiresCalculation)
    {
        CGU_FLOAT min_ex  = afUniqueValues[0];
        CGU_FLOAT max_ex  = afUniqueValues[dwUniqueValues - 1];
        CGU_FLOAT min_bnd = 0, max_bnd = 1.;
        CGU_FLOAT min_r = min_ex, max_r = max_ex;
        CGU_FLOAT gbl_l = 0, gbl_r = 0;
        CGU_FLOAT cntr = (min_r + max_r) / 2;

        CGU_FLOAT gbl_err = MAX_ERROR;
        // Trying to avoid unnecessary calculations. Heuristics: after some analisis
        // it appears that in integer case, if the input interval not more then 48
        // we won't get much better
        bool wantsSearch = !((max_ex - min_ex) <= (48.f / 256.0f));

        if (wantsSearch)
        {
            // Search.
            // 1. take the vicinities of both low and high bound of the input
            // interval.
            // 2. setup some search step
            // 3. find the new low and high bound which provides an (sub) optimal
            // (infinite precision) clusterization.
            CGU_FLOAT gbl_llb = (min_bnd > min_r - GBL_SCH_EXT) ? min_bnd : min_r - GBL_SCH_EXT;
            CGU_FLOAT gbl_rrb = (max_bnd < max_r + GBL_SCH_EXT) ? max_bnd : max_r + GBL_SCH_EXT;
            CGU_FLOAT gbl_lrb = (cntr < min_r + GBL_SCH_EXT) ? cntr : min_r + GBL_SCH_EXT;
            CGU_FLOAT gbl_rlb = (cntr > max_r - GBL_SCH_EXT) ? cntr : max_r - GBL_SCH_EXT;

            for (CGU_FLOAT step_l = gbl_llb; step_l < gbl_lrb; step_l += GBL_SCH_STEP)
            {
                for (CGU_FLOAT step_r = gbl_rrb; gbl_rlb <= step_r; step_r -= GBL_SCH_STEP)
                {
                    CGU_FLOAT sch_err;
                    // an sse version is avaiable
                    sch_err = cmp_getRampError(afUniqueValues, afValueRepeats, gbl_err, step_l, step_r, dwUniqueValues);
                    if (sch_err < gbl_err)
                    {
                        gbl_err = sch_err;
                        gbl_l   = step_l;
                        gbl_r   = step_r;
                    }
                }
            }

            min_r = gbl_l;
            max_r = gbl_r;
        }  // want search

        // This is a refinement call. The function tries to make several small
        // stretches or squashes to minimize quantization error.
        CGU_FLOAT m_step = LCL_SCH_STEP / 256.0f;
        cmp_linearBlockRefine(afUniqueValues, afValueRepeats, gbl_err, CMP_REFINOUT min_r, CMP_REFINOUT max_r, m_step, min_bnd, max_bnd, dwUniqueValues);

        min_ex = min_r;
        max_ex = max_r;
        max_ex *= 255.0f;
        min_ex *= 255.0f;

        Ramp[0] = floor(min_ex + 0.5f);
        Ramp[1] = floor(max_ex + 0.5f);
    }

    // Ensure that the two endpoints are not the same
    // This is legal but serves no need & can break some optimizations in the compressor
    if (Ramp[0] == Ramp[1])
    {
        if (Ramp[1] < 255.f)
            Ramp[1] = Ramp[1] + 1.0f;
        else if (Ramp[1] > 0.0f)
            Ramp[1] = Ramp[1] - 1.0f;
    }

    cmpMinMax.x = Ramp[0];
    cmpMinMax.y = Ramp[1];

    return cmpMinMax;
}

static CGU_Vec2ui cmp_getBlockPackedIndices(CGU_Vec2f RampMinMax, CGU_FLOAT alphaBlock[BLOCK_SIZE_4X4], CMP_IN CGU_FLOAT fquality)
{
    CGU_UINT32 i;
    CGU_UINT32 j;
    CGU_Vec2ui cmpBlock = {0, 0};
    CGU_UINT32 MinRampU;
    CGU_UINT32 MaxRampU;
    CGU_INT32  pcIndices[BLOCK_SIZE_4X4];

    if (fquality < CMP_QUALITY2)
    {
        CGU_FLOAT Range;
        CGU_FLOAT RampSteps;  // segments into 0..7 sections
        CGU_FLOAT Bias;

        if (RampMinMax.x != RampMinMax.y)
            Range = RampMinMax.x - RampMinMax.y;
        else
            Range = 1.0f;

        RampSteps = 7.f / Range;  // segments into 0..7 sections
        Bias      = -RampSteps * RampMinMax.y;

        for (i = 0; i < 16; ++i)
        {
            pcIndices[i] = (CGU_UINT32)round(alphaBlock[i] * RampSteps + Bias);
            if (i < 5)
            {
                pcIndices[i] += (pcIndices[i] > 0) - (7 * (pcIndices[i] == 7));
            }
            else if (i > 5)
            {
                pcIndices[i] += (pcIndices[i] > 0) - (7 * (pcIndices[i] == 7 ? 1 : 0));
            }
            else
            {
                pcIndices[i] += (pcIndices[i] > 0) - (7 * (pcIndices[i] == 7));
            }
        }

        MinRampU = (CGU_UINT32)round(RampMinMax.x * 255.0f);
        MaxRampU = (CGU_UINT32)round(RampMinMax.y * 255.0f);

        cmpBlock.x = (MinRampU << 8) | MaxRampU;
        cmpBlock.y = 0;
        for (i = 0; i < 5; ++i)
        {
            cmpBlock.x |= (pcIndices[i] << (16 + (i * 3)));
        }
        {
            cmpBlock.x |= (pcIndices[5] << 31);
            cmpBlock.y |= (pcIndices[5] >> 1);
        }
        for (i = 6; i < BLOCK_SIZE_4X4; ++i)
        {
            cmpBlock.y |= (pcIndices[i] << (i * 3 - 16));
        }
    }
    else
    {
        CGU_UINT32 epoint;
        CGU_FLOAT  alpha[BLOCK_SIZE_4X4];
        CGU_FLOAT  OverIntFctr;
        CGU_FLOAT  shortest;
        CGU_FLOAT  adist;

        for (i = 0; i < BLOCK_SIZE_4X4; i++)
            pcIndices[i] = 0;

        for (i = 0; i < MAX_POINTS; i++)
            alpha[i] = 0;

        // GetRmp1
        {
            if (RampMinMax.x <= RampMinMax.y)
            {
                CGU_FLOAT t  = RampMinMax.x;
                RampMinMax.x = RampMinMax.y;
                RampMinMax.y = t;
            }

            //=============================
            // final clusterization applied
            //=============================
            CGU_FLOAT ramp[NUM_ENDPOINTS];

            ramp[0] = RampMinMax.x;
            ramp[1] = RampMinMax.y;

            {
                // BldRmp1
                alpha[0] = ramp[0];
                alpha[1] = ramp[1];
                for (epoint = 1; epoint < CMP_ALPHA_RAMP - 1; epoint++)
                    alpha[epoint + 1] = (alpha[0] * (CMP_ALPHA_RAMP - 1 - epoint) + alpha[1] * epoint) / (CGU_FLOAT)(CMP_ALPHA_RAMP - 1);
                for (epoint = CMP_ALPHA_RAMP; epoint < BLOCK_SIZE_4X4; epoint++)
                    alpha[epoint] = 100000.f;
            }  // BldRmp1

            // FixedRamp
            for (i = 0; i < CMP_ALPHA_RAMP; i++)
            {
                alpha[i] = floor(alpha[i] + 0.5f);
            }
        }  // GetRmp1

        OverIntFctr = 1.f / 255.0f;
        for (i = 0; i < CMP_ALPHA_RAMP; i++)
            alpha[i] *= OverIntFctr;

        // For each colour in the original block, calculate its weighted
        // distance from each point in the original and assign it
        // to the closest cluster
        for (i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            shortest = 10000000.f;
            for (j = 0; j < CMP_ALPHA_RAMP; j++)
            {
                adist = (alphaBlock[i] - alpha[j]);
                adist *= adist;
                if (adist < shortest)
                {
                    shortest     = adist;
                    pcIndices[i] = j;
                }
            }
        }

        //==================================================
        // EncodeAlphaBlock
        //==================================================
        MinRampU = (CGU_UINT32)RampMinMax.x;
        MaxRampU = (CGU_UINT32)RampMinMax.y;

        cmpBlock.x = (MaxRampU << 8) | MinRampU;
        cmpBlock.y = 0;
        for (i = 0; i < 5; i++)
        {
            cmpBlock.x |= (pcIndices[i]) << (16 + (i * 3));
        }
        {
            cmpBlock.x |= (pcIndices[5] & 0x1) << 31;
            cmpBlock.y |= (pcIndices[5] & 0x6) >> 1;
        }
        for (i = 6; i < BLOCK_SIZE_4X4; i++)
        {
            cmpBlock.y |= (pcIndices[i]) << (i * 3 - 16);
        }
    }
    return cmpBlock;
}

//======================= SNORM CODE ==================================

static CGU_INT8 cmp_snormFloatToSInt(CGU_FLOAT fsnorm)
{
    if (isnan(fsnorm))
        fsnorm = 0;
    else if (fsnorm > 1)
        fsnorm = 1;  // Clamp to 1
    else if (fsnorm < -1)
        fsnorm = -1;  // Clamp to -1

    fsnorm = fsnorm * 127U;

    // shift round up or down
    if (fsnorm >= 0)
        fsnorm += .5f;
    else
        fsnorm -= .5f;

#ifdef ASPM_GPU
    CGU_INT8 res = (CGU_INT8)fsnorm;
#else
    CGU_INT8  res       = static_cast<CGU_INT8>(fsnorm);
#endif
    return (res);
}

static CGU_Vec2f cmp_optimizeEndPoints(CGU_FLOAT pPoints[BLOCK_SIZE_4X4], CGU_INT8 cSteps, CGU_BOOL isSigned)
{
    CGU_Vec2f fendpoints;
    CGU_FLOAT MAX_VALUE = 1.0f;
    CGU_FLOAT MIN_VALUE = isSigned ? -1.0f : 0.0f;

    // Find Min and Max points, as starting point
    CGU_FLOAT fX = MAX_VALUE;
    CGU_FLOAT fY = MIN_VALUE;

    if (8 == cSteps)
    {
        for (CGU_INT8 iPoint = 0; iPoint < BLOCK_SIZE_4X4; iPoint++)
        {
            if (pPoints[iPoint] < fX)
                fX = pPoints[iPoint];

            if (pPoints[iPoint] > fY)
                fY = pPoints[iPoint];
        }
    }
    else
    {
        for (CGU_INT8 iPoint = 0; iPoint < BLOCK_SIZE_4X4; iPoint++)
        {
            if (pPoints[iPoint] < fX && pPoints[iPoint] > MIN_VALUE)
                fX = pPoints[iPoint];

            if (pPoints[iPoint] > fY && pPoints[iPoint] < MAX_VALUE)
                fY = pPoints[iPoint];
        }

        if (fX == fY)
        {
            fY = MAX_VALUE;
        }
    }

    //===================
    // Use Newton Method
    //===================
#ifdef ASPM_GPU
    CGU_FLOAT cStepsDiv = (CGU_FLOAT)(cSteps - 1);
#else
    CGU_FLOAT cStepsDiv = static_cast<CGU_FLOAT>(cSteps - 1);
#endif
    CGU_FLOAT pSteps[8];
    CGU_FLOAT fc;
    CGU_FLOAT fd;

    for (CGU_INT8 iIteration = 0; iIteration < 8; iIteration++)
    {
        // reach minimum threashold break
        if ((fY - fX) < (1.0f / 256.0f))
            break;

        CGU_FLOAT fScale = cStepsDiv / (fY - fX);

        // Calculate new steps
        for (CGU_INT8 iStep = 0; iStep < cSteps; iStep++)
        {
            fc            = (cStepsDiv - (CGU_FLOAT)iStep) / cStepsDiv;
            fd            = (CGU_FLOAT)iStep / cStepsDiv;
            pSteps[iStep] = fc * fX + fd * fY;
        }

        if (6 == cSteps)
        {
            pSteps[6] = MIN_VALUE;
            pSteps[7] = MAX_VALUE;
        }

        // Evaluate function, and derivatives
        CGU_FLOAT dX  = 0.0f;
        CGU_FLOAT dY  = 0.0f;
        CGU_FLOAT d2X = 0.0f;
        CGU_FLOAT d2Y = 0.0f;

        for (CGU_INT8 iPoint = 0; iPoint < BLOCK_SIZE_4X4; iPoint++)
        {
            CGU_FLOAT fDot = (pPoints[iPoint] - fX) * fScale;

            CGU_INT8 iStep;
            if (fDot <= 0.0f)
            {
                iStep = ((6 == cSteps) && (pPoints[iPoint] <= (fX + MIN_VALUE) * 0.5f)) ? 6u : 0u;
            }
            else if (fDot >= cStepsDiv)
            {
                iStep = ((6 == cSteps) && (pPoints[iPoint] >= (fY + MAX_VALUE) * 0.5f)) ? 7u : (cSteps - 1);
            }
            else
            {
                iStep = (CGU_UINT32)(fDot + 0.5f);
            }

            // steps to improve quality
            if (iStep < cSteps)
            {
                fc              = (cStepsDiv - (CGU_FLOAT)iStep) / cStepsDiv;
                fd              = (CGU_FLOAT)iStep / cStepsDiv;
                CGU_FLOAT fDiff = pSteps[iStep] - pPoints[iPoint];
                dX += fc * fDiff;
                d2X += fc * fc;
                dY += fd * fDiff;
                d2Y += fd * fd;
            }
        }

        // Move endpoints
        if (d2X > 0.0f)
            fX -= dX / d2X;

        if (d2Y > 0.0f)
            fY -= dY / d2Y;

        if (fX > fY)
        {
            float f = fX;
            fX      = fY;
            fY      = f;
        }

        if ((dX * dX < (1.0f / 64.0f)) && (dY * dY < (1.0f / 64.0f)))
            break;
    }

    fendpoints.x = (fX < MIN_VALUE) ? MIN_VALUE : (fX > MAX_VALUE) ? MAX_VALUE : fX;
    fendpoints.y = (fY < MIN_VALUE) ? MIN_VALUE : (fY > MAX_VALUE) ? MAX_VALUE : fY;

    return fendpoints;
}

static CGU_Vec2i cmp_findEndpointsAlphaBlockSnorm(CGU_FLOAT alphaBlockSnorm[BLOCK_SIZE_4X4])
{
    //================================================================
    // Bounding Box
    // lowest quality calculation to get min and max value to use
    //================================================================
    CGU_Vec2f cmpMinMax;
    cmpMinMax.x = alphaBlockSnorm[0];
    cmpMinMax.y = alphaBlockSnorm[0];

    for (CGU_UINT8 i = 0; i < BLOCK_SIZE_4X4; ++i)
    {
        if (alphaBlockSnorm[i] < cmpMinMax.x)
        {
            cmpMinMax.x = alphaBlockSnorm[i];
        }
        else if (alphaBlockSnorm[i] > cmpMinMax.y)
        {
            cmpMinMax.y = alphaBlockSnorm[i];
        }
    }

    CGU_Vec2i endpoints;
    CGU_Vec2f fendpoints;

    // Are we done for lowest quality setting!
    // CGU_FLOAT fquality = 1.0f;
    //
    // if (fquality < CMP_QUALITY2) {
    //     endpoints.x = (CGU_INT8)(cmpMinMax.x);
    //     endpoints.y = (CGU_INT8)(cmpMinMax.y);
    //     return endpoints;
    // }

    //================================================================
    // Do more calculations to get the best min and max values to use
    //================================================================
    if ((-1.0f == cmpMinMax.x || 1.0f == cmpMinMax.y))
    {
        fendpoints  = cmp_optimizeEndPoints(alphaBlockSnorm, 6, true);
        endpoints.x = cmp_snormFloatToSInt(fendpoints.x);
        endpoints.y = cmp_snormFloatToSInt(fendpoints.y);
    }
    else
    {
        fendpoints  = cmp_optimizeEndPoints(alphaBlockSnorm, 8, true);
        endpoints.x = cmp_snormFloatToSInt(fendpoints.y);
        endpoints.y = cmp_snormFloatToSInt(fendpoints.x);
    }

    return endpoints;
}

#ifndef ASPM_HLSL
static CGU_UINT64 cmp_getBlockPackedIndicesSNorm(CGU_Vec2f alphaMinMax, CGU_FLOAT alphaBlockSnorm[BLOCK_SIZE_4X4], CGU_UINT64 data)
{
    CGU_FLOAT alpha[8];
    alpha[0] = alphaMinMax.x;
    alpha[1] = alphaMinMax.y;

    if (alphaMinMax.x > alphaMinMax.y)
    {
        // 8-alpha block:  derive the other six alphas.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
        alpha[2] = (alpha[0] * 6.0f + alpha[1]) / 7.0f;
        alpha[3] = (alpha[0] * 5.0f + alpha[1] * 2.0f) / 7.0f;
        alpha[4] = (alpha[0] * 4.0f + alpha[1] * 3.0f) / 7.0f;
        alpha[5] = (alpha[0] * 3.0f + alpha[1] * 4.0f) / 7.0f;
        alpha[6] = (alpha[0] * 2.0f + alpha[1] * 5.0f) / 7.0f;
        alpha[7] = (alpha[0] + alpha[1] * 6.0f) / 7.0f;
    }
    else
    {
        // 6-alpha block.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
        alpha[2] = (alpha[0] * 4.0f + alpha[1]) / 5.0f;
        alpha[3] = (alpha[0] * 3.0f + alpha[1] * 2.0f) / 5.0f;
        alpha[4] = (alpha[0] * 2.0f + alpha[1] * 3.0f) / 5.0f;
        alpha[5] = (alpha[0] + alpha[1] * 4.0f) / 5.0f;
        alpha[6] = -1.0f;
        alpha[7] = 1.0f;
    }

    // Index all colors using best alpha value
    for (CGU_UINT8 i = 0; i < BLOCK_SIZE_4X4; ++i)
    {
        CGU_UINT8 uBestIndex = 0;
        CGU_FLOAT fBestDelta = CMP_FLOAT_MAX;
        for (CGU_INT32 uIndex = 0; uIndex < 8; uIndex++)
        {
            CGU_FLOAT fCurrentDelta = fabs(alpha[uIndex] - alphaBlockSnorm[i]);
            if (fCurrentDelta < fBestDelta)
            {
                uBestIndex = uIndex;
                fBestDelta = fCurrentDelta;
            }
        }

        data &= ~((CGU_UINT64)(0x07) << (3 * i + 16));
        data |= ((CGU_UINT64)(uBestIndex) << (3 * i + 16));
    }

    return data;
}
#endif
static void cmp_getCompressedAlphaRampS(CGU_INT8 alpha[8], const CGU_UINT32 compressedBlock[2])
{
    alpha[0] = (CGU_INT8)(compressedBlock[0] & 0xff);
    alpha[1] = (CGU_INT8)((compressedBlock[0] >> 8) & 0xff);

    if (alpha[0] > alpha[1])
    {
        // 8-alpha block:  derive the other six alphas.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
#ifdef ASPM_GPU
        alpha[2] = (CGU_UINT8)((6 * alpha[0] + 1 * alpha[1] + 3) / 7);  // bit code 010
        alpha[3] = (CGU_UINT8)((5 * alpha[0] + 2 * alpha[1] + 3) / 7);  // bit code 011
        alpha[4] = (CGU_UINT8)((4 * alpha[0] + 3 * alpha[1] + 3) / 7);  // bit code 100
        alpha[5] = (CGU_UINT8)((3 * alpha[0] + 4 * alpha[1] + 3) / 7);  // bit code 101
        alpha[6] = (CGU_UINT8)((2 * alpha[0] + 5 * alpha[1] + 3) / 7);  // bit code 110
        alpha[7] = (CGU_UINT8)((1 * alpha[0] + 6 * alpha[1] + 3) / 7);  // bit code 111
#else
        alpha[2] = static_cast<CGU_UINT8>((6 * alpha[0] + 1 * alpha[1] + 3) / 7);  // bit code 010
        alpha[3] = static_cast<CGU_UINT8>((5 * alpha[0] + 2 * alpha[1] + 3) / 7);  // bit code 011
        alpha[4] = static_cast<CGU_UINT8>((4 * alpha[0] + 3 * alpha[1] + 3) / 7);  // bit code 100
        alpha[5] = static_cast<CGU_UINT8>((3 * alpha[0] + 4 * alpha[1] + 3) / 7);  // bit code 101
        alpha[6] = static_cast<CGU_UINT8>((2 * alpha[0] + 5 * alpha[1] + 3) / 7);  // bit code 110
        alpha[7] = static_cast<CGU_UINT8>((1 * alpha[0] + 6 * alpha[1] + 3) / 7);  // bit code 111
#endif
    }
    else
    {
        // 6-alpha block.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
#ifdef ASPM_GPU
        alpha[2] = (CGU_UINT8)((4 * alpha[0] + 1 * alpha[1] + 2) / 5);  // Bit code 010
        alpha[3] = (CGU_UINT8)((3 * alpha[0] + 2 * alpha[1] + 2) / 5);  // Bit code 011
        alpha[4] = (CGU_UINT8)((2 * alpha[0] + 3 * alpha[1] + 2) / 5);  // Bit code 100
        alpha[5] = (CGU_UINT8)((1 * alpha[0] + 4 * alpha[1] + 2) / 5);  // Bit code 101
#else
        alpha[2] = static_cast<CGU_UINT8>((4 * alpha[0] + 1 * alpha[1] + 2) / 5);  // Bit code 010
        alpha[3] = static_cast<CGU_UINT8>((3 * alpha[0] + 2 * alpha[1] + 2) / 5);  // Bit code 011
        alpha[4] = static_cast<CGU_UINT8>((2 * alpha[0] + 3 * alpha[1] + 2) / 5);  // Bit code 100
        alpha[5] = static_cast<CGU_UINT8>((1 * alpha[0] + 4 * alpha[1] + 2) / 5);  // Bit code 101
#endif
        alpha[6] = -128;  // Bit code 110
        alpha[7] = 127;   // Bit code 111
    }
}

static void cmp_decompressAlphaBlockS(CGU_INT8 alphaBlock[BLOCK_SIZE_4X4], const CGU_UINT32 compressedBlock[2])
{
    CGU_UINT32 i;
    CGU_INT8   alpha[8];
    cmp_getCompressedAlphaRampS(alpha, compressedBlock);

    for (i = 0; i < BLOCK_SIZE_4X4; i++)
    {
        CGU_UINT32 index;
        if (i < 5)
            index = (compressedBlock[0] & (0x7 << (16 + (i * 3)))) >> (16 + (i * 3));
        else if (i > 5)
            index = (compressedBlock[1] & (0x7 << (2 + (i - 6) * 3))) >> (2 + (i - 6) * 3);
        else
        {
            index = (compressedBlock[0] & 0x80000000) >> 31;
            index |= (compressedBlock[1] & 0x3) << 1;
        }

        alphaBlock[i] = alpha[index];
    }
}

//=============================================================================

// Processes Alpha Channel either as Unsigned Norm (0..1) or (Signed Norm -1..1)
static CGU_Vec2ui cmp_compressAlphaBlock(CMP_IN CGU_FLOAT alphaBlock[BLOCK_SIZE_4X4], CMP_IN CGU_FLOAT fquality, CMP_IN CGU_BOOL isSigned)
{
    CGU_Vec2ui CmpBlock;

    if (isSigned)
    {
#ifndef ASPM_HLSL
        union
        {
            CGU_INT32 compressedBlock[2];
            struct
            {
                CGU_INT8  red_0;
                CGU_INT8  red_1;
                CGU_UINT8 indices[6];
            };
            CGU_UINT64 data;
        } BC4_Snorm_block;

#ifndef ASPM_GPU
        BC4_Snorm_block.data = 0LL;
#else
        BC4_Snorm_block.data         = 0;
#endif

        CGU_Vec2i reds;
        reds = cmp_findEndpointsAlphaBlockSnorm(alphaBlock);

        BC4_Snorm_block.red_0 = reds.x & 0xFF;
        BC4_Snorm_block.red_1 = reds.y & 0xFF;

        // check low end boundaries
        if (BC4_Snorm_block.red_0 == -128)
            BC4_Snorm_block.red_0 = -127;
        if (BC4_Snorm_block.red_1 == -128)
            BC4_Snorm_block.red_1 = -127;

        // Normalize signed int -128..127 to float -1..1
        CGU_Vec2f alphaMinMax;
        alphaMinMax.x = (CGU_FLOAT)(BC4_Snorm_block.red_0) / 127.0f;
        alphaMinMax.y = (CGU_FLOAT)(BC4_Snorm_block.red_1) / 127.0f;

        BC4_Snorm_block.data = cmp_getBlockPackedIndicesSNorm(alphaMinMax, alphaBlock, BC4_Snorm_block.data);
        CmpBlock.x           = BC4_Snorm_block.compressedBlock[0];
        CmpBlock.y           = BC4_Snorm_block.compressedBlock[1];
#else
        CGU_Vec2f RampMinMax;
        RampMinMax = cmp_getLinearEndPoints(alphaBlock, fquality, false);  // revert code to remove the false param
        CmpBlock   = cmp_getBlockPackedIndices(RampMinMax, alphaBlock, fquality);
#endif
    }
    else
    {
        CGU_Vec2f RampMinMax;
        RampMinMax = cmp_getLinearEndPoints(alphaBlock, fquality, false);  // revert code to remove the false param
        CmpBlock   = cmp_getBlockPackedIndices(RampMinMax, alphaBlock, fquality);
    }

    return CmpBlock;
}

static void cmp_getCompressedAlphaRamp(CGU_UINT8 alpha[8], const CGU_UINT32 compressedBlock[2])
{
    alpha[0] = (CGU_UINT8)(compressedBlock[0] & 0xff);
    alpha[1] = (CGU_UINT8)((compressedBlock[0] >> 8) & 0xff);

    if (alpha[0] > alpha[1])
    {
        // 8-alpha block:  derive the other six alphas.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
#ifdef ASPM_GPU
        alpha[2] = (CGU_UINT8)((6 * alpha[0] + 1 * alpha[1] + 3) / 7);  // bit code 010
        alpha[3] = (CGU_UINT8)((5 * alpha[0] + 2 * alpha[1] + 3) / 7);  // bit code 011
        alpha[4] = (CGU_UINT8)((4 * alpha[0] + 3 * alpha[1] + 3) / 7);  // bit code 100
        alpha[5] = (CGU_UINT8)((3 * alpha[0] + 4 * alpha[1] + 3) / 7);  // bit code 101
        alpha[6] = (CGU_UINT8)((2 * alpha[0] + 5 * alpha[1] + 3) / 7);  // bit code 110
        alpha[7] = (CGU_UINT8)((1 * alpha[0] + 6 * alpha[1] + 3) / 7);  // bit code 111
#else
        alpha[2]   = static_cast<CGU_UINT8>((6 * alpha[0] + 1 * alpha[1] + 3) / 7);  // bit code 010
        alpha[3]   = static_cast<CGU_UINT8>((5 * alpha[0] + 2 * alpha[1] + 3) / 7);  // bit code 011
        alpha[4]   = static_cast<CGU_UINT8>((4 * alpha[0] + 3 * alpha[1] + 3) / 7);  // bit code 100
        alpha[5]   = static_cast<CGU_UINT8>((3 * alpha[0] + 4 * alpha[1] + 3) / 7);  // bit code 101
        alpha[6]   = static_cast<CGU_UINT8>((2 * alpha[0] + 5 * alpha[1] + 3) / 7);  // bit code 110
        alpha[7]   = static_cast<CGU_UINT8>((1 * alpha[0] + 6 * alpha[1] + 3) / 7);  // bit code 111
#endif
    }
    else
    {
        // 6-alpha block.
        // Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
#ifdef ASPM_GPU
        alpha[2] = (CGU_UINT8)((4 * alpha[0] + 1 * alpha[1] + 2) / 5);  // Bit code 010
        alpha[3] = (CGU_UINT8)((3 * alpha[0] + 2 * alpha[1] + 2) / 5);  // Bit code 011
        alpha[4] = (CGU_UINT8)((2 * alpha[0] + 3 * alpha[1] + 2) / 5);  // Bit code 100
        alpha[5] = (CGU_UINT8)((1 * alpha[0] + 4 * alpha[1] + 2) / 5);  // Bit code 101
#else
        alpha[2]   = static_cast<CGU_UINT8>((4 * alpha[0] + 1 * alpha[1] + 2) / 5);  // Bit code 010
        alpha[3]   = static_cast<CGU_UINT8>((3 * alpha[0] + 2 * alpha[1] + 2) / 5);  // Bit code 011
        alpha[4]   = static_cast<CGU_UINT8>((2 * alpha[0] + 3 * alpha[1] + 2) / 5);  // Bit code 100
        alpha[5]   = static_cast<CGU_UINT8>((1 * alpha[0] + 4 * alpha[1] + 2) / 5);  // Bit code 101
#endif
        alpha[6] = 0;    // Bit code 110
        alpha[7] = 255;  // Bit code 111
    }
}

static void cmp_decompressAlphaBlock(CGU_UINT8 alphaBlock[BLOCK_SIZE_4X4], const CGU_UINT32 compressedBlock[2])
{
    CGU_UINT32 i;
    CGU_UINT8  alpha[8];
    cmp_getCompressedAlphaRamp(alpha, compressedBlock);

    for (i = 0; i < BLOCK_SIZE_4X4; i++)
    {
        CGU_UINT32 index;
        if (i < 5)
            index = (compressedBlock[0] & (0x7 << (16 + (i * 3)))) >> (16 + (i * 3));
        else if (i > 5)
            index = (compressedBlock[1] & (0x7 << (2 + (i - 6) * 3))) >> (2 + (i - 6) * 3);
        else
        {
            index = (compressedBlock[0] & 0x80000000) >> 31;
            index |= (compressedBlock[1] & 0x3) << 1;
        }

        alphaBlock[i] = alpha[index];
    }
}

static void cmp_ProcessColors(CMP_INOUT CGU_Vec3f CMP_PTRINOUT colorMin,
                              CMP_INOUT CGU_Vec3f CMP_PTRINOUT colorMax,
                              CMP_INOUT CGU_UINT32 CMP_PTRINOUT c0,
                              CMP_INOUT CGU_UINT32 CMP_PTRINOUT c1,
                              CGU_INT                           setopt,
                              CGU_BOOL                          isSRGB)
{
    // CGU_UINT32 srbMap[32] = {0,5,8,11,12,13,14,15,16,17,18,19,20,21,22,23,23,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31};
    // CGU_UINT32 sgMap[64]  = {0,10,14,16,19,20,22,24,25,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,42,43,43,44,45,45,
    //                          46,47,47,48,48,49,50,50,51,52,52,53,53,54,54,55,55,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,63};
    CGU_INT32 x, y, z;
    CGU_Vec3f scale = {31.0f, 63.0f, 31.0f};
    CGU_Vec3f MinColorScaled;
    CGU_Vec3f MaxColorScaled;

    // Clamp or Transform is needed, the transforms have built in clamps
    if (isSRGB)
    {
        MinColorScaled = cmp_linearToSrgb(CMP_PTRINOUT colorMin);
        MaxColorScaled = cmp_linearToSrgb(CMP_PTRINOUT colorMax);
    }
    else
    {
        MinColorScaled = cmp_clampVec3f(CMP_PTRINOUT colorMin, 0.0f, 1.0f);
        MaxColorScaled = cmp_clampVec3f(CMP_PTRINOUT colorMax, 0.0f, 1.0f);
    }

    switch (setopt)
    {
    case 0:  // Use Min Max processing
        MinColorScaled        = floor(MinColorScaled * scale);
        MaxColorScaled        = ceil(MaxColorScaled * scale);
        CMP_PTRINOUT colorMin = MinColorScaled / scale;
        CMP_PTRINOUT colorMax = MaxColorScaled / scale;
        break;
    default:  // Use round processing
        MinColorScaled = round(MinColorScaled * scale);
        MaxColorScaled = round(MaxColorScaled * scale);
        break;
    }

    x = (CGU_UINT32)(MinColorScaled.x);
    y = (CGU_UINT32)(MinColorScaled.y);
    z = (CGU_UINT32)(MinColorScaled.z);

    //if (isSRGB) {
    //    // scale RB
    //    x = srbMap[x]; // &0x1F];
    //    y = sgMap [y]; // &0x3F];
    //    z = srbMap[z]; // &0x1F];
    //    // scale G
    //}
    CMP_PTRINOUT c0 = (x << 11) | (y << 5) | z;

    x               = (CGU_UINT32)(MaxColorScaled.x);
    y               = (CGU_UINT32)(MaxColorScaled.y);
    z               = (CGU_UINT32)(MaxColorScaled.z);
    CMP_PTRINOUT c1 = (x << 11) | (y << 5) | z;
}

#ifndef ASPM_GPU  // Used by BC1, BC2 & BC3
//----------------------------------------------------
// This function decompresses a DXT colour block
// The block is decompressed to 8 bits per channel
// Result buffer is RGBA format, A is set to 255
//----------------------------------------------------
static void cmp_decompressDXTRGBA_Internal(CGU_UINT8 rgbBlock[BLOCK_SIZE_4X4X4], const CGU_Vec2ui compressedBlock, const CGU_BOOL mapDecodeRGBA)
{
    CGU_BOOL   bDXT1 = TRUE;
    CGU_UINT32 n0    = compressedBlock.x & 0xffff;
    CGU_UINT32 n1    = compressedBlock.x >> 16;
    CGU_UINT32 r0;
    CGU_UINT32 g0;
    CGU_UINT32 b0;
    CGU_UINT32 r1;
    CGU_UINT32 g1;
    CGU_UINT32 b1;

    r0 = ((n0 & 0xf800) >> 8);
    g0 = ((n0 & 0x07e0) >> 3);
    b0 = ((n0 & 0x001f) << 3);

    r1 = ((n1 & 0xf800) >> 8);
    g1 = ((n1 & 0x07e0) >> 3);
    b1 = ((n1 & 0x001f) << 3);

    // Apply the lower bit replication to give full dynamic range
    r0 += (r0 >> 5);
    r1 += (r1 >> 5);
    g0 += (g0 >> 6);
    g1 += (g1 >> 6);
    b0 += (b0 >> 5);
    b1 += (b1 >> 5);

    if (!mapDecodeRGBA)
    {
        //--------------------------------------------------------------
        // Channel mapping output as BGRA
        //--------------------------------------------------------------
        CGU_UINT32 c0 = 0xff000000 | (r0 << 16) | (g0 << 8) | b0;
        CGU_UINT32 c1 = 0xff000000 | (r1 << 16) | (g1 << 8) | b1;

        if (!bDXT1 || n0 > n1)
        {
            CGU_UINT32 c2 = 0xff000000 | (((2 * r0 + r1) / 3) << 16) | (((2 * g0 + g1) / 3) << 8) | (((2 * b0 + b1) / 3));
            CGU_UINT32 c3 = 0xff000000 | (((2 * r1 + r0) / 3) << 16) | (((2 * g1 + g0) / 3) << 8) | (((2 * b1 + b0) / 3));

            for (int i = 0; i < 16; i++)
            {
                int index = (compressedBlock.y >> (2 * i)) & 3;

                switch (index)
                {
                case 0:
                    ((CGU_UINT32*)rgbBlock)[i] = c0;
                    break;
                case 1:
                    ((CGU_UINT32*)rgbBlock)[i] = c1;
                    break;
                case 2:
                    ((CGU_UINT32*)rgbBlock)[i] = c2;
                    break;
                case 3:
                    ((CGU_UINT32*)rgbBlock)[i] = c3;
                    break;
                }
            }
        }
        else
        {
            // Transparent decode
            CGU_UINT32 c2 = 0xff000000 | (((r0 + r1) / 2) << 16) | (((g0 + g1) / 2) << 8) | (((b0 + b1) / 2));

            for (int i = 0; i < 16; i++)
            {
                int index = (compressedBlock.y >> (2 * i)) & 3;

                switch (index)
                {
                case 0:
                    ((CGU_UINT32*)rgbBlock)[i] = c0;
                    break;
                case 1:
                    ((CGU_UINT32*)rgbBlock)[i] = c1;
                    break;
                case 2:
                    ((CGU_UINT32*)rgbBlock)[i] = c2;
                    break;
                case 3:
                    ((CGU_UINT32*)rgbBlock)[i] = 0x00000000;
                    break;
                }
            }
        }
    }
    else
    {
        // MAP_BC15_TO_ABGR
        //--------------------------------------------------------------
        // Channel mapping output as RGBA
        //--------------------------------------------------------------

        CGU_UINT32 c0 = 0xff000000 | (b0 << 16) | (g0 << 8) | r0;
        CGU_UINT32 c1 = 0xff000000 | (b1 << 16) | (g1 << 8) | r1;

        if (!bDXT1 || n0 > n1)
        {
            CGU_UINT32 c2 = 0xff000000 | (((2 * b0 + b1 + 1) / 3) << 16) | (((2 * g0 + g1 + 1) / 3) << 8) | (((2 * r0 + r1 + 1) / 3));
            CGU_UINT32 c3 = 0xff000000 | (((2 * b1 + b0 + 1) / 3) << 16) | (((2 * g1 + g0 + 1) / 3) << 8) | (((2 * r1 + r0 + 1) / 3));

            for (int i = 0; i < 16; i++)
            {
                int index = (compressedBlock.y >> (2 * i)) & 3;
                switch (index)
                {
                case 0:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c0;
                    break;
                case 1:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c1;
                    break;
                case 2:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c2;
                    break;
                case 3:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c3;
                    break;
                }
            }
        }
        else
        {
            // Transparent decode
            CGU_UINT32 c2 = 0xff000000 | (((b0 + b1) / 2) << 16) | (((g0 + g1) / 2) << 8) | (((r0 + r1) / 2));

            for (int i = 0; i < 16; i++)
            {
                int index = (compressedBlock.y >> (2 * i)) & 3;
                switch (index)
                {
                case 0:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c0;
                    break;
                case 1:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c1;
                    break;
                case 2:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = c2;
                    break;
                case 3:
                    ((CMP_GLOBAL CGU_UINT32*)rgbBlock)[i] = 0x00000000;
                    break;
                }
            }
        }
    }  //MAP_ABGR
}
#endif  // !ASPM_GPU

//--------------------------------------------------------------------------------------------------------
// Decompress is RGB (0.0f..255.0f)
//--------------------------------------------------------------------------------------------------------
static void cmp_decompressRGBBlock(CMP_INOUT CGU_Vec3f rgbBlock[BLOCK_SIZE_4X4], const CGU_Vec2ui compressedBlock)
{
    CGU_UINT32 n0 = compressedBlock.x & 0xffff;
    CGU_UINT32 n1 = compressedBlock.x >> 16;
    CGU_UINT32 index;

    //-------------------------------------------------------
    // Decode the compressed block 0..255 color range
    //-------------------------------------------------------
    CGU_Vec3f c0 = cmp_565ToLinear(n0);  // max color
    CGU_Vec3f c1 = cmp_565ToLinear(n1);  // min color
    CGU_Vec3f c2;
    CGU_Vec3f c3;

    if (n0 > n1)
    {
        c2 = (c0 * 2.0f + c1) / 3.0f;
        c3 = (c1 * 2.0f + c0) / 3.0f;

        for (CGU_UINT32 i = 0; i < 16; i++)
        {
            index = (compressedBlock.y >> (2 * i)) & 3;
            switch (index)
            {
            case 0:
                rgbBlock[i] = c0;
                break;
            case 1:
                rgbBlock[i] = c1;
                break;
            case 2:
                rgbBlock[i] = c2;
                break;
            case 3:
                rgbBlock[i] = c3;
                break;
            }
        }
    }
    else
    {
        // Transparent decode
        c2 = (c0 + c1) / 2.0f;

        for (CGU_UINT32 i = 0; i < 16; i++)
        {
            index = (compressedBlock.y >> (2 * i)) & 3;
            switch (index)
            {
            case 0:
                rgbBlock[i] = c0;
                break;
            case 1:
                rgbBlock[i] = c1;
                break;
            case 2:
                rgbBlock[i] = c2;
                break;
            case 3:
                rgbBlock[i] = 0.0f;
                break;
            }
        }
    }
}

// The source is 0..1, decompressed data using cmp_decompressRGBBlock is 0..255 which is converted down to 0..1
static float CMP_RGBBlockError(const CGU_Vec3f src_rgbBlock[BLOCK_SIZE_4X4], const CGU_Vec2ui compressedBlock, CGU_BOOL isSRGB)
{
    CGU_Vec3f rgbBlock[BLOCK_SIZE_4X4];

    // Decompressed block channels are 0..255
    cmp_decompressRGBBlock(rgbBlock, compressedBlock);

    //------------------------------------------------------------------
    // Calculate MSE of the block
    // Note : pow is used as Float type for the code to be usable on CPU
    //------------------------------------------------------------------
    CGU_Vec3f serr;
    serr = 0.0f;

    float sR, sG, sB, R, G, B;

    for (int j = 0; j < 16; j++)
    {
        if (isSRGB)
        {
            sR = round(cmp_linearToSrgbf(src_rgbBlock[j].x) * 255.0f);
            sG = round(cmp_linearToSrgbf(src_rgbBlock[j].y) * 255.0f);
            sB = round(cmp_linearToSrgbf(src_rgbBlock[j].z) * 255.0f);
        }
        else
        {
            sR = round(src_rgbBlock[j].x * 255.0f);
            sG = round(src_rgbBlock[j].y * 255.0f);
            sB = round(src_rgbBlock[j].z * 255.0f);
        }

        rgbBlock[j] = rgbBlock[j];

        R = rgbBlock[j].x;
        G = rgbBlock[j].y;
        B = rgbBlock[j].z;

        // Norm colors
        serr.x += pow(sR - R, 2.0f);
        serr.y += pow(sG - G, 2.0f);
        serr.z += pow(sB - B, 2.0f);
    }

    // MSE for 16 texels
    return (serr.x + serr.y + serr.z) / 48.0f;
}

// Processing input source 0..1.0f)
static CGU_Vec2ui CompressRGBBlock_FM(const CGU_Vec3f rgbBlockUVf[16], CMP_IN CGU_FLOAT fquality, CGU_BOOL isSRGB, CMP_INOUT CGU_FLOAT CMP_PTRINOUT errout)
{
    CGU_Vec3f  axisVectorRGB = {0.0f, 0.0f, 0.0f};  // The axis vector for index projection
    CGU_FLOAT  pos_on_axis[16];                     // The distance each unique falls along the compression axis
    CGU_FLOAT  axisleft   = 0;                      // The extremities and centre (average of left/right) of srcRGB along the compression axis
    CGU_FLOAT  axisright  = 0;                      // The extremities and centre (average of left/right) of srcRGB along the compression axis
    CGU_FLOAT  axiscentre = 0;                      // The extremities and centre (average of left/right) of srcRGB along the compression axis
    CGU_INT32  swap       = 0;                      // Indicator if the RGB values need swapping to generate an opaque result
    CGU_Vec3f  average_rgb;                         // The centrepoint of the axis
    CGU_Vec3f  srcRGB[16];                          // The list of source colors with blue channel altered
    CGU_Vec3f  srcBlock[16];                        // The list of source colors with any color space transforms and clipping
    CGU_Vec3f  rgb;
    CGU_UINT32 c0              = 0;
    CGU_UINT32 c1              = 0;
    CGU_Vec2ui compressedBlock = {0, 0};
    CGU_FLOAT  Q1CompErr = CMP_FLT_MAX;
    CGU_Vec2ui Q1CompData = {0,0};

    // -------------------------------------------------------------------------------------
    // (1) Find the array of unique pixel values and sum them to find their average position
    // -------------------------------------------------------------------------------------
    {
        CGU_FLOAT  errLQ             = 0.0f;
        CGU_BOOL   fastProcess       = (fquality <= CMP_QUALITY1);
        CGU_Vec3f  srcMin            = 1.0f;  // Min source color
        CGU_Vec3f  srcMax            = 0.0f;  // Max source color
        CGU_Vec2ui Q1compressedBlock = {0, 0};

        average_rgb = 0.0f;
        // Get average and modifed src
        // find average position and save list of pixels as 0F..255F range for processing
        // Note: z (blue) is average of blue+green channels
        for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            srcMin = cmp_minVec3f(srcMin, rgbBlockUVf[i]);
            srcMax = cmp_maxVec3f(srcMax, rgbBlockUVf[i]);
            if (!fastProcess)
            {
                rgb         = isSRGB ? cmp_linearToSrgb(rgbBlockUVf[i]) : cmp_saturate(rgbBlockUVf[i]);
                rgb.z       = (rgb.y + rgb.z) * 0.5F;  // Z-axiz => (R+G)/2
                srcRGB[i]   = rgb;
                average_rgb = average_rgb + rgb;
            }
        }

        // Process two colors for saving in 565 format as C0 and C1
        cmp_ProcessColors(CMP_REFINOUT srcMin, CMP_REFINOUT srcMax, CMP_REFINOUT c0, CMP_REFINOUT c1, isSRGB ? 1 : 0, isSRGB);

        // Save simple min-max encoding
        CGU_UINT32 index = 0;
        if (c0 < c1)
        {
            Q1CompData.x = (c0 << 16) | c1;
            errLQ               = cmp_getIndicesRGB(CMP_REFINOUT index, rgbBlockUVf, srcMin, srcMax, false);
            Q1CompData.y        = index;
            CMP_PTRINOUT errout = errLQ;
        }
        else
        {
            // Most simple case all colors are equal or 0.0f
            Q1compressedBlock.x = (c1 << 16) | c0;
            Q1compressedBlock.y = 0;
            CMP_PTRINOUT errout = 0.0f;
            return Q1compressedBlock;
        }

        if (fastProcess)
            return Q1CompData;

        // 0.0625F is (1/BLOCK_SIZE_4X4)
        average_rgb = average_rgb * 0.0625F;
    }

    // -------------------------------------------------------------------------------------
    // (4) For each component, reflect points about the average so all lie on the same side
    // of the average, and compute the new average - this gives a second point that defines the axis
    // To compute the sign of the axis sum the positive differences of G for each of R and B (the
    // G axis is always positive in this implementation
    // -------------------------------------------------------------------------------------
    // An interesting situation occurs if the G axis contains no information, in which case the RB
    // axis is also compared. I am not entirely sure if this is the correct implementation - should
    // the priority axis be determined by magnitude?
    {
        CGU_FLOAT rg_pos = 0.0f;
        CGU_FLOAT bg_pos = 0.0f;
        CGU_FLOAT rb_pos = 0.0f;

        for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            rgb           = srcRGB[i] - average_rgb;
            axisVectorRGB = axisVectorRGB + fabs(rgb);
            if (rgb.x > 0)
            {
                rg_pos += rgb.y;
                rb_pos += rgb.z;
            }
            if (rgb.z > 0)
                bg_pos += rgb.y;
        }

        // Average over BLOCK_SIZE_4X4
        axisVectorRGB = axisVectorRGB * 0.0625F;

        // New average position
        if (rg_pos < 0)
            axisVectorRGB.x = -axisVectorRGB.x;
        if (bg_pos < 0)
            axisVectorRGB.z = -axisVectorRGB.z;
        if ((rg_pos == bg_pos) && (rg_pos == 0))
        {
            if (rb_pos < 0)
                axisVectorRGB.z = -axisVectorRGB.z;
        }
    }

    // -------------------------------------------------------------------------------------
    // (5) Axis projection and remapping
    // -------------------------------------------------------------------------------------
    {
        CGU_FLOAT v2_recip;
        // Normalize the axis for simplicity of future calculation
        v2_recip = dot(axisVectorRGB, axisVectorRGB);
        if (v2_recip > 0)
            v2_recip = 1.0f / (CGU_FLOAT)sqrt(v2_recip);
        else
            v2_recip = 1.0f;
        axisVectorRGB = axisVectorRGB * v2_recip;
    }

    // -------------------------------------------------------------------------------------
    // (6) Map the axis
    // -------------------------------------------------------------------------------------
    // the line joining (and extended on either side of) average and axis
    // defines the axis onto which the points will be projected
    // Project all the points onto the axis, calculate the distance along
    // the axis from the centre of the axis (average)
    // From Foley & Van Dam: Closest point of approach of a line (P + v) to a point (R) is
    //     P + ((R-P).v) / (v.v))v
    // The distance along v is therefore (R-P).v / (v.v) where (v.v) is 1 if v is a unit vector.
    //
    // Calculate the extremities at the same time - these need to be reasonably accurately
    // represented in all cases
    {
        axisleft  = CMP_FLOAT_MAX;
        axisright = -CMP_FLOAT_MAX;
        for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            // Compute the distance along the axis of the point of closest approach
            CGU_Vec3f temp = (srcRGB[i] - average_rgb);
            pos_on_axis[i] = dot(temp, axisVectorRGB);

            // Work out the extremities
            if (pos_on_axis[i] < axisleft)
                axisleft = pos_on_axis[i];
            if (pos_on_axis[i] > axisright)
                axisright = pos_on_axis[i];
        }
    }

    // ---------------------------------------------------------------------------------------------
    // (7) Now we have a good axis and the basic information about how the points are mapped to it
    // Our initial guess is to represent the endpoints accurately, by moving the average
    // to the centre and recalculating the point positions along the line
    // ---------------------------------------------------------------------------------------------
    {
        axiscentre  = (axisleft + axisright) * 0.5F;
        average_rgb = average_rgb + (axisVectorRGB * axiscentre);
        for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
            pos_on_axis[i] -= axiscentre;
        axisright -= axiscentre;
        axisleft -= axiscentre;
    }

    // -------------------------------------------------------------------------------------
    // (8) Calculate the high and low output colour values
    // Involved in this is a rounding procedure which is undoubtedly slightly twitchy. A
    // straight rounded average is not correct, as the decompressor 'unrounds' by replicating
    // the top bits to the bottom.
    // In order to take account of this process, we don't just apply a straight rounding correction,
    // but base our rounding on the input value (a straight rounding is actually pretty good in terms of
    // error measure, but creates a visual colour and/or brightness shift relative to the original image)
    // The method used here is to apply a centre-biased rounding dependent on the input value, which was
    // (mostly by experiment) found to give minimum MSE while preserving the visual characteristics of
    // the image.
    // rgb = (average_rgb + (left|right)*axisVectorRGB);
    // -------------------------------------------------------------------------------------
    {
        CGU_Vec3f MinColor, MaxColor;

        MinColor   = average_rgb + (axisVectorRGB * axisleft);
        MaxColor   = average_rgb + (axisVectorRGB * axisright);
        MinColor.z = (MinColor.z * 2) - MinColor.y;
        MaxColor.z = (MaxColor.z * 2) - MaxColor.y;

        cmp_ProcessColors(CMP_REFINOUT MinColor, CMP_REFINOUT MaxColor, CMP_REFINOUT c0, CMP_REFINOUT c1, 1, false);

        // Force to be a 4-colour opaque block - in which case, c0 is greater than c1
        swap = 0;
        if (c0 < c1)
        {
            CGU_UINT32 t;
            t    = c0;
            c0   = c1;
            c1   = t;
            swap = 1;
        }
        else if (c0 == c1)
        {
            // This block will always be encoded in 3-colour mode
            // Need to ensure that only one of the two points gets used,
            // avoiding accidentally setting some transparent pixels into the block
            for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
                pos_on_axis[i] = axisleft;
        }

        compressedBlock.x = c0 | (c1 << 16);

        // -------------------------------------------------------------------------------------
        // (9) Final clustering, creating the 2-bit values that define the output
        // -------------------------------------------------------------------------------------

        CGU_UINT32 index;
        CGU_FLOAT  division;
        {
            compressedBlock.y = 0;
            division          = axisright * 2.0f / 3.0f;
            axiscentre        = (axisleft + axisright) / 2;  // Actually, this code only works if centre is 0 or approximately so

            CGU_FLOAT CompMinErr;

            // This feature is work in progress
            // remap to BC1 spec for decoding offsets,
            // where cn[0] > cn[1] Max Color = index 0, 2/3 offset =index 2, 1/3 offset = index 3, Min Color = index 1
            // CGU_Vec3f   cn[4];
            // cn[0] = MaxColor;
            // cn[1] = MinColor;
            // cn[2] = cn[0]*2.0f/3.0f + cn[1]*1.0f/3.0f;
            // cn[3] = cn[0]*1.0f/3.0f + cn[1]*2.0f/3.0f;

            for (CGU_INT32 i = 0; i < BLOCK_SIZE_4X4; i++)
            {
                // Endpoints (indicated by block > average) are 0 and 1, while
                // interpolants are 2 and 3
                if (fabs(pos_on_axis[i]) >= division)
                    index = 0;
                else
                    index = 2;
                // Positive is in the latter half of the block
                if (pos_on_axis[i] >= axiscentre)
                    index += 1;

                index = index ^ swap;
                // Set the output, taking swapping into account
                compressedBlock.y |= (index << (2 * i));

                // use err calc for use in higher quality code
                //CompMinErr += dot(srcRGBRef[i] - cn[index],srcRGBRef[i] - cn[index]);
            }

            //CompMinErr = CompMinErr * 0.0208333f;

            CompMinErr = CMP_RGBBlockError(rgbBlockUVf, compressedBlock, isSRGB);
            Q1CompErr  = CMP_RGBBlockError(rgbBlockUVf, Q1CompData, isSRGB);

            if (CompMinErr > Q1CompErr)
            {
                compressedBlock     = Q1CompData;
                CMP_PTRINOUT errout = Q1CompErr;
            }
            else
                CMP_PTRINOUT errout = CompMinErr;
        }
    }
    // done

    return compressedBlock;
}

#ifndef CMP_USE_LOWQUALITY
static CMP_EndPoints CompressRGBBlock_Slow(CGU_Vec3f  BlkInBGRf_UV[BLOCK_SIZE_4X4],
                                           CGU_FLOAT  Rpt[BLOCK_SIZE_4X4],
                                           CGU_UINT32 dwUniqueColors,
                                           CGU_Vec3f  channelWeightsBGR,
                                           CGU_UINT32 m_nRefinementSteps)
{
    CMP_UNUSED(channelWeightsBGR);
    CMP_UNUSED(m_nRefinementSteps);
    ALIGN_16 CGU_FLOAT Prj0[BLOCK_SIZE_4X4];
    ALIGN_16 CGU_FLOAT Prj[BLOCK_SIZE_4X4];
    ALIGN_16 CGU_FLOAT PrjErr[BLOCK_SIZE_4X4];
    ALIGN_16 CGU_FLOAT RmpIndxs[BLOCK_SIZE_4X4];

    CGU_Vec3f LineDirG;
    CGU_Vec3f LineDir;
    CGU_FLOAT LineDir0[NUM_CHANNELS];
    CGU_Vec3f BlkUV[BLOCK_SIZE_4X4];
    CGU_Vec3f BlkSh[BLOCK_SIZE_4X4];
    CGU_Vec3f Mdl;

    CGU_Vec3f  rsltC0;
    CGU_Vec3f  rsltC1;
    CGU_Vec3f  PosG0 = {0.0f, 0.0f, 0.0f};
    CGU_Vec3f  PosG1 = {0.0f, 0.0f, 0.0f};
    CGU_UINT32 i;

    for (i = 0; i < dwUniqueColors; i++)
    {
        BlkUV[i] = BlkInBGRf_UV[i];
    }

    // if not more then 2 different colors, we've done
    if (dwUniqueColors <= 2)
    {
        rsltC0 = BlkInBGRf_UV[0] * 255.0f;
        rsltC1 = BlkInBGRf_UV[dwUniqueColors - 1] * 255.0f;
    }
    else
    {
        //    This is our first attempt to find an axis we will go along.
        //    The cumulation is done to find a line minimizing the MSE from the
        //    input 3D points.

        //    While trying to find the axis we found that the diameter of the input
        //    set is quite small. Do not bother.

        // FindAxisIsSmall(BlkSh, LineDir0, Mdl, Blk, Rpt,dwUniqueColors);
        {
            CGU_UINT32 ii;
            CGU_UINT32 jj;
            CGU_UINT32 kk;

            // These vars cannot be Vec3 as index to them are varying
            CGU_FLOAT Crrl[NUM_CHANNELS];
            CGU_FLOAT RGB2[NUM_CHANNELS];

            LineDir0[0] = LineDir0[1] = LineDir0[2] = RGB2[0] = RGB2[1] = RGB2[2] = Crrl[0] = Crrl[1] = Crrl[2] = Mdl.x = Mdl.y = Mdl.z = 0.f;

            // sum position of all points
            CGU_FLOAT fNumPoints = 0.0f;
            for (ii = 0; ii < dwUniqueColors; ii++)
            {
                Mdl.x += BlkUV[ii].x * Rpt[ii];
                Mdl.y += BlkUV[ii].y * Rpt[ii];
                Mdl.z += BlkUV[ii].z * Rpt[ii];
                fNumPoints += Rpt[ii];
            }

            // and then average to calculate center coordinate of block
            Mdl /= fNumPoints;

            for (ii = 0; ii < dwUniqueColors; ii++)
            {
                // calculate output block as offsets around block center
                BlkSh[ii] = BlkUV[ii] - Mdl;

                // compute correlation matrix
                // RGB2 = sum of ((distance from point from center) squared)
                RGB2[0] += BlkSh[ii].x * BlkSh[ii].x * Rpt[ii];
                RGB2[1] += BlkSh[ii].y * BlkSh[ii].y * Rpt[ii];
                RGB2[2] += BlkSh[ii].z * BlkSh[ii].z * Rpt[ii];

                Crrl[0] += BlkSh[ii].x * BlkSh[ii].y * Rpt[ii];
                Crrl[1] += BlkSh[ii].y * BlkSh[ii].z * Rpt[ii];
                Crrl[2] += BlkSh[ii].z * BlkSh[ii].x * Rpt[ii];
            }

            // if set's diameter is small
            CGU_UINT32 i0 = 0, i1 = 1;
            CGU_FLOAT  mxRGB2 = 0.0f;

            CGU_FLOAT fEPS = fNumPoints * EPS;
            for (kk = 0, jj = 0; jj < 3; jj++)
            {
                if (RGB2[jj] >= fEPS)
                    kk++;
                else
                    RGB2[jj] = 0.0f;

                if (mxRGB2 < RGB2[jj])
                {
                    mxRGB2 = RGB2[jj];
                    i0     = jj;
                }
            }

            CGU_FLOAT fEPS2 = fNumPoints * EPS2;
            CGU_BOOL  AxisIsSmall;

            AxisIsSmall = (RGB2[0] < fEPS2);
            AxisIsSmall = AxisIsSmall && (RGB2[1] < fEPS2);
            AxisIsSmall = AxisIsSmall && (RGB2[2] < fEPS2);

            // all are very small to avoid division on the small determinant
            if (AxisIsSmall)
            {
                rsltC0 = BlkInBGRf_UV[0] * 255.0f;
                rsltC1 = BlkInBGRf_UV[dwUniqueColors - 1] * 255.0f;
            }
            else
            {
                // !AxisIsSmall
                if (kk == 1)  // really only 1 dimension
                    LineDir0[i0] = 1.;
                else if (kk == 2)
                {  // really only 2 dimensions
                    i1            = (RGB2[(i0 + 1) % 3] > 0.f) ? (i0 + 1) % 3 : (i0 + 2) % 3;
                    CGU_FLOAT Crl = (i1 == (i0 + 1) % 3) ? Crrl[i0] : Crrl[(i0 + 2) % 3];
                    LineDir0[i1]  = Crl / RGB2[i0];
                    LineDir0[i0]  = 1.;
                }
                else
                {
                    CGU_FLOAT maxDet = 100000.f;
                    CGU_FLOAT Cs[3];
                    // select max det for precision
                    for (jj = 0; jj < 3; jj++)
                    {
                        // 3 = nDimensions
                        CGU_FLOAT Det = RGB2[jj] * RGB2[(jj + 1) % 3] - Crrl[jj] * Crrl[jj];
                        Cs[jj]        = fabs(Crrl[jj] / sqrt(RGB2[jj] * RGB2[(jj + 1) % 3]));
                        if (maxDet < Det)
                        {
                            maxDet = Det;
                            i0     = jj;
                        }
                    }

                    // inverse correl matrix
                    //  --      --       --      --
                    //  |  A   B |       |  C  -B |
                    //  |  B   C |  =>   | -B   A |
                    //  --      --       --     --
                    CGU_FLOAT mtrx1[2][2];
                    CGU_FLOAT vc1[2];
                    CGU_FLOAT vc[2];
                    vc1[0] = Crrl[(i0 + 2) % 3];
                    vc1[1] = Crrl[(i0 + 1) % 3];
                    // C
                    mtrx1[0][0] = RGB2[(i0 + 1) % 3];
                    // A
                    mtrx1[1][1] = RGB2[i0];
                    // -B
                    mtrx1[1][0] = mtrx1[0][1] = -Crrl[i0];
                    // find a solution
                    vc[0] = mtrx1[0][0] * vc1[0] + mtrx1[0][1] * vc1[1];
                    vc[1] = mtrx1[1][0] * vc1[0] + mtrx1[1][1] * vc1[1];
                    // normalize
                    vc[0] /= maxDet;
                    vc[1] /= maxDet;
                    // find a line direction vector
                    LineDir0[i0]           = 1.;
                    LineDir0[(i0 + 1) % 3] = 1.;
                    LineDir0[(i0 + 2) % 3] = vc[0] + vc[1];
                }

                // normalize direction vector
                CGU_FLOAT Len = LineDir0[0] * LineDir0[0] + LineDir0[1] * LineDir0[1] + LineDir0[2] * LineDir0[2];
                Len           = sqrt(Len);

                LineDir0[0] = (Len > 0.f) ? LineDir0[0] / Len : 0.0f;
                LineDir0[1] = (Len > 0.f) ? LineDir0[1] / Len : 0.0f;
                LineDir0[2] = (Len > 0.f) ? LineDir0[2] / Len : 0.0f;
            }
        }  // FindAxisIsSmall

        // GCC is being an awful being when it comes to goto-jumps.
        // So please bear with this.
        CGU_FLOAT ErrG = 10000000.f;
        CGU_FLOAT PrjBnd0;
        CGU_FLOAT PrjBnd1;
        ALIGN_16 CGU_FLOAT PreMRep[BLOCK_SIZE_4X4];

        LineDir.x = LineDir0[0];
        LineDir.y = LineDir0[1];
        LineDir.z = LineDir0[2];

        //    Here is the main loop.
        //    1. Project input set on the axis in consideration.
        //    2. Run 1 dimensional search (see scalar case) to find an (sub) optimal pair of end points.
        //    3. Compute the vector of indexes (or clusters) for the current approximate ramp.
        //    4. Present our color channels as 3 16DIM vectors.
        //    5. Find closest approximation of each of 16DIM color vector with the projection of the 16DIM index vector.
        //    6. Plug the projections as a new directional vector for the axis.
        //    7. Goto 1.
        //    D - is 16 dim "index" vector (or 16 DIM vector of indexes - {0, 1/3,2/3, 0, ...,}, but shifted and normalized).
        //    Ci - is a 16 dim vector of color i. for each Ci find a scalar Ai such that (Ai * D - Ci) (Ai * D - Ci) -> min ,
        //         i.e distance between vector AiD and C is min. You can think of D as a unit interval(vector) "clusterizer", and Ai is a scale
        //         you need to apply to the clusterizer to approximate the Ci vector instead of the unit vector.
        //    Solution is
        //    Ai = (D . Ci) / (D . D); . - is a dot product.
        //    in 3 dim space Ai(s) represent a line direction, along which
        //    we again try to find (sub)optimal quantizer.
        //    That's what our for(;;) loop is about.
        for (;;)
        {
            //  1. Project input set on the axis in consideration.
            // From Foley & Van Dam: Closest point of approach of a line (P + v) to a
            // point (R) is
            //                            P + ((R-P).v) / (v.v))v
            // The distance along v is therefore (R-P).v / (v.v)
            // (v.v) is 1 if v is a unit vector.
            //
            PrjBnd0 = 1000.0f;
            PrjBnd1 = -1000.0f;
            for (i = 0; i < BLOCK_SIZE_4X4; i++)
                Prj0[i] = Prj[i] = PrjErr[i] = PreMRep[i] = 0.f;

            for (i = 0; i < dwUniqueColors; i++)
            {
                Prj0[i] = Prj[i] = dot(BlkSh[i], LineDir);
                PrjErr[i]        = dot(BlkSh[i] - LineDir * Prj[i], BlkSh[i] - LineDir * Prj[i]);
                PrjBnd0          = min(PrjBnd0, Prj[i]);
                PrjBnd1          = max(PrjBnd1, Prj[i]);
            }

            //  2. Run 1 dimensional search (see scalar case) to find an (sub) optimal
            //  pair of end points.

            // min and max of the search interval
            CGU_FLOAT Scl0;
            CGU_FLOAT Scl1;
            Scl0 = PrjBnd0 - (PrjBnd1 - PrjBnd0) * 0.125f;
            Scl1 = PrjBnd1 + (PrjBnd1 - PrjBnd0) * 0.125f;

            // compute scaling factor to scale down the search interval to [0.,1]
            const CGU_FLOAT Scl2    = (Scl1 - Scl0) * (Scl1 - Scl0);
            const CGU_FLOAT overScl = 1.f / (Scl1 - Scl0);

            for (i = 0; i < dwUniqueColors; i++)
            {
                // scale them
                Prj[i] = (Prj[i] - Scl0) * overScl;
                // premultiply the scale square to plug into error computation later
                PreMRep[i] = Rpt[i] * Scl2;
            }

            // scale first approximation of end points
            PrjBnd0 = (PrjBnd0 - Scl0) * overScl;
            PrjBnd1 = (PrjBnd1 - Scl0) * overScl;

            CGU_FLOAT StepErr = MAX_ERROR;

            // search step
            CGU_FLOAT searchStep = 0.025f;

            // low Start/End; high Start/End
            const CGU_FLOAT lowStartEnd  = (PrjBnd0 - 2.f * searchStep > 0.f) ? PrjBnd0 - 2.f * searchStep : 0.f;
            const CGU_FLOAT highStartEnd = (PrjBnd1 + 2.f * searchStep < 1.f) ? PrjBnd1 + 2.f * searchStep : 1.f;

            // find the best endpoints
            CGU_FLOAT Pos0 = 0;
            CGU_FLOAT Pos1 = 0;
            CGU_FLOAT lowPosStep, highPosStep;
            CGU_FLOAT err;

            int l, h;
            for (l = 0, lowPosStep = lowStartEnd; l < 8; l++, lowPosStep += searchStep)
            {
                for (h = 0, highPosStep = highStartEnd; h < 8; h++, highPosStep -= searchStep)
                {
                    // compute an error for the current pair of end points.
                    err = cmp_getRampErr(Prj, PrjErr, PreMRep, StepErr, lowPosStep, highPosStep, dwUniqueColors);

                    if (err < StepErr)
                    {
                        // save better result
                        StepErr = err;
                        Pos0    = lowPosStep;
                        Pos1    = highPosStep;
                    }
                }
            }

            // inverse the scaling
            Pos0 = Pos0 * (Scl1 - Scl0) + Scl0;
            Pos1 = Pos1 * (Scl1 - Scl0) + Scl0;

            // did we find somthing better from the previous run?
            if (StepErr + 0.001 < ErrG)
            {
                // yes, remember it
                ErrG     = StepErr;
                LineDirG = LineDir;

                PosG0.x = Pos0;
                PosG0.y = Pos0;
                PosG0.z = Pos0;
                PosG1.x = Pos1;
                PosG1.y = Pos1;
                PosG1.z = Pos1;

                //  3. Compute the vector of indexes (or clusters) for the current
                //  approximate ramp.
                // indexes
                const CGU_FLOAT step      = (Pos1 - Pos0) / 3.0f;  // (dwNumChannels=4 - 1);
                const CGU_FLOAT step_h    = step * (CGU_FLOAT)0.5;
                const CGU_FLOAT rstep     = (CGU_FLOAT)1.0f / step;
                const CGU_FLOAT overBlkTp = 1.f / 3.0f;  // (dwNumChannels=4 - 1);

                // here the index vector is computed,
                // shifted and normalized
                CGU_FLOAT indxAvrg = 3.0f / 2.0f;  // (dwNumChannels=4 - 1);

                for (i = 0; i < dwUniqueColors; i++)
                {
                    CGU_FLOAT del;
                    // CGU_UINT32 n = (CGU_UINT32)((b - _min_ex + (step*0.5f)) * rstep);
                    if ((del = Prj0[i] - Pos0) <= 0)
                        RmpIndxs[i] = 0.f;
                    else if (Prj0[i] - Pos1 >= 0)
                        RmpIndxs[i] = 3.0f;  // (dwNumChannels=4 - 1);
                    else
                        RmpIndxs[i] = floor((del + step_h) * rstep);
                    // shift and normalization
                    RmpIndxs[i] = (RmpIndxs[i] - indxAvrg) * overBlkTp;
                }

                //  4. Present our color channels as 3 16 DIM vectors.
                //  5. Find closest aproximation of each of 16DIM color vector with the
                //  pojection of the 16DIM index vector.
                CGU_Vec3f Crs = {0.0f, 0.0f, 0.0f};
                CGU_FLOAT Len = 0.0f;

                for (i = 0; i < dwUniqueColors; i++)
                {
                    const CGU_FLOAT PreMlt = RmpIndxs[i] * Rpt[i];
                    Len += RmpIndxs[i] * PreMlt;
                    Crs.x += BlkSh[i].x * PreMlt;
                    Crs.y += BlkSh[i].y * PreMlt;
                    Crs.z += BlkSh[i].z * PreMlt;
                }

                LineDir.x = LineDir.y = LineDir.z = 0.0f;
                if (Len > 0.0f)
                {
                    CGU_FLOAT Len2;
                    LineDir = Crs / Len;
                    //  6. Plug the projections as a new directional vector for the axis.
                    //  7. Goto 1.
                    Len2 = dot(LineDir, LineDir);  // LineDir.x * LineDir.x + LineDir.y * LineDir.y + LineDir.z * LineDir.z;
                    Len2 = sqrt(Len2);
                    LineDir /= Len2;
                }
            }
            else  // We was not able to find anything better.  Drop out.
                break;
        }

        // inverse transform to find end-points of 3-color ramp
        rsltC0 = (PosG0 * LineDirG + Mdl) * 255.f;
        rsltC1 = (PosG1 * LineDirG + Mdl) * 255.f;
    }  // !isDone

    // We've dealt with (almost) unrestricted full precision realm.
    // Now back digital world.

    // round the end points to make them look like compressed ones
    CGU_Vec3f inpRmpEndPts0 = {0.0f, 255.0f, 0.0f};
    CGU_Vec3f inpRmpEndPts1 = {0.0f, 255.0f, 0.0f};
    CGU_Vec3f Fctrs0        = {8.0f, 4.0f, 8.0f};     //(1 << (PIX_GRID - BG)); x (1 << (PIX_GRID - GG)); y (1 << (PIX_GRID - RG)); z
    CGU_Vec3f Fctrs1        = {32.0f, 64.0f, 32.0f};  //(CGU_FLOAT)(1 << RG); z (CGU_FLOAT)(1 << GG); y (CGU_FLOAT)(1 << BG); x
    CGU_FLOAT _Min          = 0.0f;
    CGU_FLOAT _Max          = 255.0f;

    {
        // MkRmpOnGrid(inpRmpEndPts, rsltC, _Min, _Max);

        inpRmpEndPts0 = floor(rsltC0);

        if (inpRmpEndPts0.x <= _Min)
            inpRmpEndPts0.x = _Min;
        else
        {
            inpRmpEndPts0.x += floor(128.f / Fctrs1.x) - floor(inpRmpEndPts0.x / Fctrs1.x);
            inpRmpEndPts0.x = min(inpRmpEndPts0.x, _Max);
        }
        if (inpRmpEndPts0.y <= _Min)
            inpRmpEndPts0.y = _Min;
        else
        {
            inpRmpEndPts0.y += floor(128.f / Fctrs1.y) - floor(inpRmpEndPts0.y / Fctrs1.y);
            inpRmpEndPts0.y = min(inpRmpEndPts0.y, _Max);
        }
        if (inpRmpEndPts0.z <= _Min)
            inpRmpEndPts0.z = _Min;
        else
        {
            inpRmpEndPts0.z += floor(128.f / Fctrs1.z) - floor(inpRmpEndPts0.z / Fctrs1.z);
            inpRmpEndPts0.z = min(inpRmpEndPts0.z, _Max);
        }

        inpRmpEndPts0 = floor(inpRmpEndPts0 / Fctrs0) * Fctrs0;

        inpRmpEndPts1 = floor(rsltC1);
        if (inpRmpEndPts1.x <= _Min)
            inpRmpEndPts1.x = _Min;
        else
        {
            inpRmpEndPts1.x += floor(128.f / Fctrs1.x) - floor(inpRmpEndPts1.x / Fctrs1.x);
            inpRmpEndPts1.x = min(inpRmpEndPts1.x, _Max);
        }
        if (inpRmpEndPts1.y <= _Min)
            inpRmpEndPts1.y = _Min;
        else
        {
            inpRmpEndPts1.y += floor(128.f / Fctrs1.y) - floor(inpRmpEndPts1.y / Fctrs1.y);
            inpRmpEndPts1.y = min(inpRmpEndPts1.y, _Max);
        }
        if (inpRmpEndPts1.z <= _Min)
            inpRmpEndPts1.z = _Min;
        else
        {
            inpRmpEndPts1.z += floor(128.f / Fctrs1.z) - floor(inpRmpEndPts1.z / Fctrs1.z);
            inpRmpEndPts1.z = min(inpRmpEndPts1.z, _Max);
        }

        inpRmpEndPts1 = floor(inpRmpEndPts1 / Fctrs0) * Fctrs0;
    }  // MkRmpOnGrid

    CMP_EndPoints EndPoints;
    EndPoints.Color0 = inpRmpEndPts0;
    EndPoints.Color1 = inpRmpEndPts1;

    return EndPoints;
}
#endif

// Process a rgbBlock which is normalized (0.0f ... 1.0f), signed normal is not implemented
static CGU_Vec2ui CompressBlockBC1_RGBA_Internal(const CGU_Vec3f rgbBlockUVf[BLOCK_SIZE_4X4],
                                                 const CGU_FLOAT BlockA[BLOCK_SIZE_4X4],
                                                 CGU_Vec3f       channelWeights,
                                                 CGU_UINT32      dwAlphaThreshold,
                                                 CGU_UINT32      m_nRefinementSteps,
                                                 CMP_IN CGU_FLOAT fquality,
                                                 CGU_BOOL         isSRGB)
{
    CGU_Vec2ui cmpBlock = {0, 0};
    CGU_FLOAT  errLQ    = 1e6f;

    cmpBlock = CompressRGBBlock_FM(rgbBlockUVf, fquality, isSRGB, CMP_REFINOUT errLQ);

#ifndef CMP_USE_LOWQUALITY
    //------------------------------------------------------------------
    // Processing is in 0..255 range, code needs to be normized to 0..1
    //------------------------------------------------------------------
    if ((errLQ > 0.0f) && (fquality > CMP_QUALITY2))
    {
        CGU_Vec3f  rgbBlock_normal[BLOCK_SIZE_4X4];
        CGU_UINT32 nCmpIndices = 0;
        CGU_UINT32 c0, c1;
        // High Quality
        CMP_EndPoints EndPoints = {{0, 0, 0xFF}, {0, 0, 0xFF}};
        // Hold a err ref to lowest quality compression, to check if new compression is any better
        CGU_Vec2ui Q1CompData = cmpBlock;
        // High Quality
        CGU_UINT32 i;

        ALIGN_16 CGU_FLOAT Rpt[BLOCK_SIZE_4X4];
        CGU_UINT32         pcIndices = 0;

        m_nRefinementSteps = 0;

        CGU_Vec3f BlkInBGRf_UV[BLOCK_SIZE_4X4];  // Normalized Block Input (0..1) in BGR channel format
        // Default inidices & endpoints for Transparent Block
        CGU_Vec3ui nEndpoints0 = {0, 0, 0};           // Endpoints are stored BGR as x,y,z
        CGU_Vec3ui nEndpoints1 = {0xFF, 0xFF, 0xFF};  // Endpoints are stored BGR as x,y,z

        for (i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            Rpt[i] = 0.0f;
        }

        //===============================================================
        // Check if we have more then 2 colors and process Alpha block
        CGU_UINT32 dwColors = 0;
        CGU_UINT32 dwBlk[BLOCK_SIZE_4X4];
        CGU_UINT32 R, G, B, A;
        for (i = 0; i < BLOCK_SIZE_4X4; i++)
        {
            // Do any color conversion prior to processing the block
            rgbBlock_normal[i] = isSRGB ? cmp_linearToSrgb(rgbBlockUVf[i]) : rgbBlockUVf[i];

            R = (CGU_UINT32)(rgbBlock_normal[i].x * 255.0f);
            G = (CGU_UINT32)(rgbBlock_normal[i].y * 255.0f);
            B = (CGU_UINT32)(rgbBlock_normal[i].z * 255.0f);

            if (dwAlphaThreshold > 0)
                A = (CGU_UINT32)BlockA[i];
            else
                A = 255;

            // Punch Through Alpha in BC1 Codec (1 bit alpha)
            if ((dwAlphaThreshold == 0) || (A >= dwAlphaThreshold))
            {
                // copy to local RGB data and have alpha set to 0xFF
                dwBlk[dwColors++] = A << 24 | R << 16 | G << 8 | B;
            }
        }

        if (!dwColors)
        {
            // All are colors transparent
            EndPoints.Color0.x = EndPoints.Color0.y = EndPoints.Color0.z = 0.0f;
            EndPoints.Color1.x = EndPoints.Color1.y = EndPoints.Color0.z = 255.0f;
            nCmpIndices                                                  = 0xFFFFFFFF;
        }
        else
        {
            // We have colors to process
            nCmpIndices = 0;
// Punch Through Alpha Support ToDo
// CGU_BOOL bHasAlpha = (dwColors != BLOCK_SIZE_4X4);
// bHasAlpha = bHasAlpha && (dwAlphaThreshold > 0); // valid for  (dwNumChannels=4);
// if (bHasAlpha) {
//      CGU_Vec2ui  compBlock = {0xf800f800,0};
//     return compBlock;
// }

// Here we are computing an unique number of sorted colors.
// For each unique value we compute the number of it appearences.
// qsort((void *)dwBlk, (size_t)dwColors, sizeof(CGU_UINT32), QSortIntCmp);
#ifndef ASPM_GPU
            std::sort(dwBlk, dwBlk + 15);
#else
            {
                CGU_UINT32 j;
                CMP_di     what[BLOCK_SIZE_4X4];

                for (i = 0; i < dwColors; i++)
                {
                    what[i].index = i;
                    what[i].data  = dwBlk[i];
                }

                CGU_UINT32 tmp_index;
                CGU_UINT32 tmp_data;

                for (i = 1; i < dwColors; i++)
                {
                    for (j = i; j > 0; j--)
                    {
                        if (what[j - 1].data > what[j].data)
                        {
                            tmp_index         = what[j].index;
                            tmp_data          = what[j].data;
                            what[j].index     = what[j - 1].index;
                            what[j].data      = what[j - 1].data;
                            what[j - 1].index = tmp_index;
                            what[j - 1].data  = tmp_data;
                        }
                    }
                }
                for (i = 0; i < dwColors; i++)
                    dwBlk[i] = what[i].data;
            }
#endif
            CGU_UINT32 new_p;
            CGU_UINT32 dwBlkU[BLOCK_SIZE_4X4];
            CGU_UINT32 dwUniqueColors = 0;
            new_p = dwBlkU[0]   = dwBlk[0];
            Rpt[dwUniqueColors] = 1.f;
            for (i = 1; i < dwColors; i++)
            {
                if (new_p != dwBlk[i])
                {
                    dwUniqueColors++;
                    new_p = dwBlkU[dwUniqueColors] = dwBlk[i];
                    Rpt[dwUniqueColors]            = 1.f;
                }
                else
                    Rpt[dwUniqueColors] += 1.f;
            }
            dwUniqueColors++;

            // Simple case of only 2 colors to process
            // no need for futher processing as lowest quality methods work best for this case
            if (dwUniqueColors <= 2)
            {
                return Q1CompData;
            }
            else
            {
                // switch from int range back to UV floats
                for (i = 0; i < dwUniqueColors; i++)
                {
                    R                 = (dwBlkU[i] >> 16) & 0xff;
                    G                 = (dwBlkU[i] >> 8) & 0xff;
                    B                 = (dwBlkU[i] >> 0) & 0xff;
                    BlkInBGRf_UV[i].z = (CGU_FLOAT)R / 255.0f;
                    BlkInBGRf_UV[i].y = (CGU_FLOAT)G / 255.0f;
                    BlkInBGRf_UV[i].x = (CGU_FLOAT)B / 255.0f;
                }

                CGU_Vec3f channelWeightsBGR;
                channelWeightsBGR.x = channelWeights.z;
                channelWeightsBGR.y = channelWeights.y;
                channelWeightsBGR.z = channelWeights.x;

                EndPoints = CompressRGBBlock_Slow(BlkInBGRf_UV, Rpt, dwUniqueColors, channelWeightsBGR, m_nRefinementSteps);
            }
        }  // colors

        //===================================================================
        // Process Cluster INPUT is constant EndPointsf OUTPUT is pcIndices
        //===================================================================
        if (nCmpIndices == 0)
        {
            R                  = (CGU_UINT32)(EndPoints.Color0.z);
            G                  = (CGU_UINT32)(EndPoints.Color0.y);
            B                  = (CGU_UINT32)(EndPoints.Color0.x);
            CGU_INT32 cluster0 = cmp_constructColor(R, G, B);

            R                  = (CGU_UINT32)(EndPoints.Color1.z);
            G                  = (CGU_UINT32)(EndPoints.Color1.y);
            B                  = (CGU_UINT32)(EndPoints.Color1.x);
            CGU_INT32 cluster1 = cmp_constructColor(R, G, B);

            CGU_Vec3f InpRmp[NUM_ENDPOINTS];
            if ((cluster0 <= cluster1)  // valid for 4 channels
                                        // || (cluster0 > cluster1)    // valid for 3 channels
            )
            {
                // inverse endpoints
                InpRmp[0] = EndPoints.Color1;
                InpRmp[1] = EndPoints.Color0;
            }
            else
            {
                InpRmp[0] = EndPoints.Color0;
                InpRmp[1] = EndPoints.Color1;
            }

            CGU_Vec3f srcblockBGR[BLOCK_SIZE_4X4];
            CGU_FLOAT srcblockA[BLOCK_SIZE_4X4];

            // Swizzle the source RGB to BGR for processing
            for (i = 0; i < BLOCK_SIZE_4X4; i++)
            {
                srcblockBGR[i].z = rgbBlock_normal[i].x * 255.0f;
                srcblockBGR[i].y = rgbBlock_normal[i].y * 255.0f;
                srcblockBGR[i].x = rgbBlock_normal[i].z * 255.0f;
                srcblockA[i]     = 0.0f;
                if (dwAlphaThreshold > 0)
                {
                    CGU_UINT32 alpha = (CGU_UINT32)BlockA[i];
                    if (alpha >= dwAlphaThreshold)
                        srcblockA[i] = BlockA[i];
                }
            }

            // input ramp is on the coarse grid
            // make ramp endpoints the way they'll going to be decompressed
            CGU_Vec3f InpRmpL[NUM_ENDPOINTS];
            CGU_Vec3f Fctrs = {32.0F, 64.0F, 32.0F};  // 1 << RG,1 << GG,1 << BG

            {
                //   ConstantRamp = MkWkRmpPts(InpRmpL, InpRmp);
                InpRmpL[0] = InpRmp[0] + floor(InpRmp[0] / Fctrs);
                InpRmpL[0] = cmp_clampVec3f(InpRmpL[0], 0.0f, 255.0f);
                InpRmpL[1] = InpRmp[1] + floor(InpRmp[1] / Fctrs);
                InpRmpL[1] = cmp_clampVec3f(InpRmpL[1], 0.0f, 255.0f);
            }  // MkWkRmpPts

            // build ramp
            CGU_Vec3f LerpRmp[4];
            CGU_Vec3f offset = {1.0f, 1.0f, 1.0f};
            {
                //BldRmp(Rmp, InpRmpL, dwNumChannels);
                // linear interpolate end points to get the ramp
                LerpRmp[0] = InpRmpL[0];
                LerpRmp[3] = InpRmpL[1];
                LerpRmp[1] = floor((InpRmpL[0] * 2.0f + LerpRmp[3] + offset) / 3.0f);
                LerpRmp[2] = floor((InpRmpL[0] + LerpRmp[3] * 2.0f + offset) / 3.0f);
            }  // BldRmp

            //=========================================================================
            // Clusterize, Compute error and find DXTC indexes for the current cluster
            //=========================================================================
            {
                // Clusterize
                CGU_UINT32 alpha;

                // For each colour in the original block assign it
                // to the closest cluster and compute the cumulative error
                for (i = 0; i < BLOCK_SIZE_4X4; i++)
                {
                    alpha = (CGU_UINT32)srcblockA[i];
                    if ((dwAlphaThreshold > 0) && alpha == 0)
                    {                                      //*((CGU_DWORD *)&_Blk[i][AC]) == 0)
                        pcIndices |= cmp_set2Bit32(4, i);  // dwNumChannels 3 or 4 (default is 4)
                    }
                    else
                    {
                        CGU_FLOAT shortest      = 99999999999.f;
                        CGU_UINT8 shortestIndex = 0;

                        CGU_Vec3f channelWeightsBGR;
                        channelWeightsBGR.x = channelWeights.z;
                        channelWeightsBGR.y = channelWeights.y;
                        channelWeightsBGR.z = channelWeights.x;

                        for (CGU_UINT8 rampindex = 0; rampindex < 4; rampindex++)
                        {
                            // r is either 1 or 4
                            // calculate the distance for each component
                            CGU_FLOAT distance =
                                dot(((srcblockBGR[i] - LerpRmp[rampindex]) * channelWeightsBGR), ((srcblockBGR[i] - LerpRmp[rampindex]) * channelWeightsBGR));
                            if (distance < shortest)
                            {
                                shortest      = distance;
                                shortestIndex = rampindex;
                            }
                        }

                        // The total is a sum of (error += shortest)
                        // We have the index of the best cluster, so assign this in the block
                        // Reorder indices to match correct DXTC ordering
                        if (shortestIndex == 3)  // dwNumChannels - 1
                            shortestIndex = 1;
                        else if (shortestIndex)
                            shortestIndex++;
                        pcIndices |= cmp_set2Bit32(shortestIndex, i);
                    }
                }  // BLOCK_SIZE_4X4
            }      // Clusterize
        }          // Process Cluster

        //==============================================================
        // Generate Compressed Result from nEndpoints & pcIndices
        //==============================================================
        R  = (CGU_UINT32)(EndPoints.Color0.z);
        G  = (CGU_UINT32)(EndPoints.Color0.y);
        B  = (CGU_UINT32)(EndPoints.Color0.x);
        c0 = cmp_constructColor(R, G, B);

        R  = (CGU_UINT32)(EndPoints.Color1.z);
        G  = (CGU_UINT32)(EndPoints.Color1.y);
        B  = (CGU_UINT32)(EndPoints.Color1.x);
        c1 = cmp_constructColor(R, G, B);

        // Get Processed indices if not set
        if (nCmpIndices == 0)
            nCmpIndices = pcIndices;

        if (c0 <= c1)
        {
            cmpBlock.x = c1 | (c0 << 16);
        }
        else
            cmpBlock.x = c0 | (c1 << 16);

        cmpBlock.y = nCmpIndices;

        // Select best compression
        CGU_FLOAT CompErr = CMP_RGBBlockError(rgbBlockUVf, cmpBlock, isSRGB);
        if (CompErr > errLQ)
            cmpBlock = Q1CompData;
    }
#endif
    return cmpBlock;
}

//============================= Alpha: New single header interfaces: supports GPU shader interface  ==================================================

// Compress a BC1 block - Use new code in cmp_bc1.h
static CGU_Vec2ui CompressBlockBC1_UNORM(CGU_Vec3f rgbablockf[BLOCK_SIZE_4X4], CMP_IN CGU_FLOAT fquality, CGU_BOOL isSRGB)
{
    CGU_FLOAT BlockA[BLOCK_SIZE_4X4];  // Not used but required
    CGU_Vec3f channelWeights = {1.0f, 1.0f, 1.0f};

    return CompressBlockBC1_RGBA_Internal(rgbablockf,
                                          BlockA,  // ToDo support nullptr
                                          channelWeights,
                                          0,
                                          1,
                                          fquality,
                                          isSRGB);
}

// Compress a BC2 block
static CGU_Vec4ui CompressBlockBC2_UNORM(CMP_IN CGU_Vec3f BlockRGB[BLOCK_SIZE_4X4],
                                         CMP_IN CGU_FLOAT BlockA[BLOCK_SIZE_4X4],
                                         CGU_FLOAT        fquality,
                                         CGU_BOOL         isSRGB)
{
    CGU_Vec2ui compressedBlocks;
    CGU_Vec4ui compBlock;
    compressedBlocks = cmp_compressExplicitAlphaBlock(BlockA);
    compBlock.x      = compressedBlocks.x;
    compBlock.y      = compressedBlocks.y;

    CGU_Vec3f channelWeights = {1.0f, 1.0f, 1.0f};
    compressedBlocks         = CompressBlockBC1_RGBA_Internal(BlockRGB, BlockA, channelWeights, 0, 1, fquality, isSRGB);
    compBlock.z              = compressedBlocks.x;
    compBlock.w              = compressedBlocks.y;
    return compBlock;
}

// Compress a BC3 block
static CGU_Vec4ui CompressBlockBC3_UNORM(CMP_IN CGU_Vec3f BlockRGB[BLOCK_SIZE_4X4],
                                         CMP_IN CGU_FLOAT BlockA[BLOCK_SIZE_4X4],
                                         CGU_FLOAT        fquality,
                                         CGU_BOOL         isSRGB)
{
    CGU_Vec4ui compBlock;
    CGU_Vec2ui cmpBlock;

    cmpBlock    = cmp_compressAlphaBlock(BlockA, fquality, FALSE);
    compBlock.x = cmpBlock.x;
    compBlock.y = cmpBlock.y;

    CGU_Vec2ui compressedBlocks;
    compressedBlocks = CompressBlockBC1_UNORM(BlockRGB, fquality, isSRGB);
    compBlock.z      = compressedBlocks.x;
    compBlock.w      = compressedBlocks.y;
    return compBlock;
}

// Compress a BC4 block
static CGU_Vec2ui CompressBlockBC4_UNORM(CMP_IN CGU_FLOAT Block[BLOCK_SIZE_4X4], CGU_FLOAT fquality)
{
    CGU_Vec2ui cmpBlock;
    cmpBlock = cmp_compressAlphaBlock(Block, fquality, FALSE);
    return cmpBlock;
}

// Compress a BC4 block
static CGU_Vec2ui CompressBlockBC4_SNORM(CMP_IN CGU_FLOAT Block[BLOCK_SIZE_4X4], CGU_FLOAT fquality)
{
    CGU_Vec2ui cmpBlock;
    cmpBlock = cmp_compressAlphaBlock(Block, fquality, TRUE);
    return cmpBlock;
}

// Compress a BC5 block
static CGU_Vec4ui CompressBlockBC5_UNORM(CMP_IN CGU_FLOAT BlockU[BLOCK_SIZE_4X4], CMP_IN CGU_FLOAT BlockV[BLOCK_SIZE_4X4], CGU_FLOAT fquality)
{
    CGU_Vec4ui compressedBlock = {0, 0, 0, 0};
    CGU_Vec2ui cmpBlock;
    cmpBlock          = cmp_compressAlphaBlock(BlockU, fquality, FALSE);
    compressedBlock.x = cmpBlock.x;
    compressedBlock.y = cmpBlock.y;

    cmpBlock          = cmp_compressAlphaBlock(BlockV, fquality, FALSE);
    compressedBlock.z = cmpBlock.x;
    compressedBlock.w = cmpBlock.y;

    return compressedBlock;
}

// Compress a BC6 & BC7 UNORM block ToDo

#endif
