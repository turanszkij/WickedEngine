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

#ifndef FFX_FSR2_REPROJECT_H
#define FFX_FSR2_REPROJECT_H

#ifndef FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE
#define FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE 1 // Approximate
#endif

FfxFloat32x4 WrapHistory(FfxInt32x2 iPxSample)
{
    return LoadHistory(iPxSample);
}

#if FFX_HALF
FFX_MIN16_F4 WrapHistory(FFX_MIN16_I2 iPxSample)
{
    return FFX_MIN16_F4(LoadHistory(iPxSample));
}
#endif


#if FFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF && FFX_HALF
DeclareCustomFetchBicubicSamplesMin16(FetchHistorySamples, WrapHistory)
DeclareCustomTextureSampleMin16(HistorySample, FFX_FSR2_GET_LANCZOS_SAMPLER1D(FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE), FetchHistorySamples)
#else
DeclareCustomFetchBicubicSamples(FetchHistorySamples, WrapHistory)
DeclareCustomTextureSample(HistorySample, FFX_FSR2_GET_LANCZOS_SAMPLER1D(FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE), FetchHistorySamples)
#endif

FfxFloat32x4 WrapLockStatus(FfxInt32x2 iPxSample)
{
    return FfxFloat32x4(LoadLockStatus(iPxSample), 0.0f);
}

#if FFX_HALF
FFX_MIN16_F4 WrapLockStatus(FFX_MIN16_I2 iPxSample)
{
    return FFX_MIN16_F4(LoadLockStatus(iPxSample), 0.0f);
}
#endif

#if 1
#if FFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF && FFX_HALF
DeclareCustomFetchBilinearSamplesMin16(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSampleMin16(LockStatusSample, Bilinear, FetchLockStatusSamples)
#else
DeclareCustomFetchBilinearSamples(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSample(LockStatusSample, Bilinear, FetchLockStatusSamples)
#endif
#else
#if FFX_FSR2_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF && FFX_HALF
DeclareCustomFetchBicubicSamplesMin16(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSampleMin16(LockStatusSample, FFX_FSR2_GET_LANCZOS_SAMPLER1D(FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE), FetchLockStatusSamples)
#else
DeclareCustomFetchBicubicSamples(FetchLockStatusSamples, WrapLockStatus)
DeclareCustomTextureSample(LockStatusSample, FFX_FSR2_GET_LANCZOS_SAMPLER1D(FFX_FSR2_OPTION_REPROJECT_USE_LANCZOS_TYPE), FetchLockStatusSamples)
#endif
#endif

FfxFloat32x2 GetMotionVector(FfxInt32x2 iPxHrPos, FfxFloat32x2 fHrUv)
{
#if FFX_FSR2_OPTION_LOW_RESOLUTION_MOTION_VECTORS
    FfxFloat32x2 fDilatedMotionVector = LoadDilatedMotionVector(FFX_MIN16_I2(fHrUv * RenderSize()));
#else
    FfxFloat32x2 fDilatedMotionVector = LoadInputMotionVector(iPxHrPos);
#endif

    return fDilatedMotionVector;
}

void ComputeReprojectedUVs(FfxInt32x2 iPxHrPos, FfxFloat32x2 fMotionVector, FFX_PARAMETER_OUT FfxFloat32x2 fReprojectedHrUv, FFX_PARAMETER_OUT FfxBoolean bIsExistingSample)
{
    FfxFloat32x2 fHrUv = (iPxHrPos + 0.5f) / DisplaySize();
    fReprojectedHrUv = fHrUv + fMotionVector;

    bIsExistingSample = (fReprojectedHrUv.x >= 0.0f && fReprojectedHrUv.x <= 1.0f) &&
        (fReprojectedHrUv.y >= 0.0f && fReprojectedHrUv.y <= 1.0f);
}

void ReprojectHistoryColor(FfxInt32x2 iPxHrPos, FfxFloat32x2 fReprojectedHrUv, FFX_PARAMETER_OUT FfxFloat32x4 fHistoryColorAndWeight)
{
    fHistoryColorAndWeight = HistorySample(fReprojectedHrUv, DisplaySize());
    fHistoryColorAndWeight.rgb *= Exposure();

#if FFX_FSR2_OPTION_HDR_COLOR_INPUT
        fHistoryColorAndWeight.rgb = Tonemap(fHistoryColorAndWeight.rgb);
#endif

    fHistoryColorAndWeight.rgb = RGBToYCoCg(fHistoryColorAndWeight.rgb);
}

void ReprojectHistoryLockStatus(FfxInt32x2 iPxHrPos, FfxFloat32x2 fReprojectedHrUv, FFX_PARAMETER_OUT FfxFloat32x3 fReprojectedLockStatus)
{
    // If function is called from Accumulate pass, we need to treat locks differently
    FfxFloat32 fInPlaceLockLifetime = LoadRwLockStatus(iPxHrPos)[LOCK_LIFETIME_REMAINING];

    fReprojectedLockStatus = SampleLockStatus(fReprojectedHrUv);

    // Keep lifetime if new lock
    if (fInPlaceLockLifetime < 0.0f) {
        fReprojectedLockStatus[LOCK_LIFETIME_REMAINING] = fInPlaceLockLifetime;
    }
}

#endif //!defined( FFX_FSR2_REPROJECT_H )
