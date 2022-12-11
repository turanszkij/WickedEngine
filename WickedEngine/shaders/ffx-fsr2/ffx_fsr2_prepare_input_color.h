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

#ifndef FFX_FSR2_PREPARE_INPUT_COLOR_H
#define FFX_FSR2_PREPARE_INPUT_COLOR_H

//TODO: Move to common location & share with Accumulate
void ClearResourcesForNextFrame(in FfxInt32x2 iPxHrPos)
{
    if (all(FFX_LESS_THAN(iPxHrPos, FfxInt32x2(RenderSize()))))
    {
#if FFX_FSR2_OPTION_INVERTED_DEPTH
        const FfxUInt32 farZ = 0x0;
#else
        const FfxUInt32 farZ = 0x3f800000;
#endif
        SetReconstructedDepth(iPxHrPos, farZ);
    }
}

void ComputeLumaStabilityFactor(FfxInt32x2 iPxLrPos, FfxFloat32 fCurrentFrameLuma)
{
    FfxFloat32x4 fCurrentFrameLumaHistory = LoadRwLumaHistory(iPxLrPos);

    fCurrentFrameLumaHistory.a = FfxFloat32(0);

    if (FrameIndex() > 3) {
        FfxFloat32 fDiffs0 = MinDividedByMax(fCurrentFrameLumaHistory[2], fCurrentFrameLuma);
        FfxFloat32 fDiffs1 = ffxMax(MinDividedByMax(fCurrentFrameLumaHistory[0], fCurrentFrameLuma), MinDividedByMax(fCurrentFrameLumaHistory[1], fCurrentFrameLuma));

        fCurrentFrameLumaHistory.a = ffxSaturate(fDiffs1 - fDiffs0);
    }

    //move history
    fCurrentFrameLumaHistory[0] = fCurrentFrameLumaHistory[1];
    fCurrentFrameLumaHistory[1] = fCurrentFrameLumaHistory[2];
    fCurrentFrameLumaHistory[2] = fCurrentFrameLuma;

    StoreLumaHistory(iPxLrPos, fCurrentFrameLumaHistory);
}

void PrepareInputColor(FfxInt32x2 iPxLrPos)
{
    //We assume linear data. if non-linear input (sRGB, ...),
    //then we should convert to linear first and back to sRGB on output.

    FfxFloat32x3 fRgb = ffxMax(FfxFloat32x3(0, 0, 0), LoadInputColor(iPxLrPos));

    fRgb *= Exposure();

#if FFX_FSR2_OPTION_HDR_COLOR_INPUT
    // Tonemap color, used in lockstatus and luma stability computations
    fRgb = Tonemap(fRgb);
#endif

    FfxFloat32x4 fYCoCg;

    fYCoCg.xyz = RGBToYCoCg(fRgb);

    const FfxFloat32 fPerceivedLuma = RGBToPerceivedLuma(fRgb);
    ComputeLumaStabilityFactor(iPxLrPos, fPerceivedLuma);

    //compute luma used to lock pixels, if used elsewhere the ffxPow must be moved!
    fYCoCg.w = ffxPow(fPerceivedLuma, FfxFloat32(1.0 / 6.0));

    StorePreparedInputColor(iPxLrPos, fYCoCg);
    ClearResourcesForNextFrame(iPxLrPos);
}

#endif // FFX_FSR2_PREPARE_INPUT_COLOR_H
