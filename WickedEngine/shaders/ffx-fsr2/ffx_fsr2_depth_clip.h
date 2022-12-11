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

#ifndef FFX_FSR2_DEPTH_CLIP_H
#define FFX_FSR2_DEPTH_CLIP_H

FFX_STATIC const FfxFloat32 DepthClipBaseScale = 4.0f;

FfxFloat32 ComputeSampleDepthClip(FfxInt32x2 iPxSamplePos, FfxFloat32 fPreviousDepth, FfxFloat32 fPreviousDepthBilinearWeight, FfxFloat32 fCurrentDepthViewSpace)
{
    FfxFloat32 fPrevNearestDepthViewSpace = abs(ConvertFromDeviceDepthToViewSpace(fPreviousDepth));

    // Depth separation logic ref: See "Minimum Triangle Separation for Correct Z-Buffer Occlusion"
    // Intention: worst case of formula in Figure4 combined with Ksep factor in Section 4
    // TODO: check intention and improve, some banding visible
    const FfxFloat32 fHalfViewportWidth = RenderSize().x * 0.5f;
    FfxFloat32 fDepthThreshold = ffxMin(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace);

    // WARNING: Ksep only works with reversed-z with infinite projection.
    const FfxFloat32 Ksep = 1.37e-05f;
    FfxFloat32 fRequiredDepthSeparation = Ksep * fDepthThreshold * TanHalfFoV() * fHalfViewportWidth;
    FfxFloat32 fDepthDiff = fCurrentDepthViewSpace - fPrevNearestDepthViewSpace;

    FfxFloat32 fDepthClipFactor = (fDepthDiff > 0) ? ffxSaturate(fRequiredDepthSeparation / fDepthDiff) : 1.0f;

#ifdef _DEBUG
    rw_debug_out[iPxSamplePos] = FfxFloat32x4(fCurrentDepthViewSpace, fPrevNearestDepthViewSpace, fDepthDiff, fDepthClipFactor);
#endif

    return fPreviousDepthBilinearWeight * fDepthClipFactor * ffxLerp(1.0f, DepthClipBaseScale, ffxSaturate(fDepthDiff * fDepthDiff));
}

FfxFloat32 ComputeDepthClip(FfxFloat32x2 fUvSample, FfxFloat32 fCurrentDepthViewSpace)
{
    FfxFloat32x2 fPxSample = fUvSample * RenderSize() - 0.5f;
    FfxInt32x2 iPxSample = FfxInt32x2(floor(fPxSample));
    FfxFloat32x2 fPxFrac = ffxFract(fPxSample);

    const FfxFloat32 fBilinearWeights[2][2] = {
        {
            (1 - fPxFrac.x) * (1 - fPxFrac.y),
            (fPxFrac.x) * (1 - fPxFrac.y)
        },
        {
            (1 - fPxFrac.x) * (fPxFrac.y),
            (fPxFrac.x) * (fPxFrac.y)
        }
    };

    FfxFloat32 fDepth = 0.0f;
    FfxFloat32 fWeightSum = 0.0f;
    for (FfxInt32 y = 0; y <= 1; ++y) {
        for (FfxInt32 x = 0; x <= 1; ++x) {
            FfxInt32x2 iSamplePos = iPxSample + FfxInt32x2(x, y);
            if (IsOnScreen(iSamplePos, RenderSize())) {
                FfxFloat32 fBilinearWeight = fBilinearWeights[y][x];
                if (fBilinearWeight > reconstructedDepthBilinearWeightThreshold) {
                    fDepth += ComputeSampleDepthClip(iSamplePos, LoadReconstructedPrevDepth(iSamplePos), fBilinearWeight, fCurrentDepthViewSpace);
                    fWeightSum += fBilinearWeight;
                }
            }
        }
    }

    return (fWeightSum > 0) ? fDepth / fWeightSum : DepthClipBaseScale;
}

void DepthClip(FfxInt32x2 iPxPos)
{
    FfxFloat32x2 fDepthUv = (iPxPos + 0.5f) / RenderSize();
    FfxFloat32x2 fMotionVector = LoadDilatedMotionVector(iPxPos);
    FfxFloat32x2 fDilatedUv = fDepthUv + fMotionVector;
    FfxFloat32 fCurrentDepthViewSpace = abs(ConvertFromDeviceDepthToViewSpace(LoadDilatedDepth(iPxPos)));

    FfxFloat32 fDepthClip = ComputeDepthClip(fDilatedUv, fCurrentDepthViewSpace);

    StoreDepthClip(iPxPos, fDepthClip);
}

#endif //!defined( FFX_FSR2_DEPTH_CLIPH )