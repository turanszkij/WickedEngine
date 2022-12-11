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

#ifndef FFX_FSR2_RECONSTRUCT_DILATED_VELOCITY_AND_PREVIOUS_DEPTH_H
#define FFX_FSR2_RECONSTRUCT_DILATED_VELOCITY_AND_PREVIOUS_DEPTH_H

void ReconstructPrevDepth(FfxInt32x2 iPxPos, FfxFloat32 fDepth, FfxFloat32x2 fMotionVector, FfxInt32x2 iPxDepthSize)
{
    FfxFloat32x2 fDepthUv = (iPxPos + FfxFloat32(0.5)) / iPxDepthSize;
    FfxFloat32x2 fPxPrevPos = (fDepthUv + fMotionVector) * iPxDepthSize - FfxFloat32x2(0.5, 0.5);
    FfxInt32x2 iPxPrevPos = FfxInt32x2(floor(fPxPrevPos));
    FfxFloat32x2 fPxFrac = ffxFract(fPxPrevPos);

    const FfxFloat32 bilinearWeights[2][2] = {
        {
            (1 - fPxFrac.x) * (1 - fPxFrac.y),
            (fPxFrac.x) * (1 - fPxFrac.y)
        },
        {
            (1 - fPxFrac.x) * (fPxFrac.y),
            (fPxFrac.x) * (fPxFrac.y)
        }
    };

    // Project current depth into previous frame locations.
    // Push to all pixels having some contribution if reprojection is using bilinear logic.
    for (FfxInt32 y = 0; y <= 1; ++y) {
        for (FfxInt32 x = 0; x <= 1; ++x) {
            
            FfxInt32x2 offset = FfxInt32x2(x, y);
            FfxFloat32 w = bilinearWeights[y][x];

            if (w > reconstructedDepthBilinearWeightThreshold) {

                FfxInt32x2 storePos = iPxPrevPos + offset;
                if (IsOnScreen(storePos, iPxDepthSize)) {
                    StoreReconstructedDepth(storePos, fDepth);
                }
            }
        }
    }
}

void FindNearestDepth(FFX_PARAMETER_IN FfxInt32x2 iPxPos, FFX_PARAMETER_IN FfxInt32x2 iPxSize, FFX_PARAMETER_OUT FfxFloat32 fNearestDepth, FFX_PARAMETER_OUT FfxInt32x2 fNearestDepthCoord)
{
    const FfxInt32 iSampleCount = 9;
    const FfxInt32x2 iSampleOffsets[iSampleCount] = {
        FfxInt32x2(+0, +0),
        FfxInt32x2(+1, +0),
        FfxInt32x2(+0, +1),
        FfxInt32x2(+0, -1),
        FfxInt32x2(-1, +0),
        FfxInt32x2(-1, +1),
        FfxInt32x2(+1, +1),
        FfxInt32x2(-1, -1),
        FfxInt32x2(+1, -1),
    };

    // pull out the depth loads to allow SC to batch them
    FfxFloat32 depth[9];
    FfxInt32 iSampleIndex = 0;
    FFX_UNROLL
    for (iSampleIndex = 0; iSampleIndex < iSampleCount; ++iSampleIndex) {

        FfxInt32x2 iPos = iPxPos + iSampleOffsets[iSampleIndex];
        depth[iSampleIndex] = LoadInputDepth(iPos);
    }

    // find closest depth
    fNearestDepthCoord = iPxPos;
    fNearestDepth = depth[0];
    FFX_UNROLL
    for (iSampleIndex = 1; iSampleIndex < iSampleCount; ++iSampleIndex) {

        FfxInt32x2 iPos = iPxPos + iSampleOffsets[iSampleIndex];
        if (IsOnScreen(iPos, iPxSize)) {

            FfxFloat32 fNdDepth = depth[iSampleIndex];
#if FFX_FSR2_OPTION_INVERTED_DEPTH
            if (fNdDepth > fNearestDepth) {
#else
            if (fNdDepth < fNearestDepth) {
#endif
                fNearestDepthCoord = iPos;
                fNearestDepth = fNdDepth;
            }
        }
    }
}

FfxFloat32 ComputeMotionDivergence(FfxInt32x2 iPxPos, FfxInt32x2 iPxInputMotionVectorSize)
{
    FfxFloat32 minconvergence = 1.0f;

    FfxFloat32x2 fMotionVectorNucleus = LoadInputMotionVector(iPxPos) * RenderSize();
    FfxFloat32 fNucleusVelocity = length(fMotionVectorNucleus);

    const FfxFloat32 MotionVectorVelocityEpsilon = 1e-02f;

    if (fNucleusVelocity > MotionVectorVelocityEpsilon) {
        for (FfxInt32 y = -1; y <= 1; ++y) {
            for (FfxInt32 x = -1; x <= 1; ++x) {

                FfxInt32x2 sp = ClampLoad(iPxPos, FfxInt32x2(x, y), iPxInputMotionVectorSize);

                FfxFloat32x2 fMotionVector = LoadInputMotionVector(sp) * RenderSize();
                FfxFloat32 fVelocity = length(fMotionVector);

                fVelocity = ffxMax(fVelocity, fNucleusVelocity);
                minconvergence = ffxMin(minconvergence, dot(fMotionVector / fVelocity, fMotionVectorNucleus / fVelocity));
            }
        }
    }

    return ffxSaturate(1.0f - minconvergence);
}


void PreProcessReactiveMasks(FfxInt32x2 iPxLrPos, FfxFloat32 fMotionDivergence)
{
    // Compensate for bilinear sampling in accumulation pass

    FfxFloat32x3 fReferenceColor = LoadPreparedInputColor(iPxLrPos);
    FfxFloat32x2 fReactiveFactor = FfxFloat32x2(0.0f, fMotionDivergence);

    for (int y = -1; y < 2; y++) {
        for (int x = -1; x < 2; x++) {

            const FfxInt32x2 sampleCoord = ClampLoad(iPxLrPos, FfxInt32x2(x, y), FfxInt32x2(RenderSize()));

            FfxFloat32x3 fColorSample = LoadPreparedInputColor(sampleCoord);
            FfxFloat32 fReactiveSample = LoadReactiveMask(sampleCoord);
            FfxFloat32 fTransparencyAndCompositionSample = LoadTransparencyAndCompositionMask(sampleCoord);

            const FfxFloat32 fColorSimilarity = dot(normalize(fReferenceColor), normalize(fColorSample));
            const FfxFloat32 fVelocitySimilarity = 1.0f - abs(length(fReferenceColor) - length(fColorSample));
            const FfxFloat32 fSimilarity = fColorSimilarity * fVelocitySimilarity;

            // Increase power for non-similar samples
            const FfxFloat32 fPowerBiasMax = 6.0f;
            const FfxFloat32 fSimilarityPower = 1.0f + (fPowerBiasMax - fSimilarity * fPowerBiasMax);
            const FfxFloat32 fWeightedReactiveSample = ffxPow(fReactiveSample, fSimilarityPower);
            const FfxFloat32 fWeightedTransparencyAndCompositionSample = ffxPow(fTransparencyAndCompositionSample, fSimilarityPower);

            fReactiveFactor = ffxMax(fReactiveFactor, FfxFloat32x2(fWeightedReactiveSample, fWeightedTransparencyAndCompositionSample));
        }
    }

    StoreDilatedReactiveMasks(iPxLrPos, fReactiveFactor);
}

void ReconstructAndDilate(FfxInt32x2 iPxLrPos)
{
    FfxFloat32 fDilatedDepth;
    FfxInt32x2 iNearestDepthCoord;

    FindNearestDepth(iPxLrPos, RenderSize(), fDilatedDepth, iNearestDepthCoord);

#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
    FfxInt32x2 iSamplePos = iPxLrPos;
    FfxInt32x2 iMotionVectorPos = iNearestDepthCoord;
#else
    FfxInt32x2 iSamplePos = ComputeHrPosFromLrPos(iPxLrPos);
    FfxInt32x2 iMotionVectorPos = ComputeHrPosFromLrPos(iNearestDepthCoord);
#endif

    FfxFloat32x2 fDilatedMotionVector = LoadInputMotionVector(iMotionVectorPos);

    StoreDilatedDepth(iPxLrPos, fDilatedDepth);
    StoreDilatedMotionVector(iPxLrPos, fDilatedMotionVector);

    ReconstructPrevDepth(iPxLrPos, fDilatedDepth, fDilatedMotionVector, RenderSize());

#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
    FfxFloat32 fMotionDivergence = ComputeMotionDivergence(iSamplePos, RenderSize());
#else
    FfxFloat32 fMotionDivergence = ComputeMotionDivergence(iSamplePos, DisplaySize());
#endif

    PreProcessReactiveMasks(iPxLrPos, fMotionDivergence);
}


#endif //!defined( FFX_FSR2_RECONSTRUCT_DILATED_VELOCITY_AND_PREVIOUS_DEPTH_H )
