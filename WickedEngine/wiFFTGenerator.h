#ifndef _FFT_GENERATOR_H_
#define _FFT_GENERATOR_H_

// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

#include "CommonInclude.h"
#include "wiGraphicsAPI.h"


//Memory access coherency (in threads)
#define COHERENCY_GRANULARITY 128


///////////////////////////////////////////////////////////////////////////////
// Common types
///////////////////////////////////////////////////////////////////////////////

typedef struct CSFFT_512x512_Data_t
{
	wiGraphicsTypes::ComputeShader* pRadix008A_CS;
	wiGraphicsTypes::ComputeShader* pRadix008A_CS2;

	// More than one array can be transformed at same time
	UINT slices;

	// For 512x512 config, we need 6 constant buffers
	wiGraphicsTypes::GPUBuffer* pRadix008A_CB[6];

	// Temporary buffers
	wiGraphicsTypes::GPUBuffer* pBuffer_Tmp;
	wiGraphicsTypes::GPUUnorderedResource* pUAV_Tmp;
	wiGraphicsTypes::GPUResource* pSRV_Tmp;
} CSFFT512x512_Plan;

////////////////////////////////////////////////////////////////////////////////
// Common constants
////////////////////////////////////////////////////////////////////////////////
#define TWO_PI 6.283185307179586476925286766559

#define FFT_DIMENSIONS 3U
#define FFT_PLAN_SIZE_LIMIT (1U << 27)

#define FFT_FORWARD -1
#define FFT_INVERSE 1


void fft512x512_create_plan(CSFFT512x512_Plan* plan, UINT slices);
void fft512x512_destroy_plan(CSFFT512x512_Plan* plan);

void fft_512x512_c2c(CSFFT512x512_Plan* fft_plan,
	wiGraphicsTypes::GPUUnorderedResource* pUAV_Dst,
	wiGraphicsTypes::GPUResource* pSRV_Dst,
	wiGraphicsTypes::GPUResource* pSRV_Src);


#endif // _FFT_GENERATOR_H_
