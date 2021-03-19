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

#define SORT_SIZE 512

#if( SORT_SIZE>2048 )
#error
#endif

#define NUM_THREADS		(SORT_SIZE/2)
#define INVERSION		(16*2 + 8*3)

//--------------------------------------------------------------------------------------
// Structured Buffers
//--------------------------------------------------------------------------------------
RAWBUFFER(counterBuffer, 0);
STRUCTUREDBUFFER(comparisonBuffer, float, 1);
RWSTRUCTUREDBUFFER(indexBuffer, uint, 0);


//--------------------------------------------------------------------------------------
// Bitonic Sort Compute Shader
//--------------------------------------------------------------------------------------
groupshared float2	g_LDS[SORT_SIZE];


[numthreads(NUM_THREADS, 1, 1)]
void main(uint3 Gid	: SV_GroupID,
	uint3 DTid : SV_DispatchThreadID,
	uint3 GTid : SV_GroupThreadID,
	uint	GI : SV_GroupIndex)
{
	uint NumElements = counterBuffer.Load(counterReadOffset);

	uint4 tgp;

	tgp.x = Gid.x * 256;
	tgp.y = 0;
	tgp.z = NumElements;
	tgp.w = min(512, max(0, NumElements - Gid.x * 512));

	uint GlobalBaseIndex = tgp.y + tgp.x * 2 + GTid.x;
	uint LocalBaseIndex = GI;
	uint i;

	// Load shared data
	[unroll]for (i = 0; i < 2; ++i)
	{
		if (GI + i * NUM_THREADS < tgp.w)
		{
			uint particleIndex = indexBuffer[GlobalBaseIndex + i * NUM_THREADS];
			float dist = comparisonBuffer[particleIndex];
			g_LDS[LocalBaseIndex + i * NUM_THREADS] = float2(dist, (float)particleIndex);
		}
	}
	GroupMemoryBarrierWithGroupSync();

	// sort threadgroup shared memory
	for (int nMergeSubSize = SORT_SIZE >> 1; nMergeSubSize > 0; nMergeSubSize = nMergeSubSize >> 1)
	{
		int tmp_index = GI;
		int index_low = tmp_index & (nMergeSubSize - 1);
		int index_high = 2 * (tmp_index - index_low);
		int index = index_high + index_low;

		uint nSwapElem = index_high + nMergeSubSize + index_low;

		if (nSwapElem < tgp.w)
		{
			float2 a = g_LDS[index];
			float2 b = g_LDS[nSwapElem];

			if (a.x > b.x)
			{
				g_LDS[index] = b;
				g_LDS[nSwapElem] = a;
			}
		}
		GroupMemoryBarrierWithGroupSync();
	}

	// Store shared data
	[unroll]for (i = 0; i < 2; ++i)
	{
		if (GI + i * NUM_THREADS < tgp.w)
		{
			indexBuffer[GlobalBaseIndex + i * NUM_THREADS] = (uint)g_LDS[LocalBaseIndex + i * NUM_THREADS].y;
		}
	}
}
