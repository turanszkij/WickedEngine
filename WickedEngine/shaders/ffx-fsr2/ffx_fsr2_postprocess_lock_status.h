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

#ifndef FFX_FSR2_POSTPROCESS_LOCK_STATUS_H
#define FFX_FSR2_POSTPROCESS_LOCK_STATUS_H

FfxFloat32x4 WrapShadingChangeLuma(FfxInt32x2 iPxSample)
{
    return FfxFloat32x4(LoadMipLuma(iPxSample, LumaMipLevelToUse()), 0, 0, 0);
}

#if FFX_HALF
FFX_MIN16_F4 WrapShadingChangeLuma(FFX_MIN16_I2 iPxSample)
{
    return FFX_MIN16_F4(LoadMipLuma(iPxSample, LumaMipLevelToUse()), 0, 0, 0);
}
#endif

#if FFX_FSR2_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF && FFX_HALF
DeclareCustomFetchBilinearSamplesMin16(FetchShadingChangeLumaSamples, WrapShadingChangeLuma)
#else
DeclareCustomFetchBilinearSamples(FetchShadingChangeLumaSamples, WrapShadingChangeLuma)
#endif
DeclareCustomTextureSample(ShadingChangeLumaSample, Bilinear, FetchShadingChangeLumaSamples)

FfxFloat32 GetShadingChangeLuma(FfxFloat32x2 fUvCoord)
{
    // const FfxFloat32 fShadingChangeLuma = exp(ShadingChangeLumaSample(fUvCoord, LumaMipDimensions()) * LumaMipRcp());
    const FfxFloat32 fShadingChangeLuma = FfxFloat32(exp(SampleMipLuma(fUvCoord, LumaMipLevelToUse()) * FfxFloat32(LumaMipRcp())));
    return fShadingChangeLuma;
}

LockState GetLockState(FfxFloat32x3 fLockStatus)
{
    LockState state = { FFX_FALSE, FFX_FALSE };

    //Check if this is a new or refreshed lock
    state.NewLock = fLockStatus[LOCK_LIFETIME_REMAINING] < FfxFloat32(0.0f);

    //For a non-refreshed lock, the lifetime is set to LockInitialLifetime()
    state.WasLockedPrevFrame = fLockStatus[LOCK_TRUST] != FfxFloat32(0.0f);

    return state;
}

LockState PostProcessLockStatus(FfxInt32x2 iPxHrPos, FFX_PARAMETER_IN FfxFloat32x2 fLrUvJittered, FFX_PARAMETER_IN FfxFloat32 fDepthClipFactor, const FfxFloat32 fAccumulationMask, FFX_PARAMETER_IN FfxFloat32 fHrVelocity,
    FFX_PARAMETER_INOUT FfxFloat32 fAccumulationTotalWeight, FFX_PARAMETER_INOUT FfxFloat32x3 fLockStatus, FFX_PARAMETER_OUT FfxFloat32 fLuminanceDiff) {

    const LockState state = GetLockState(fLockStatus);

    fLockStatus[LOCK_LIFETIME_REMAINING] = abs(fLockStatus[LOCK_LIFETIME_REMAINING]);

    FfxFloat32 fShadingChangeLuma = GetShadingChangeLuma(fLrUvJittered);

    //init temporal shading change factor, init to -1 or so in reproject to know if "true new"?
    fLockStatus[LOCK_TEMPORAL_LUMA] = (fLockStatus[LOCK_TEMPORAL_LUMA] == FfxFloat32(0.0f)) ? fShadingChangeLuma : fLockStatus[LOCK_TEMPORAL_LUMA];

    FfxFloat32 fPreviousShadingChangeLuma = fLockStatus[LOCK_TEMPORAL_LUMA];
    fLockStatus[LOCK_TEMPORAL_LUMA] = ffxLerp(fLockStatus[LOCK_TEMPORAL_LUMA], FfxFloat32(fShadingChangeLuma), FfxFloat32(0.5f));
    fLuminanceDiff = FfxFloat32(1) - MinDividedByMax(fPreviousShadingChangeLuma, fShadingChangeLuma);

    if (fLuminanceDiff > FfxFloat32(0.2f)) {
        KillLock(fLockStatus);
    }

    if (!state.NewLock && fLockStatus[LOCK_LIFETIME_REMAINING] >= FfxFloat32(0))
    {
        fLockStatus[LOCK_LIFETIME_REMAINING] *= (1.0f - fAccumulationMask);

        const FfxFloat32 depthClipThreshold = FfxFloat32(0.99f);
        if (fDepthClipFactor < depthClipThreshold)
        {
            KillLock(fLockStatus);
        }
    }

    return state;
}

#endif //!defined( FFX_FSR2_POSTPROCESS_LOCK_STATUS_H )
