#include "wiFFTGenerator.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "shaders/ShaderInterop_FFTGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace wi::graphics;

namespace wi::fftgenerator
{
	Shader radix008A_CS;
	Shader radix008A_CS2;

	struct CSFFT512x512_Plan
	{
		// More than one array can be transformed at same time
		uint32_t slices;

		// For 512x512 config, we need 6 constant buffers
		wi::graphics::GPUBuffer pRadix008A_CB[6];

		wi::graphics::GPUBuffer pBuffer_Tmp;

		inline bool IsValid() const { return pBuffer_Tmp.IsValid(); }
	};
	CSFFT512x512_Plan fft_plan;

#define TWO_PI 6.283185307179586476925286766559

#define FFT_DIMENSIONS 3U
#define FFT_PLAN_SIZE_LIMIT (1U << 27)

#define FFT_FORWARD -1
#define FFT_INVERSE 1

	void radix008A(
		const GPUResource& pUAV_Dst,
		const GPUResource& pSRV_Src,
		uint32_t thread_count,
		uint32_t istride,
		CommandList cmd)
	{
		// Setup execution configuration
		uint32_t grid = thread_count / COHERENCY_GRANULARITY;

		GraphicsDevice* device = wi::graphics::GetDevice();

		// Buffers
		const GPUResource* srvs[1] = { &pSRV_Src };
		device->BindResources(srvs, 0, 1, cmd);

		const GPUResource* uavs[1] = { &pUAV_Dst };
		device->BindUAVs(uavs, 0, arraysize(uavs), cmd);

		// Shader
		if (istride > 1)
		{
			device->BindComputeShader(&radix008A_CS, cmd);
		}
		else
		{
			device->BindComputeShader(&radix008A_CS2, cmd);
		}

		// Execute
		device->Dispatch(grid, 1, 1, cmd);

		GPUBarrier barriers[] = {
			GPUBarrier::Memory(),
		};
		device->Barrier(barriers, arraysize(barriers), cmd);

		// Unbind resource
	}

	void fft_512x512_c2c(
		const GPUResource& pUAV_Dst,
		const GPUResource& pSRV_Dst,
		const GPUResource& pSRV_Src,
		CommandList cmd)
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		if (!fft_plan.IsValid())
		{
			fft_plan.slices = 3;

			// Create 6 cbuffers for 512x512 transform.

			GPUBufferDesc cb_desc;
			cb_desc.bind_flags = BindFlag::CONSTANT_BUFFER;
			cb_desc.size = sizeof(FFTGeneratorCB);
			cb_desc.stride = 0;

			// Buffer 0
			const uint32_t thread_count = fft_plan.slices * (512 * 512) / 8;
			uint32_t ostride = 512 * 512 / 8;
			uint32_t istride = ostride;
			double phase_base = -TWO_PI / (512.0 * 512.0);

			FFTGeneratorCB cb_data_buf0 = { thread_count, ostride, istride, 512, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf0, &fft_plan.pRadix008A_CB[0]);

			// Buffer 1
			istride /= 8;
			phase_base *= 8.0;

			FFTGeneratorCB cb_data_buf1 = { thread_count, ostride, istride, 512, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf1, &fft_plan.pRadix008A_CB[1]);

			// Buffer 2
			istride /= 8;
			phase_base *= 8.0;

			FFTGeneratorCB cb_data_buf2 = { thread_count, ostride, istride, 512, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf2, &fft_plan.pRadix008A_CB[2]);

			// Buffer 3
			istride /= 8;
			phase_base *= 8.0;
			ostride /= 512;

			FFTGeneratorCB cb_data_buf3 = { thread_count, ostride, istride, 1, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf3, &fft_plan.pRadix008A_CB[3]);

			// Buffer 4
			istride /= 8;
			phase_base *= 8.0;

			FFTGeneratorCB cb_data_buf4 = { thread_count, ostride, istride, 1, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf4, &fft_plan.pRadix008A_CB[4]);

			// Buffer 5
			istride /= 8;
			phase_base *= 8.0;

			FFTGeneratorCB cb_data_buf5 = { thread_count, ostride, istride, 1, (float)phase_base };

			device->CreateBuffer(&cb_desc, &cb_data_buf5, &fft_plan.pRadix008A_CB[5]);

			// Temp buffer
			GPUBufferDesc buf_desc;
			buf_desc.size = sizeof(float) * 2 * (512 * fft_plan.slices) * 512;
			buf_desc.usage = Usage::DEFAULT;
			buf_desc.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			buf_desc.misc_flags = ResourceMiscFlag::BUFFER_STRUCTURED;
			buf_desc.stride = sizeof(float) * 2;

			device->CreateBuffer(&buf_desc, nullptr, &fft_plan.pBuffer_Tmp);
		}

		const uint32_t thread_count = fft_plan.slices * (512 * 512) / 8;
		const GPUBuffer* cs_cbs;

		uint32_t istride = 512 * 512 / 8;
		cs_cbs = &fft_plan.pRadix008A_CB[0];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(fft_plan.pBuffer_Tmp, pSRV_Src, thread_count, istride, cmd);

		istride /= 8;
		cs_cbs = &fft_plan.pRadix008A_CB[1];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(pUAV_Dst, fft_plan.pBuffer_Tmp, thread_count, istride, cmd);

		istride /= 8;
		cs_cbs = &fft_plan.pRadix008A_CB[2];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(fft_plan.pBuffer_Tmp, pSRV_Dst, thread_count, istride, cmd);

		istride /= 8;
		cs_cbs = &fft_plan.pRadix008A_CB[3];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(pUAV_Dst, fft_plan.pBuffer_Tmp, thread_count, istride, cmd);

		istride /= 8;
		cs_cbs = &fft_plan.pRadix008A_CB[4];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(fft_plan.pBuffer_Tmp, pSRV_Dst, thread_count, istride, cmd);

		istride /= 8;
		cs_cbs = &fft_plan.pRadix008A_CB[5];
		device->BindConstantBuffer(cs_cbs, CB_GETBINDSLOT(FFTGeneratorCB), cmd);
		radix008A(pUAV_Dst, fft_plan.pBuffer_Tmp, thread_count, istride, cmd);
	}

	void LoadShaders()
	{
		wi::renderer::LoadShader(ShaderStage::CS, radix008A_CS, "fft_512x512_c2c_CS.cso");
		wi::renderer::LoadShader(ShaderStage::CS, radix008A_CS2, "fft_512x512_c2c_v2_CS.cso");
	}

}
