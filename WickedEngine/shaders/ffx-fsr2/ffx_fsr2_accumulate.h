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

#ifndef FFX_FSR2_ACCUMULATE_H
#define FFX_FSR2_ACCUMULATE_H

#define FFX_FSR2_OPTION_GUARANTEE_UPSAMPLE_WEIGHT_ON_NEW_SAMPLES 1

FfxFloat32 GetPxHrVelocity(FfxFloat32x2 fMotionVector)
{
    return length(fMotionVector * DisplaySize());
}
#if FFX_HALF
FFX_MIN16_F GetPxHrVelocity(FFX_MIN16_F2 fMotionVector)
{
    return length(fMotionVector * FFX_MIN16_F2(DisplaySize()));
}
#endif

void Accumulate(FfxInt32x2 iPxHrPos, FFX_PARAMETER_INOUT FfxFloat32x4 fHistory, FFX_PARAMETER_IN FfxFloat32x4 fUpsampled, FFX_PARAMETER_IN FfxFloat32 fDepthClipFactor, FFX_PARAMETER_IN FfxFloat32 fHrVelocity)
{
    fHistory.w = fHistory.w + fUpsampled.w;

    fUpsampled.rgb = YCoCgToRGB(fUpsampled.rgb);

    const FfxFloat32 fAlpha = fUpsampled.w / fHistory.w;
    fHistory.rgb = ffxLerp(fHistory.rgb, fUpsampled.rgb, fAlpha);

    FfxFloat32 fMaxAverageWeight = FfxFloat32(ffxLerp(MaxAccumulationWeight(), accumulationMaxOnMotion, ffxSaturate(fHrVelocity * 10.0f)));
    fHistory.w = ffxMin(fHistory.w, fMaxAverageWeight);
}

void RectifyHistory(
    RectificationBoxData clippingBox,
    inout FfxFloat32x4 fHistory,
    FFX_PARAMETER_IN FfxFloat32x3 fLockStatus,
    FFX_PARAMETER_IN FfxFloat32 fDepthClipFactor,
    FFX_PARAMETER_IN FfxFloat32 fLumaStabilityFactor,
    FFX_PARAMETER_IN FfxFloat32 fLuminanceDiff,
    FFX_PARAMETER_IN FfxFloat32 fUpsampleWeight,
    FFX_PARAMETER_IN FfxFloat32 fLockContributionThisFrame)
{
    FfxFloat32 fScaleFactorInfluence = FfxFloat32(1.0f / DownscaleFactor().x - 1);
    FfxFloat32 fBoxScale = FfxFloat32(1.0f) + (FfxFloat32(0.5f) * fScaleFactorInfluence);

    FfxFloat32x3 fScaledBoxVec = clippingBox.boxVec * fBoxScale;
    FfxFloat32x3 boxMin = clippingBox.boxCenter - fScaledBoxVec;
    FfxFloat32x3 boxMax = clippingBox.boxCenter + fScaledBoxVec;
    FfxFloat32x3 boxCenter = clippingBox.boxCenter;
    FfxFloat32 boxVecSize = length(clippingBox.boxVec);

    boxMin = ffxMax(clippingBox.aabbMin, boxMin);
    boxMax = ffxMin(clippingBox.aabbMax, boxMax);

    FfxFloat32x3 distToClampOutside = ffxMax(ffxMax(FfxFloat32x3(0, 0, 0), boxMin - fHistory.xyz), ffxMax(FfxFloat32x3(0, 0, 0), fHistory.xyz - boxMax));

    if (any(FFX_GREATER_THAN(distToClampOutside, FfxFloat32x3(0, 0, 0)))) {

        const FfxFloat32x3 clampedHistorySample = clamp(fHistory.xyz, boxMin, boxMax);

        FfxFloat32x3 clippedHistoryToBoxCenter = abs(clampedHistorySample - boxCenter);
        FfxFloat32x3 historyToBoxCenter = abs(fHistory.xyz - boxCenter);
        FfxFloat32x3 HistoryColorWeight;
        HistoryColorWeight.x = historyToBoxCenter.x > FfxFloat32(0) ? clippedHistoryToBoxCenter.x / historyToBoxCenter.x : FfxFloat32(0.0f);
        HistoryColorWeight.y = historyToBoxCenter.y > FfxFloat32(0) ? clippedHistoryToBoxCenter.y / historyToBoxCenter.y : FfxFloat32(0.0f);
        HistoryColorWeight.z = historyToBoxCenter.z > FfxFloat32(0) ? clippedHistoryToBoxCenter.z / historyToBoxCenter.z : FfxFloat32(0.0f);

        FfxFloat32x3 fHistoryContribution = HistoryColorWeight;

        // only lock luma
        fHistoryContribution += ffxMax(fLockContributionThisFrame, fLumaStabilityFactor).xxx;
        fHistoryContribution *= (fDepthClipFactor * fDepthClipFactor);

        fHistory.xyz = ffxLerp(clampedHistorySample.xyz, fHistory.xyz, ffxSaturate(fHistoryContribution));
    }
}

void WriteUpscaledOutput(FfxInt32x2 iPxHrPos, FfxFloat32x3 fUpscaledColor)
{
    StoreUpscaledOutput(iPxHrPos, fUpscaledColor);
}

FfxFloat32 GetLumaStabilityFactor(FfxFloat32x2 fHrUv, FfxFloat32 fHrVelocity)
{
    FfxFloat32 fLumaStabilityFactor = SampleLumaStabilityFactor(fHrUv);

    // Only apply on still, have to reproject luma history resource if we want it to work on motion
    fLumaStabilityFactor *= FfxFloat32(fHrVelocity < 0.1f);

    return fLumaStabilityFactor;
}

FfxFloat32 GetLockContributionThisFrame(FfxFloat32x2 fUvCoord, FfxFloat32 fAccumulationMask, FfxFloat32 fParticleMask, FfxFloat32x3 fLockStatus)
{
    const FfxFloat32 fNormalizedLockLifetime = GetNormalizedRemainingLockLifetime(fLockStatus);

    // Rectify on lock frame
    FfxFloat32 fLockContributionThisFrame = ffxSaturate(fNormalizedLockLifetime * FfxFloat32(4));

    return fLockContributionThisFrame;
}

void FinalizeLockStatus(FfxInt32x2 iPxHrPos, FfxFloat32x3 fLockStatus, FfxFloat32 fUpsampledWeight)
{
    // Increase trust
    const FfxFloat32 fTrustIncreaseLanczosMax = FfxFloat32(12); // same increase no matter the MaxAccumulationWeight() value.
    const FfxFloat32 fTrustIncrease = FfxFloat32(fUpsampledWeight / fTrustIncreaseLanczosMax);
    fLockStatus[LOCK_TRUST] = ffxMin(FfxFloat32(1), fLockStatus[LOCK_TRUST] + fTrustIncrease);

    // Decrease lock lifetime
    const FfxFloat32 fLifetimeDecreaseLanczosMax = FfxFloat32(JitterSequenceLength()) * FfxFloat32(averageLanczosWeightPerFrame);
    const FfxFloat32 fLifetimeDecrease = FfxFloat32(fUpsampledWeight / fLifetimeDecreaseLanczosMax);
    fLockStatus[LOCK_LIFETIME_REMAINING] = ffxMax(FfxFloat32(0), fLockStatus[LOCK_LIFETIME_REMAINING] - fLifetimeDecrease);

    StoreLockStatus(iPxHrPos, fLockStatus);
}

FfxFloat32 ComputeMaxAccumulationWeight(FfxFloat32 fHrVelocity, FfxFloat32 fReactiveMax, FfxFloat32 fDepthClipFactor, FfxFloat32 fLuminanceDiff, LockState lockState) {

    FfxFloat32 normalizedMinimum = FfxFloat32(accumulationMaxOnMotion) / FfxFloat32(MaxAccumulationWeight());

    FfxFloat32 fReactiveMaxAccumulationWeight = FfxFloat32(1) - fReactiveMax;
    FfxFloat32 fMotionMaxAccumulationWeight = ffxLerp(FfxFloat32(1), normalizedMinimum, ffxSaturate(fHrVelocity * FfxFloat32(10)));
    FfxFloat32 fDepthClipMaxAccumulationWeight = fDepthClipFactor;

    FfxFloat32 fLuminanceDiffMaxAccumulationWeight = ffxSaturate(ffxMax(normalizedMinimum, FfxFloat32(1) - fLuminanceDiff));

    FfxFloat32 maxAccumulation = FfxFloat32(MaxAccumulationWeight()) * ffxMin(
        ffxMin(fReactiveMaxAccumulationWeight, fMotionMaxAccumulationWeight),
        ffxMin(fDepthClipMaxAccumulationWeight, fLuminanceDiffMaxAccumulationWeight)
    );

    return (lockState.NewLock && !lockState.WasLockedPrevFrame) ? FfxFloat32(accumulationMaxOnMotion) : maxAccumulation;
}

FfxFloat32x2 ComputeKernelWeight(in FfxFloat32 fHistoryWeight, in FfxFloat32 fDepthClipFactor, in FfxFloat32 fReactivityFactor) {
    FfxFloat32 fKernelSizeBias = ffxSaturate(ffxMax(FfxFloat32(0), fHistoryWeight - FfxFloat32(0.5)) / FfxFloat32(3));

    FfxFloat32 fOneMinusReactiveMax = FfxFloat32(1) - fReactivityFactor;
    FfxFloat32x2 fKernelWeight = FfxFloat32(1) + (FfxFloat32(1.0f) / FfxFloat32x2(DownscaleFactor()) - FfxFloat32(1)) * FfxFloat32(fKernelSizeBias) * fOneMinusReactiveMax;

    //average value on disocclusion, to help decrease high value sample importance wait for accumulation to kick in
    fKernelWeight *= FfxFloat32x2(0.5f, 0.5f) + fDepthClipFactor * FfxFloat32x2(0.5f, 0.5f);

    return ffxMin(FfxFloat32x2(1.99f, 1.99f), fKernelWeight);
}

void Accumulate(FfxInt32x2 iPxHrPos)
{
    const FfxFloat32x2 fSamplePosHr = iPxHrPos + 0.5f;
    const FfxFloat32x2 fPxLrPos = fSamplePosHr * DownscaleFactor();                   // Source resolution output pixel center position
    const FfxInt32x2 iPxLrPos = FfxInt32x2(floor(fPxLrPos));                             // TODO: what about weird upscale factors...

    const FfxFloat32x2 fSamplePosUnjitterLr = (FfxFloat32x2(iPxLrPos) + FfxFloat32x2(0.5f, 0.5f)) - Jitter();                // This is the un-jittered position of the sample at offset 0,0

    const FfxFloat32x2 fLrUvJittered = (fPxLrPos + Jitter()) / RenderSize();

    const FfxFloat32x2 fHrUv = (iPxHrPos + 0.5f) / DisplaySize();
    const FfxFloat32x2 fMotionVector = GetMotionVector(iPxHrPos, fHrUv);

    const FfxFloat32 fHrVelocity = GetPxHrVelocity(fMotionVector);
    const FfxFloat32 fDepthClipFactor = ffxSaturate(SampleDepthClip(fLrUvJittered));
    const FfxFloat32 fLumaStabilityFactor = GetLumaStabilityFactor(fHrUv, fHrVelocity);
    const FfxFloat32x2 fDilatedReactiveMasks = SampleDilatedReactiveMasks(fLrUvJittered);
    const FfxFloat32 fReactiveMax = fDilatedReactiveMasks.x;
    const FfxFloat32 fAccumulationMask = fDilatedReactiveMasks.y;
    const FfxBoolean bIsResetFrame = (0 == FrameIndex());

    FfxFloat32x4 fHistoryColorAndWeight = FfxFloat32x4(0, 0, 0, 0);
    FfxFloat32x3 fLockStatus;
    InitializeNewLockSample(fLockStatus);
    FfxBoolean bIsExistingSample = FFX_TRUE;

    FfxFloat32x2 fReprojectedHrUv = FfxFloat32x2(0, 0);
    ComputeReprojectedUVs(iPxHrPos, fMotionVector, fReprojectedHrUv, bIsExistingSample);

    if (bIsExistingSample && !bIsResetFrame) {
        ReprojectHistoryColor(iPxHrPos, fReprojectedHrUv, fHistoryColorAndWeight);
        ReprojectHistoryLockStatus(iPxHrPos, fReprojectedHrUv, fLockStatus);
    }

    FfxFloat32 fLuminanceDiff = FfxFloat32(0.0f);

    LockState lockState = PostProcessLockStatus(iPxHrPos, fLrUvJittered, FfxFloat32(fDepthClipFactor), fAccumulationMask, fHrVelocity, fHistoryColorAndWeight.w, fLockStatus, fLuminanceDiff);

    fHistoryColorAndWeight.w = ffxMin(fHistoryColorAndWeight.w, ComputeMaxAccumulationWeight(
        FfxFloat32(fHrVelocity), fReactiveMax, FfxFloat32(fDepthClipFactor), FfxFloat32(fLuminanceDiff), lockState
    ));

    const FfxFloat32 fNormalizedLockLifetime = GetNormalizedRemainingLockLifetime(fLockStatus);

    // Kill accumulation based on shading change
    fHistoryColorAndWeight.w = ffxMin(fHistoryColorAndWeight.w, FfxFloat32(ffxMax(0.0f, MaxAccumulationWeight() * ffxPow(FfxFloat32(1) - fLuminanceDiff, 2.0f / 1.0f))));

    // Load upsampled input color
    RectificationBoxData clippingBox;

    FfxFloat32 fKernelBias = fAccumulationMask * ffxSaturate(ffxMax(0.0f, fHistoryColorAndWeight.w - 0.5f) / 3.0f);

    FfxFloat32 fReactiveWeighted = 0;

    // No trust in reactive areas
    fLockStatus[LOCK_TRUST] = ffxMin(fLockStatus[LOCK_TRUST], FfxFloat32(1.0f) - FfxFloat32(pow(fReactiveMax, 1.0f / 3.0f)));
    fLockStatus[LOCK_TRUST] = ffxMin(fLockStatus[LOCK_TRUST], FfxFloat32(fDepthClipFactor));

    FfxFloat32x2 fKernelWeight = ComputeKernelWeight(fHistoryColorAndWeight.w, FfxFloat32(fDepthClipFactor), ffxMax((FfxFloat32(1) - fLockStatus[LOCK_TRUST]), fReactiveMax));

    FfxFloat32x4 fUpsampledColorAndWeight = ComputeUpsampledColorAndWeight(iPxHrPos, fKernelWeight, clippingBox);

#if FFX_FSR2_OPTION_GUARANTEE_UPSAMPLE_WEIGHT_ON_NEW_SAMPLES
    // Make sure all samples have same weight on reset/first frame. Upsampled weight should never be 0.0f when history accumulation is 0.0f.
    fUpsampledColorAndWeight.w = (fHistoryColorAndWeight.w == 0.0f) ? ffxMax(FSR2_EPSILON, fUpsampledColorAndWeight.w) : fUpsampledColorAndWeight.w;
#endif

    FfxFloat32 fLockContributionThisFrame = GetLockContributionThisFrame(fHrUv, fAccumulationMask, fReactiveMax, fLockStatus);

    // Update accumulation and rectify history
    if (fHistoryColorAndWeight.w > FfxFloat32(0)) {

        RectifyHistory(clippingBox, fHistoryColorAndWeight, fLockStatus, FfxFloat32(fDepthClipFactor), FfxFloat32(fLumaStabilityFactor), FfxFloat32(fLuminanceDiff), fUpsampledColorAndWeight.w, fLockContributionThisFrame);

        fHistoryColorAndWeight.rgb = YCoCgToRGB(fHistoryColorAndWeight.rgb);
    }

    Accumulate(iPxHrPos, fHistoryColorAndWeight, fUpsampledColorAndWeight, fDepthClipFactor, fHrVelocity);

    //Subtract accumulation weight in reactive areas
    fHistoryColorAndWeight.w -= fUpsampledColorAndWeight.w * fReactiveMax;

#if FFX_FSR2_OPTION_HDR_COLOR_INPUT
    fHistoryColorAndWeight.rgb = InverseTonemap(fHistoryColorAndWeight.rgb);
#endif
    fHistoryColorAndWeight.rgb /= FfxFloat32(Exposure());

    FinalizeLockStatus(iPxHrPos, fLockStatus, fUpsampledColorAndWeight.w);

    StoreInternalColorAndWeight(iPxHrPos, fHistoryColorAndWeight);

    // Output final color when RCAS is disabled
#if FFX_FSR2_OPTION_APPLY_SHARPENING == 0
    WriteUpscaledOutput(iPxHrPos, fHistoryColorAndWeight.rgb);
#endif
}

#endif // FFX_FSR2_ACCUMULATE_H
