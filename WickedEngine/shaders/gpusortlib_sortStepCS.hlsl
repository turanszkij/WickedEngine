//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "ShaderInterop_GPUSortLib.h"

//--------------------------------------------------------------------------------------
// Structured Buffers
//--------------------------------------------------------------------------------------
RAWBUFFER(counterBuffer, 0);
STRUCTUREDBUFFER(comparisonBuffer, float, 1);
RWSTRUCTUREDBUFFER(indexBuffer, uint, 0);

[numthreads(256, 1, 1)]
void main(uint3 Gid	: SV_GroupID,
	uint3 GTid : SV_GroupThreadID)
{
	uint NumElements = counterBuffer.Load(counterReadOffset);

	uint4 tgp;

	tgp.x = Gid.x * 256;
	tgp.y = 0;
	tgp.z = NumElements;
	tgp.w = min(512, max(0, NumElements - Gid.x * 512));

	uint localID = tgp.x + GTid.x; // calculate threadID within this sortable-array

	uint index_low = localID & (job_params.x - 1);
	uint index_high = 2 * (localID - index_low);

	uint index = tgp.y + index_high + index_low;
	uint nSwapElem = tgp.y + index_high + job_params.y + job_params.z*index_low;

	if (nSwapElem < tgp.y + tgp.z)
	{
		uint index_a = indexBuffer[index];
		uint index_b = indexBuffer[nSwapElem];
		float a = comparisonBuffer[index_a];
		float b = comparisonBuffer[index_b];

		if (a > b)
		{
			indexBuffer[index] = index_b;
			indexBuffer[nSwapElem] = index_a;
		}
	}
}
