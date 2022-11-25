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

#ifndef FFX_FSR2_LOCK_H
#define FFX_FSR2_LOCK_H

FfxFloat32 GetLuma(FfxInt32x2 pos)
{
    //add some bias to avoid locking dark areas
    return FfxFloat32(LoadPreparedInputColorLuma(pos));
}

FfxFloat32 ComputeThinFeatureConfidence(FfxInt32x2 pos)
{
    const FfxInt32 RADIUS = 1;

    FfxFloat32 fNucleus = GetLuma(pos);

    FfxFloat32 similar_threshold = 1.05f;
    FfxFloat32 dissimilarLumaMin = FSR2_FLT_MAX;
    FfxFloat32 dissimilarLumaMax = 0;

    /*
     0 1 2
     3 4 5
     6 7 8
    */

    #define SETBIT(x) (1U << x)

    FfxUInt32 mask = SETBIT(4); //flag fNucleus as similar

    const FfxUInt32 rejectionMasks[4] = {
        SETBIT(0) | SETBIT(1) | SETBIT(3) | SETBIT(4), //Upper left
        SETBIT(1) | SETBIT(2) | SETBIT(4) | SETBIT(5), //Upper right
        SETBIT(3) | SETBIT(4) | SETBIT(6) | SETBIT(7), //Lower left
        SETBIT(4) | SETBIT(5) | SETBIT(7) | SETBIT(8), //Lower right
    };

    FfxInt32 idx = 0;
    FFX_UNROLL
    for (FfxInt32 y = -RADIUS; y <= RADIUS; y++) {
        FFX_UNROLL
        for (FfxInt32 x = -RADIUS; x <= RADIUS; x++, idx++) {
            if (x == 0 && y == 0) continue;

            FfxInt32x2 samplePos = ClampLoad(pos, FfxInt32x2(x, y), FfxInt32x2(RenderSize()));

            FfxFloat32 sampleLuma = GetLuma(samplePos);
            FfxFloat32 difference = ffxMax(sampleLuma, fNucleus) / ffxMin(sampleLuma, fNucleus);

            if (difference > 0 && (difference < similar_threshold)) {
                mask |= SETBIT(idx);
            } else {
                dissimilarLumaMin = ffxMin(dissimilarLumaMin, sampleLuma);
                dissimilarLumaMax = ffxMax(dissimilarLumaMax, sampleLuma);
            }
        }
    }

    FfxBoolean isRidge = fNucleus > dissimilarLumaMax || fNucleus < dissimilarLumaMin;

    if (FFX_FALSE == isRidge) {

        return 0;
    }

    FFX_UNROLL
    for (FfxInt32 i = 0; i < 4; i++) {

        if ((mask & rejectionMasks[i]) == rejectionMasks[i]) {
            return 0;
        }
    }
    
    return 1;
}

FFX_STATIC FfxBoolean s_bLockUpdated = FFX_FALSE;

FfxFloat32x3 ComputeLockStatus(FfxInt32x2 iPxLrPos, FfxFloat32x3 fLockStatus)
{
    FfxFloat32 fConfidenceOfThinFeature = ComputeThinFeatureConfidence(iPxLrPos);

    s_bLockUpdated = FFX_FALSE;
    if (fConfidenceOfThinFeature > 0.0f)
    {
        //put to negative on new lock
        fLockStatus[LOCK_LIFETIME_REMAINING] = (fLockStatus[LOCK_LIFETIME_REMAINING] == FfxFloat32(0.0f)) ? FfxFloat32(-LockInitialLifetime()) : FfxFloat32(-(LockInitialLifetime() * 2));

        s_bLockUpdated = FFX_TRUE;
    }

    return fLockStatus;
}

void ComputeLock(FfxInt32x2 iPxLrPos)
{
    FfxInt32x2 iPxHrPos = ComputeHrPosFromLrPos(iPxLrPos);

    FfxFloat32x3 fLockStatus = ComputeLockStatus(iPxLrPos, LoadLockStatus(iPxHrPos));

    if ((s_bLockUpdated)) {
        StoreLockStatus(iPxHrPos, fLockStatus);
    }
}

#endif // FFX_FSR2_LOCK_H
