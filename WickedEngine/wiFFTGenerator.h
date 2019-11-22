#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

namespace wiFFTGenerator
{

	struct CSFFT512x512_Plan
	{
		// More than one array can be transformed at same time
		uint32_t slices;

		// For 512x512 config, we need 6 constant buffers
		wiGraphics::GPUBuffer pRadix008A_CB[6];

		wiGraphics::GPUBuffer pBuffer_Tmp;
	};


	void fft512x512_create_plan(CSFFT512x512_Plan& plan, uint32_t slices);

	void fft_512x512_c2c(
		const CSFFT512x512_Plan& fft_plan,
		const wiGraphics::GPUResource& pUAV_Dst,
		const wiGraphics::GPUResource& pSRV_Dst,
		const wiGraphics::GPUResource& pSRV_Src,
		wiGraphics::CommandList cmd);

	void LoadShaders();
}
