#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"

namespace wi::fftgenerator
{
	void fft_512x512_c2c(
		const wi::graphics::GPUResource& pUAV_Dst,
		const wi::graphics::GPUResource& pSRV_Dst,
		const wi::graphics::GPUResource& pSRV_Src,
		wi::graphics::CommandList cmd);

	void LoadShaders();
}
