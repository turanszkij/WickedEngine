#ifndef _FFT_GENERATOR_H_
#define _FFT_GENERATOR_H_

#include "CommonInclude.h"
#include "wiGraphicsDevice.h"


//Memory access coherency (in threads)
#define COHERENCY_GRANULARITY 128


///////////////////////////////////////////////////////////////////////////////
// Common types
///////////////////////////////////////////////////////////////////////////////

typedef struct CSFFT_512x512_Data_t
{
	// More than one array can be transformed at same time
	UINT slices;

	// For 512x512 config, we need 6 constant buffers
	wiGraphics::GPUBuffer pRadix008A_CB[6];

	wiGraphics::GPUBuffer pBuffer_Tmp;

	static void LoadShaders();
} CSFFT512x512_Plan;

////////////////////////////////////////////////////////////////////////////////
// Common constants
////////////////////////////////////////////////////////////////////////////////
#define TWO_PI 6.283185307179586476925286766559

#define FFT_DIMENSIONS 3U
#define FFT_PLAN_SIZE_LIMIT (1U << 27)

#define FFT_FORWARD -1
#define FFT_INVERSE 1


void fft512x512_create_plan(CSFFT512x512_Plan& plan, UINT slices);

void fft_512x512_c2c(
	const CSFFT512x512_Plan& fft_plan,
	const wiGraphics::GPUResource& pUAV_Dst,
	const wiGraphics::GPUResource& pSRV_Dst,
	const wiGraphics::GPUResource& pSRV_Src, 
	GRAPHICSTHREAD threadID);


#endif // _FFT_GENERATOR_H_
