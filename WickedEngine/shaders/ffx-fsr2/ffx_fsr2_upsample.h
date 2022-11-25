// This file is part of the FidelityFX SDK.
//
// Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef FFX_FSR2_UPSAMPLE_H
#define FFX_FSR2_UPSAMPLE_H

#define FFX_FSR2_OPTION_GUARANTEE_POSITIVE_UPSAMPLE_WEIGHT 0

FFX_STATIC const FfxUInt32 iLanczos2SampleCount = 16;

void Deringing(RectificationBoxData clippingBox, FFX_PARAMETER_INOUT FfxFloat32x3 fColor)
{
    fColor = clamp(fColor, clippingBox.aabbMin, clippingBox.aabbMax);
}
#if FFX_HALF
void Deringing(RectificationBoxDataMin16 clippingBox, FFX_PARAMETER_INOUT FFX_MIN16_F3 fColor)
{
    fColor = clamp(fColor, clippingBox.aabbMin, clippingBox.aabbMax);
}
#endif

#ifndef FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE
#define FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE 1 // Approximate
#endif

FfxFloat32 GetUpsampleLanczosWeight(FfxFloat32x2 fSrcSampleOffset, FfxFloat32x2 fKernelWeight)
{
    FfxFloat32x2 fSrcSampleOffsetBiased = fSrcSampleOffset * fKernelWeight;
#if FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 0 // LANCZOS_TYPE_REFERENCE
    FfxFloat32 fSampleWeight = Lanczos2(length(fSrcSampleOffsetBiased));
#elif FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 1 // LANCZOS_TYPE_LUT
    FfxFloat32 fSampleWeight = Lanczos2_UseLUT(length(fSrcSampleOffsetBiased));
#elif FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 2 // LANCZOS_TYPE_APPROXIMATE
    FfxFloat32 fSampleWeight = Lanczos2ApproxSq(dot(fSrcSampleOffsetBiased, fSrcSampleOffsetBiased));
#else
#error "Invalid Lanczos type"
#endif
    return fSampleWeight;
}

#if FFX_HALF
FFX_MIN16_F GetUpsampleLanczosWeight(FFX_MIN16_F2 fSrcSampleOffset, FFX_MIN16_F2 fKernelWeight)
{
    FFX_MIN16_F2 fSrcSampleOffsetBiased = fSrcSampleOffset * fKernelWeight;
#if FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 0 // LANCZOS_TYPE_REFERENCE
    FFX_MIN16_F fSampleWeight = Lanczos2(length(fSrcSampleOffsetBiased));
#elif FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 1 // LANCZOS_TYPE_APPROXIMATE
    FFX_MIN16_F fSampleWeight = Lanczos2ApproxSq(dot(fSrcSampleOffsetBiased, fSrcSampleOffsetBiased));
#elif FFX_FSR2_OPTION_UPSAMPLE_USE_LANCZOS_TYPE == 2 // LANCZOS_TYPE_LUT
    FFX_MIN16_F fSampleWeight = Lanczos2_UseLUT(length(fSrcSampleOffsetBiased));
    // To Test: Save reciproqual sqrt compute
    // FfxFloat32 fSampleWeight = Lanczos2Sq_UseLUT(dot(fSrcSampleOffsetBiased, fSrcSampleOffsetBiased));
#else
#error "Invalid Lanczos type"
#endif
    return fSampleWeight;
}
#endif

FfxFloat32 Pow3(FfxFloat32 x)
{
    return x * x * x;
}

#if FX_HALF
FFX_MIN16_F Pow3(FFX_MIN16_F x)
{
    return x * x * x;
}
#endif

FfxFloat32x4 ComputeUpsampledColorAndWeight(FfxInt32x2 iPxHrPos, FfxFloat32x2 fKernelWeight, FFX_PARAMETER_INOUT RectificationBoxData clippingBox)
{
#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
#include "ffx_fsr2_force16_begin.h"
#endif
    // We compute a sliced lanczos filter with 2 lobes (other slices are accumulated temporaly)
    FfxFloat32x2 fDstOutputPos = FfxFloat32x2(iPxHrPos) + FFX_BROADCAST_FLOAT32X2(0.5f);      // Destination resolution output pixel center position
    FfxFloat32x2 fSrcOutputPos = fDstOutputPos * DownscaleFactor();                   // Source resolution output pixel center position
    FfxInt32x2 iSrcInputPos = FfxInt32x2(floor(fSrcOutputPos));                     // TODO: what about weird upscale factors...

#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
#include "ffx_fsr2_force16_end.h"
#endif

#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
#include "ffx_fsr2_force16_begin.h"
    RectificationBoxMin16 fRectificationBox;
#else
    RectificationBox fRectificationBox;
#endif

    FfxFloat32x3 fSamples[iLanczos2SampleCount];


    FfxFloat32x2 fSrcUnjitteredPos = (FfxFloat32x2(iSrcInputPos) + FfxFloat32x2(0.5f, 0.5f)) - Jitter(); // This is the un-jittered position of the sample at offset 0,0
    
    FfxInt32x2 offsetTL;
    offsetTL.x = (fSrcUnjitteredPos.x > fSrcOutputPos.x) ? FfxInt32(-2) : FfxInt32(-1);
    offsetTL.y = (fSrcUnjitteredPos.y > fSrcOutputPos.y) ? FfxInt32(-2) : FfxInt32(-1);

    //Load samples
    // If fSrcUnjitteredPos.y > fSrcOutputPos.y, indicates offsetTL.y = -2, sample offset Y will be [-2, 1], clipbox will be rows [1, 3].
    // Flip row# for sampling offset in this case, so first 0~2 rows in the sampled array can always be used for computing the clipbox.
    // This reduces branch or cmove on sampled colors, but moving this overhead to sample position / weight calculation time which apply to less values.
    const FfxBoolean bFlipRow = fSrcUnjitteredPos.y > fSrcOutputPos.y;
    const FfxBoolean bFlipCol = fSrcUnjitteredPos.x > fSrcOutputPos.x;

    FfxFloat32x2 fOffsetTL = FfxFloat32x2(offsetTL);

    FFX_UNROLL
    for (FfxInt32 row = 0; row < 4; row++) {

        FFX_UNROLL
        for (FfxInt32 col = 0; col < 4; col++) {
            FfxInt32 iSampleIndex = col + (row << 2);

            FfxInt32x2 sampleColRow = FfxInt32x2(bFlipCol ? (3 - col) : col, bFlipRow ? (3 - row) : row);
            FfxInt32x2 iSrcSamplePos = FfxInt32x2(iSrcInputPos) + offsetTL + sampleColRow;

            const FfxInt32x2 sampleCoord = ClampLoad(iSrcSamplePos, FfxInt32x2(0, 0), FfxInt32x2(RenderSize()));

            fSamples[iSampleIndex] = LoadPreparedInputColor(FfxInt32x2(sampleCoord));
        }
    }

    RectificationBoxReset(fRectificationBox, fSamples[0]);

    FfxFloat32x3 fColor = FfxFloat32x3(0.f, 0.f, 0.f);
    FfxFloat32 fWeight = FfxFloat32(0.f);
    FfxFloat32x2 fBaseSampleOffset = FfxFloat32x2(fSrcUnjitteredPos - fSrcOutputPos);

    FFX_UNROLL
    for (FfxInt32 row = 0; row < 3; row++) {

        FFX_UNROLL
        for (FfxInt32 col = 0; col < 3; col++) {
            FfxInt32 iSampleIndex = col + (row << 2);

            const FfxInt32x2 sampleColRow = FfxInt32x2(bFlipCol ? (3 - col) : col, bFlipRow ? (3 - row) : row);
            const FfxFloat32x2 fOffset = fOffsetTL + FfxFloat32x2(sampleColRow);
            FfxFloat32x2 fSrcSampleOffset = fBaseSampleOffset + fOffset;

            FfxInt32x2 iSrcSamplePos = FfxInt32x2(iSrcInputPos) + FfxInt32x2(offsetTL) + sampleColRow;

            FfxFloat32 fSampleWeight = FfxFloat32(IsOnScreen(FfxInt32x2(iSrcSamplePos), FfxInt32x2(RenderSize()))) * GetUpsampleLanczosWeight(fSrcSampleOffset, fKernelWeight);

            // Update rectification box
            const FfxFloat32 fSrcSampleOffsetSq = dot(fSrcSampleOffset, fSrcSampleOffset);
            FfxFloat32 fBoxSampleWeight = FfxFloat32(1) - ffxSaturate(fSrcSampleOffsetSq / FfxFloat32(3));
            fBoxSampleWeight *= fBoxSampleWeight;
            RectificationBoxAddSample(fRectificationBox, fSamples[iSampleIndex], fBoxSampleWeight);

            fWeight += fSampleWeight;
            fColor += fSampleWeight * fSamples[iSampleIndex];
        }
    }
    // Normalize for deringing (we need to compare colors)
    fColor = fColor / (abs(fWeight) > FSR2_EPSILON ? fWeight : FfxFloat32(1.f));

    RectificationBoxComputeVarianceBoxData(fRectificationBox);
#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
    RectificationBoxDataMin16 rectificationData = RectificationBoxGetData(fRectificationBox);
    clippingBox.aabbMax = rectificationData.aabbMax;
    clippingBox.aabbMin = rectificationData.aabbMin;
    clippingBox.boxCenter = rectificationData.boxCenter;
    clippingBox.boxVec = rectificationData.boxVec;
#else
    RectificationBoxData rectificationData = RectificationBoxGetData(fRectificationBox);
    clippingBox = rectificationData;
#endif

    Deringing(rectificationData, fColor);

#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
    clippingBox.aabbMax = rectificationData.aabbMax;
    clippingBox.aabbMin = rectificationData.aabbMin;
    clippingBox.boxCenter = rectificationData.boxCenter;
    clippingBox.boxVec = rectificationData.boxVec;
#endif

    if (any(FFX_LESS_THAN(fKernelWeight, FfxFloat32x2(1, 1)))) {
        fWeight = FfxFloat32(averageLanczosWeightPerFrame);
    }
#if FFX_FSR2_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF && FFX_HALF
#include "ffx_fsr2_force16_end.h"
#endif

#if FFX_FSR2_OPTION_GUARANTEE_POSITIVE_UPSAMPLE_WEIGHT
    return FfxFloat32x4(fColor, ffxMax(FfxFloat32(FSR2_EPSILON), fWeight));
#else
    return FfxFloat32x4(fColor, ffxMax(FfxFloat32(0), fWeight));
#endif
}

#endif //!defined( FFX_FSR2_UPSAMPLE_H )
