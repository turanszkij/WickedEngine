#include "wiFFTGenerator.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "ShaderInterop_FFTGenerator.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

using namespace wiGraphicsTypes;

static ComputeShader* pRadix008A_CS = nullptr;
static ComputeShader* pRadix008A_CS2 = nullptr;
static ComputePSO PSO1;
static ComputePSO PSO2;

void radix008A(CSFFT512x512_Plan* fft_plan,
	GPUResource* pUAV_Dst,
	GPUResource* pSRV_Src,
	UINT thread_count,
	UINT istride, 
	GRAPHICSTHREAD threadID)
{
	// Setup execution configuration
	UINT grid = thread_count / COHERENCY_GRANULARITY;

	GraphicsDevice* device = wiRenderer::GetDevice();

	// Buffers
	GPUResource* cs_srvs[1] = { pSRV_Src };
	device->BindResources(CS, cs_srvs, TEXSLOT_ONDEMAND0, 1, threadID);

	GPUResource* cs_uavs[1] = { pUAV_Dst };
	device->BindUAVs(CS, cs_uavs, 0, ARRAYSIZE(cs_uavs), threadID);

	// Shader
	if (istride > 1)
	{
		device->BindComputePSO(&PSO1, threadID);
	}
	else
	{
		device->BindComputePSO(&PSO2, threadID);
	}

	// Execute
	device->Dispatch(grid, 1, 1, threadID);

	device->UAVBarrier(cs_uavs, ARRAYSIZE(cs_uavs), threadID);

	// Unbind resource
	device->UnbindResources(TEXSLOT_ONDEMAND0, 1, threadID);
	device->UnbindUAVs(0, 1, threadID);
}

void fft_512x512_c2c(CSFFT512x512_Plan* fft_plan,
	GPUResource* pUAV_Dst,
	GPUResource* pSRV_Dst,
	GPUResource* pSRV_Src, 
	GRAPHICSTHREAD threadID)
{
	const UINT thread_count = fft_plan->slices * (512 * 512) / 8;
	GPUResource* pUAV_Tmp = fft_plan->pUAV_Tmp;
	GPUResource* pSRV_Tmp = fft_plan->pSRV_Tmp;
	GraphicsDevice* device = wiRenderer::GetDevice();
	GPUBuffer* cs_cbs;

	UINT istride = 512 * 512 / 8;
	cs_cbs = fft_plan->pRadix008A_CB[0];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Src, thread_count, istride, threadID);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[1];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride, threadID);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[2];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Dst, thread_count, istride, threadID);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[3];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride, threadID);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[4];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Dst, thread_count, istride, threadID);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[5];
	device->BindConstantBuffer(CS, &cs_cbs[0], CB_GETBINDSLOT(FFTGeneratorCB), threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride, threadID);
}

void create_cbuffers_512x512(CSFFT512x512_Plan* plan, GraphicsDevice* device, UINT slices)
{
	// Create 6 cbuffers for 512x512 transform.

	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = sizeof(FFTGeneratorCB);
	cb_desc.StructureByteStride = 0;

	SubresourceData cb_data;
	cb_data.SysMemPitch = 0;
	cb_data.SysMemSlicePitch = 0;

	//struct CB_Structure
	//{
	//	UINT thread_count;
	//	UINT ostride;
	//	UINT istride;
	//	UINT pstride;
	//	float phase_base;
	//};

	for (int i = 0; i < ARRAYSIZE(plan->pRadix008A_CB); ++i)
	{
		plan->pRadix008A_CB[i] = new GPUBuffer;
	}

	// Buffer 0
	const UINT thread_count = slices * (512 * 512) / 8;
	UINT ostride = 512 * 512 / 8;
	UINT istride = ostride;
	double phase_base = -TWO_PI / (512.0 * 512.0);

	FFTGeneratorCB cb_data_buf0 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf0;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[0]);
	assert(plan->pRadix008A_CB[0]);

	// Buffer 1
	istride /= 8;
	phase_base *= 8.0;

	FFTGeneratorCB cb_data_buf1 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf1;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[1]);
	assert(plan->pRadix008A_CB[1]);

	// Buffer 2
	istride /= 8;
	phase_base *= 8.0;

	FFTGeneratorCB cb_data_buf2 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf2;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[2]);
	assert(plan->pRadix008A_CB[2]);

	// Buffer 3
	istride /= 8;
	phase_base *= 8.0;
	ostride /= 512;

	FFTGeneratorCB cb_data_buf3 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf3;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[3]);
	assert(plan->pRadix008A_CB[3]);

	// Buffer 4
	istride /= 8;
	phase_base *= 8.0;

	FFTGeneratorCB cb_data_buf4 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf4;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[4]);
	assert(plan->pRadix008A_CB[4]);

	// Buffer 5
	istride /= 8;
	phase_base *= 8.0;

	FFTGeneratorCB cb_data_buf5 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf5;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[5]);
	assert(plan->pRadix008A_CB[5]);
}

void fft512x512_create_plan(CSFFT512x512_Plan* plan, UINT slices)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	plan->slices = slices;


	// Constants
	// Create 6 cbuffers for 512x512 transform
	create_cbuffers_512x512(plan, device, slices);

	// Temp buffer
	GPUBufferDesc buf_desc;
	buf_desc.ByteWidth = sizeof(float) * 2 * (512 * slices) * 512;
	buf_desc.Usage = USAGE_DEFAULT;
	buf_desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	buf_desc.CPUAccessFlags = 0;
	buf_desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	buf_desc.StructureByteStride = sizeof(float) * 2;

	plan->pBuffer_Tmp = new GPUBuffer;
	device->CreateBuffer(&buf_desc, nullptr, plan->pBuffer_Tmp);

	plan->pSRV_Tmp = (GPUResource*)plan->pBuffer_Tmp;
	plan->pUAV_Tmp = (GPUResource*)plan->pBuffer_Tmp;
}

void fft512x512_destroy_plan(CSFFT512x512_Plan* plan)
{
	SAFE_DELETE(plan->pBuffer_Tmp);

	for (int i = 0; i < 6; i++)
		SAFE_DELETE(plan->pRadix008A_CB[i]);
}



void CSFFT_512x512_Data_t::LoadShaders()
{
	std::string path = wiRenderer::GetShaderPath();

	pRadix008A_CS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path+ "fft_512x512_c2c_CS.cso", wiResourceManager::COMPUTESHADER));
	pRadix008A_CS2 = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager().add(path + "fft_512x512_c2c_v2_CS.cso", wiResourceManager::COMPUTESHADER));

	GraphicsDevice* device = wiRenderer::GetDevice();

	ComputePSODesc desc;
	desc.cs = pRadix008A_CS;
	device->CreateComputePSO(&desc, &PSO1);
	desc.cs = pRadix008A_CS2;
	device->CreateComputePSO(&desc, &PSO2);
}
