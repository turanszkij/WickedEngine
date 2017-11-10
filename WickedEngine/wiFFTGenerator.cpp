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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "wiFFTGenerator.h"
#include "wiResourceManager.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

static const GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE; // todo: make dynamic

void radix008A(CSFFT512x512_Plan* fft_plan,
	GPUUnorderedResource* pUAV_Dst,
	GPUResource* pSRV_Src,
	UINT thread_count,
	UINT istride)
{
	// Setup execution configuration
	UINT grid = thread_count / COHERENCY_GRANULARITY;

	GraphicsDevice* device = wiRenderer::GetDevice();

	// Buffers
	GPUResource* cs_srvs[1] = { pSRV_Src };
	device->BindResourcesCS(cs_srvs, 0, 1, threadID);

	GPUUnorderedResource* cs_uavs[1] = { pUAV_Dst };
	device->BindUnorderedAccessResourcesCS(cs_uavs, 0, 1, threadID);

	// Shader
	if (istride > 1)
		device->BindCS(fft_plan->pRadix008A_CS, threadID);
	else
		device->BindCS(fft_plan->pRadix008A_CS2, threadID);

	// Execute
	device->Dispatch(grid, 1, 1, threadID);

	// Unbind resource
	device->UnBindResources(0, 1, threadID);
	device->UnBindUnorderedAccessResources(0, 1, threadID);
}

void fft_512x512_c2c(CSFFT512x512_Plan* fft_plan,
	GPUUnorderedResource* pUAV_Dst,
	GPUResource* pSRV_Dst,
	GPUResource* pSRV_Src)
{
	const UINT thread_count = fft_plan->slices * (512 * 512) / 8;
	GPUUnorderedResource* pUAV_Tmp = fft_plan->pUAV_Tmp;
	GPUResource* pSRV_Tmp = fft_plan->pSRV_Tmp;
	GraphicsDevice* device = wiRenderer::GetDevice();
	GPUBuffer* cs_cbs;

	UINT istride = 512 * 512 / 8;
	cs_cbs = fft_plan->pRadix008A_CB[0];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Src, thread_count, istride);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[1];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[2];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Dst, thread_count, istride);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[3];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[4];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Tmp, pSRV_Dst, thread_count, istride);

	istride /= 8;
	cs_cbs = fft_plan->pRadix008A_CB[5];
	device->BindConstantBufferCS(&cs_cbs[0], 0, threadID);
	radix008A(fft_plan, pUAV_Dst, pSRV_Tmp, thread_count, istride);
}

void create_cbuffers_512x512(CSFFT512x512_Plan* plan, GraphicsDevice* device, UINT slices)
{
	// Create 6 cbuffers for 512x512 transform.

	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = 32;//sizeof(float) * 5;
	cb_desc.StructureByteStride = 0;

	SubresourceData cb_data;
	cb_data.SysMemPitch = 0;
	cb_data.SysMemSlicePitch = 0;

	struct CB_Structure
	{
		UINT thread_count;
		UINT ostride;
		UINT istride;
		UINT pstride;
		float phase_base;
	};

	for (int i = 0; i < ARRAYSIZE(plan->pRadix008A_CB); ++i)
	{
		plan->pRadix008A_CB[i] = new GPUBuffer;
	}

	// Buffer 0
	const UINT thread_count = slices * (512 * 512) / 8;
	UINT ostride = 512 * 512 / 8;
	UINT istride = ostride;
	double phase_base = -TWO_PI / (512.0 * 512.0);

	CB_Structure cb_data_buf0 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf0;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[0]);
	assert(plan->pRadix008A_CB[0]);

	// Buffer 1
	istride /= 8;
	phase_base *= 8.0;

	CB_Structure cb_data_buf1 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf1;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[1]);
	assert(plan->pRadix008A_CB[1]);

	// Buffer 2
	istride /= 8;
	phase_base *= 8.0;

	CB_Structure cb_data_buf2 = { thread_count, ostride, istride, 512, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf2;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[2]);
	assert(plan->pRadix008A_CB[2]);

	// Buffer 3
	istride /= 8;
	phase_base *= 8.0;
	ostride /= 512;

	CB_Structure cb_data_buf3 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf3;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[3]);
	assert(plan->pRadix008A_CB[3]);

	// Buffer 4
	istride /= 8;
	phase_base *= 8.0;

	CB_Structure cb_data_buf4 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf4;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[4]);
	assert(plan->pRadix008A_CB[4]);

	// Buffer 5
	istride /= 8;
	phase_base *= 8.0;

	CB_Structure cb_data_buf5 = { thread_count, ostride, istride, 1, (float)phase_base };
	cb_data.pSysMem = &cb_data_buf5;

	device->CreateBuffer(&cb_desc, &cb_data, plan->pRadix008A_CB[5]);
	assert(plan->pRadix008A_CB[5]);
}

void fft512x512_create_plan(CSFFT512x512_Plan* plan, UINT slices)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	plan->slices = slices;

	plan->pRadix008A_CS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fft_512x512_c2c_CS.cso", wiResourceManager::COMPUTESHADER));
	plan->pRadix008A_CS2 = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "fft_512x512_c2c_v2_CS.cso", wiResourceManager::COMPUTESHADER));


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
	device->CreateBuffer(&buf_desc, NULL, plan->pBuffer_Tmp);

	plan->pSRV_Tmp = (GPUResource*)plan->pBuffer_Tmp;
	plan->pUAV_Tmp = (GPUUnorderedResource*)plan->pBuffer_Tmp;
}

void fft512x512_destroy_plan(CSFFT512x512_Plan* plan)
{
	SAFE_DELETE(plan->pBuffer_Tmp);
	SAFE_DELETE(plan->pRadix008A_CS);
	SAFE_DELETE(plan->pRadix008A_CS2);

	for (int i = 0; i < 6; i++)
		SAFE_DELETE(plan->pRadix008A_CB[i]);
}
