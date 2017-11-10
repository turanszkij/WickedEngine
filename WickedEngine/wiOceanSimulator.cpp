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

#include "wiOceanSimulator.h"
#include "wiRenderer.h"
#include "wiResourceManager.h"

using namespace wiGraphicsTypes;

static const GRAPHICSTHREAD threadID = GRAPHICSTHREAD_IMMEDIATE; // todo: make dynamic

// Disable warning "conditional expression is constant"
#pragma warning(disable:4127)


#define HALF_SQRT_2	0.7071068f
#define GRAV_ACCEL	981.0f	// The acceleration of gravity, cm/s^2

#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

// Generating gaussian random number with mean 0 and standard deviation 1.
float Gauss()
{
	float u1 = rand() / (float)RAND_MAX;
	float u2 = rand() / (float)RAND_MAX;
	if (u1 < 1e-6f)
		u1 = 1e-6f;
	return sqrtf(-2 * logf(u1)) * cosf(2 * XM_PI * u2);
}

// Phillips Spectrum
// K: normalized wave vector, W: wind direction, v: wind velocity, a: amplitude constant
float Phillips(XMFLOAT2 K, XMFLOAT2 W, float v, float a, float dir_depend)
{
	// largest possible wave from constant wind of velocity v
	float l = v * v / GRAV_ACCEL;
	// damp out waves with very small length w << l
	float w = l / 1000;

	float Ksqr = K.x * K.x + K.y * K.y;
	float Kcos = K.x * W.x + K.y * W.y;
	float phillips = a * expf(-1 / (l * l * Ksqr)) / (Ksqr * Ksqr * Ksqr) * (Kcos * Kcos);

	// filter out waves moving opposite to wind
	if (Kcos < 0)
		phillips *= dir_depend;

	// damp out waves with very small length w << l
	return phillips * expf(-Ksqr * w * w);
}

void createBufferAndUAV(void* data, UINT byte_width, UINT byte_stride, GPUBuffer** ppBuffer)
{
	*ppBuffer = new GPUBuffer;

	// Create buffer
	GPUBufferDesc buf_desc;
	buf_desc.ByteWidth = byte_width;
	buf_desc.Usage = USAGE_DEFAULT;
	buf_desc.BindFlags = BIND_UNORDERED_ACCESS | BIND_SHADER_RESOURCE;
	buf_desc.CPUAccessFlags = 0;
	buf_desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	buf_desc.StructureByteStride = byte_stride;

	SubresourceData init_data;
	init_data.pSysMem = data;

	wiRenderer::GetDevice()->CreateBuffer(&buf_desc, data != NULL ? &init_data : NULL, *ppBuffer);


	//assert(*ppBuffer);

	//// Create undordered access view
	//D3D11_UNORDERED_ACCESS_VIEW_DESC uav_desc;
	//uav_desc.Format = DXGI_FORMAT_UNKNOWN;
	//uav_desc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	//uav_desc.Buffer.FirstElement = 0;
	//uav_desc.Buffer.NumElements = byte_width / byte_stride;
	//uav_desc.Buffer.Flags = 0;

	//pd3dDevice->CreateUnorderedAccessView(*ppBuffer, &uav_desc, ppUAV);
	//assert(*ppUAV);

	//// Create shader resource view
	//D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	//srv_desc.Format = DXGI_FORMAT_UNKNOWN;
	//srv_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	//srv_desc.Buffer.FirstElement = 0;
	//srv_desc.Buffer.NumElements = byte_width / byte_stride;

	//pd3dDevice->CreateShaderResourceView(*ppBuffer, &srv_desc, ppSRV);
	//assert(*ppSRV);
}

void createTextureAndViews(UINT width, UINT height, FORMAT format, Texture2D** ppTex)
{
	// Create 2D texture
	Texture2DDesc tex_desc;
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.MipLevels = 0;
	tex_desc.ArraySize = 1;
	tex_desc.Format = format;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.SampleDesc.Quality = 0;
	tex_desc.Usage = USAGE_DEFAULT;
	tex_desc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
	tex_desc.CPUAccessFlags = 0;
	tex_desc.MiscFlags = RESOURCE_MISC_GENERATE_MIPS;

	*ppTex = new Texture2D;
	wiRenderer::GetDevice()->CreateTexture2D(&tex_desc, NULL, ppTex);


	//assert(*ppTex);

	//// Create shader resource view
	//(*ppTex)->GetDesc(&tex_desc);
	//if (ppSRV)
	//{
	//	D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
	//	srv_desc.Format = format;
	//	srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	//	srv_desc.Texture2D.MipLevels = tex_desc.MipLevels;
	//	srv_desc.Texture2D.MostDetailedMip = 0;

	//	pd3dDevice->CreateShaderResourceView(*ppTex, &srv_desc, ppSRV);
	//	assert(*ppSRV);
	//}

	//// Create render target view
	//if (ppRTV)
	//{
	//	D3D11_RENDER_TARGET_VIEW_DESC rtv_desc;
	//	rtv_desc.Format = format;
	//	rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	//	rtv_desc.Texture2D.MipSlice = 0;

	//	pd3dDevice->CreateRenderTargetView(*ppTex, &rtv_desc, ppRTV);
	//	assert(*ppRTV);
	//}
}

OceanSimulator::OceanSimulator(OceanParameter& params)
{
	// Height map H(0)
	int height_map_size = (params.dmap_dim + 4) * (params.dmap_dim + 1);
	XMFLOAT2* h0_data = new XMFLOAT2[height_map_size * sizeof(XMFLOAT2)];
	float* omega_data = new float[height_map_size * sizeof(float)];
	initHeightMap(params, h0_data, omega_data);

	m_param = params;
	int hmap_dim = params.dmap_dim;
	int input_full_size = (hmap_dim + 4) * (hmap_dim + 1);
	// This value should be (hmap_dim / 2 + 1) * hmap_dim, but we use full sized buffer here for simplicity.
	int input_half_size = hmap_dim * hmap_dim;
	int output_size = hmap_dim * hmap_dim;

	// For filling the buffer with zeroes.
	char* zero_data = new char[3 * output_size * sizeof(float) * 2];
	memset(zero_data, 0, 3 * output_size * sizeof(float) * 2);

	// RW buffer allocations
	// H0
	UINT float2_stride = 2 * sizeof(float);
	createBufferAndUAV(h0_data, input_full_size * float2_stride, float2_stride, &m_pBuffer_Float2_H0);

	// Notice: The following 3 buffers should be half sized buffer because of conjugate symmetric input. But
	// we use full sized buffers due to the CS4.0 restriction.

	// Put H(t), Dx(t) and Dy(t) into one buffer because CS4.0 allows only 1 UAV at a time
	createBufferAndUAV(zero_data, 3 * input_half_size * float2_stride, float2_stride, &m_pBuffer_Float2_Ht);

	// omega
	createBufferAndUAV(omega_data, input_full_size * sizeof(float), sizeof(float), &m_pBuffer_Float_Omega);

	// Notice: The following 3 should be real number data. But here we use the complex numbers and C2C FFT
	// due to the CS4.0 restriction.
	// Put Dz, Dx and Dy into one buffer because CS4.0 allows only 1 UAV at a time
	createBufferAndUAV(zero_data, 3 * output_size * float2_stride, float2_stride, &m_pBuffer_Float_Dxyz);

	SAFE_DELETE_ARRAY(zero_data);
	SAFE_DELETE_ARRAY(h0_data);
	SAFE_DELETE_ARRAY(omega_data);

	// D3D11 Textures
	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R32G32B32A32_FLOAT, &m_pDisplacementMap);
	createTextureAndViews(hmap_dim, hmap_dim, FORMAT_R16G16B16A16_FLOAT, &m_pGradientMap);

	// Samplers
	SamplerDesc sam_desc;
	sam_desc.Filter = FILTER_MIN_MAG_LINEAR_MIP_POINT;
	sam_desc.AddressU = TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressV = TEXTURE_ADDRESS_WRAP;
	sam_desc.AddressW = TEXTURE_ADDRESS_WRAP;
	sam_desc.MipLODBias = 0;
	sam_desc.MaxAnisotropy = 1;
	sam_desc.ComparisonFunc = COMPARISON_NEVER;
	sam_desc.BorderColor[0] = 1.0f;
	sam_desc.BorderColor[1] = 1.0f;
	sam_desc.BorderColor[2] = 1.0f;
	sam_desc.BorderColor[3] = 1.0f;
	sam_desc.MinLOD = -FLT_MAX;
	sam_desc.MaxLOD = FLT_MAX;
	wiRenderer::GetDevice()->CreateSamplerState(&sam_desc, &m_pPointSamplerState);


	m_pUpdateSpectrumCS = static_cast<ComputeShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanSimulatorCS.cso", wiResourceManager::COMPUTESHADER));

	{
		VertexLayoutDesc layout[] =
		{
			{ "POSITION", 0, FORMAT_R32G32B32A32_FLOAT, 0, 0, INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);
		VertexShaderInfo* vsinfo = static_cast<VertexShaderInfo*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanQuadVS.cso", wiResourceManager::VERTEXSHADER, layout, numElements));
		if (vsinfo != nullptr) {
			m_pQuadVS = vsinfo->vertexShader;
			m_pQuadLayout = vsinfo->vertexLayout;
		}
	}

	m_pUpdateDisplacementPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanUpdateDisplacementPS.cso", wiResourceManager::PIXELSHADER));
	m_pGenGradientFoldingPS = static_cast<PixelShader*>(wiResourceManager::GetShaderManager()->add(wiRenderer::SHADERPATH + "oceanGradientFoldingPS.cso", wiResourceManager::PIXELSHADER));



	//// Compute shaders
	//ID3DBlob* pBlobUpdateSpectrumCS = NULL;

	//CompileShaderFromFile(L"ocean_simulator_cs.hlsl", "UpdateSpectrumCS", "cs_4_0", &pBlobUpdateSpectrumCS);
	//assert(pBlobUpdateSpectrumCS);

	//m_pd3dDevice->CreateComputeShader(pBlobUpdateSpectrumCS->GetBufferPointer(), pBlobUpdateSpectrumCS->GetBufferSize(), NULL, &m_pUpdateSpectrumCS);
	//assert(m_pUpdateSpectrumCS);

	//SAFE_RELEASE(pBlobUpdateSpectrumCS);

	//// Vertex & pixel shaders
	//ID3DBlob* pBlobQuadVS = NULL;
	//ID3DBlob* pBlobUpdateDisplacementPS = NULL;
	//ID3DBlob* pBlobGenGradientFoldingPS = NULL;

	//CompileShaderFromFile(L"ocean_simulator_vs_ps.hlsl", "QuadVS", "vs_4_0", &pBlobQuadVS);
	//CompileShaderFromFile(L"ocean_simulator_vs_ps.hlsl", "UpdateDisplacementPS", "ps_4_0", &pBlobUpdateDisplacementPS);
	//CompileShaderFromFile(L"ocean_simulator_vs_ps.hlsl", "GenGradientFoldingPS", "ps_4_0", &pBlobGenGradientFoldingPS);
	//assert(pBlobQuadVS);
	//assert(pBlobUpdateDisplacementPS);
	//assert(pBlobGenGradientFoldingPS);

	//m_pd3dDevice->CreateVertexShader(pBlobQuadVS->GetBufferPointer(), pBlobQuadVS->GetBufferSize(), NULL, &m_pQuadVS);
	//m_pd3dDevice->CreatePixelShader(pBlobUpdateDisplacementPS->GetBufferPointer(), pBlobUpdateDisplacementPS->GetBufferSize(), NULL, &m_pUpdateDisplacementPS);
	//m_pd3dDevice->CreatePixelShader(pBlobGenGradientFoldingPS->GetBufferPointer(), pBlobGenGradientFoldingPS->GetBufferSize(), NULL, &m_pGenGradientFoldingPS);
	//assert(m_pQuadVS);
	//assert(m_pUpdateDisplacementPS);
	//assert(m_pGenGradientFoldingPS);
	//SAFE_RELEASE(pBlobUpdateDisplacementPS);
	//SAFE_RELEASE(pBlobGenGradientFoldingPS);

	//// Input layout
	//D3D11_INPUT_ELEMENT_DESC quad_layout_desc[] =
	//{
	//	{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	//};
	//m_pd3dDevice->CreateInputLayout(quad_layout_desc, 1, pBlobQuadVS->GetBufferPointer(), pBlobQuadVS->GetBufferSize(), &m_pQuadLayout);
	//assert(m_pQuadLayout);

	//SAFE_RELEASE(pBlobQuadVS);

	// Quad vertex buffer
	GPUBufferDesc vb_desc;
	vb_desc.ByteWidth = 4 * sizeof(XMFLOAT4);
	vb_desc.Usage = USAGE_IMMUTABLE;
	vb_desc.BindFlags = BIND_VERTEX_BUFFER;
	vb_desc.CPUAccessFlags = 0;
	vb_desc.MiscFlags = 0;

	float quad_verts[] =
	{
		-1, -1, 0, 1,
		-1,  1, 0, 1,
		1, -1, 0, 1,
		1,  1, 0, 1,
	};
	SubresourceData init_data;
	init_data.pSysMem = &quad_verts[0];
	init_data.SysMemPitch = 0;
	init_data.SysMemSlicePitch = 0;

	m_pQuadVB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&vb_desc, &init_data, m_pQuadVB);

	// Constant buffers
	UINT actual_dim = m_param.dmap_dim;
	UINT input_width = actual_dim + 4;
	// We use full sized data here. The value "output_width" should be actual_dim/2+1 though.
	UINT output_width = actual_dim;
	UINT output_height = actual_dim;
	UINT dtx_offset = actual_dim * actual_dim;
	UINT dty_offset = actual_dim * actual_dim * 2;
	UINT immutable_consts[] = { actual_dim, input_width, output_width, output_height, dtx_offset, dty_offset };
	SubresourceData init_cb0;
	init_cb0.pSysMem = &immutable_consts[0];

	GPUBufferDesc cb_desc;
	cb_desc.Usage = USAGE_IMMUTABLE;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = 0;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = PAD16(sizeof(immutable_consts));
	m_pImmutableCB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&cb_desc, &init_cb0, m_pImmutableCB);

	cb_desc.Usage = USAGE_DYNAMIC;
	cb_desc.BindFlags = BIND_CONSTANT_BUFFER;
	cb_desc.CPUAccessFlags = CPU_ACCESS_WRITE;
	cb_desc.MiscFlags = 0;
	cb_desc.ByteWidth = PAD16(sizeof(float) * 3);
	m_pPerFrameCB = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&cb_desc, NULL, m_pPerFrameCB);

	// FFT
	fft512x512_create_plan(&m_fft_plan, 3);

#ifdef CS_DEBUG_BUFFER
	GPUBufferDesc buf_desc;
	buf_desc.ByteWidth = 3 * input_half_size * float2_stride;
	buf_desc.Usage = USAGE_STAGING;
	buf_desc.BindFlags = 0;
	buf_desc.CPUAccessFlags = CPU_ACCESS_READ;
	buf_desc.MiscFlags = RESOURCE_MISC_BUFFER_STRUCTURED;
	buf_desc.StructureByteStride = float2_stride;

	m_pDebugBuffer = new GPUBuffer;
	wiRenderer::GetDevice()->CreateBuffer(&buf_desc, NULL, m_pDebugBuffer);
#endif
}

OceanSimulator::~OceanSimulator()
{
	fft512x512_destroy_plan(&m_fft_plan);

	SAFE_DELETE(m_pBuffer_Float2_H0);
	SAFE_DELETE(m_pBuffer_Float_Omega);
	SAFE_DELETE(m_pBuffer_Float2_Ht);
	SAFE_DELETE(m_pBuffer_Float_Dxyz);

	SAFE_DELETE(m_pQuadVB);

	SAFE_DELETE(m_pDisplacementMap);
	SAFE_DELETE(m_pGradientMap);

	SAFE_DELETE(m_pUpdateSpectrumCS);
	SAFE_DELETE(m_pQuadVS);
	SAFE_DELETE(m_pUpdateDisplacementPS);
	SAFE_DELETE(m_pGenGradientFoldingPS);

	SAFE_DELETE(m_pQuadLayout);

	SAFE_DELETE(m_pImmutableCB);
	SAFE_DELETE(m_pPerFrameCB);

#ifdef CS_DEBUG_BUFFER
	SAFE_DELETE(m_pDebugBuffer);
#endif
}


// Initialize the vector field.
// wlen_x: width of wave tile, in meters
// wlen_y: length of wave tile, in meters
void OceanSimulator::initHeightMap(OceanParameter& params, XMFLOAT2* out_h0, float* out_omega)
{
	int i, j;
	XMFLOAT2 K, Kn;

	XMFLOAT2 wind_dir;
	XMStoreFloat2(&wind_dir, XMVector2Normalize(XMLoadFloat2(&params.wind_dir)));
	float a = params.wave_amplitude * 1e-7f;	// It is too small. We must scale it for editing.
	float v = params.wind_speed;
	float dir_depend = params.wind_dependency;

	int height_map_dim = params.dmap_dim;
	float patch_length = params.patch_length;

	// initialize random generator.
	srand(0);

	for (i = 0; i <= height_map_dim; i++)
	{
		// K is wave-vector, range [-|DX/W, |DX/W], [-|DY/H, |DY/H]
		K.y = (-height_map_dim / 2.0f + i) * (2 * XM_PI / patch_length);

		for (j = 0; j <= height_map_dim; j++)
		{
			K.x = (-height_map_dim / 2.0f + j) * (2 * XM_PI / patch_length);

			float phil = (K.x == 0 && K.y == 0) ? 0 : sqrtf(Phillips(K, wind_dir, v, a, dir_depend));

			out_h0[i * (height_map_dim + 4) + j].x = float(phil * Gauss() * HALF_SQRT_2);
			out_h0[i * (height_map_dim + 4) + j].y = float(phil * Gauss() * HALF_SQRT_2);

			// The angular frequency is following the dispersion relation:
			//            out_omega^2 = g*k
			// The equation of Gerstner wave:
			//            x = x0 - K/k * A * sin(dot(K, x0) - sqrt(g * k) * t), x is a 2D vector.
			//            z = A * cos(dot(K, x0) - sqrt(g * k) * t)
			// Gerstner wave shows that a point on a simple sinusoid wave is doing a uniform circular
			// motion with the center (x0, y0, z0), radius A, and the circular plane is parallel to
			// vector K.
			out_omega[i * (height_map_dim + 4) + j] = sqrtf(GRAV_ACCEL * sqrtf(K.x * K.x + K.y * K.y));
		}
	}
}

void OceanSimulator::updateDisplacementMap(float time)
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("OceanSimulator", threadID);

	// ---------------------------- H(0) -> H(t), D(x, t), D(y, t) --------------------------------
	// Compute shader
	device->BindCS(m_pUpdateSpectrumCS, threadID);

	// Buffers
	GPUResource* cs0_srvs[2] = { 
		m_pBuffer_Float2_H0, 
		m_pBuffer_Float_Omega
	};
	device->BindResourcesCS(cs0_srvs, 0, 2, threadID);

	GPUUnorderedResource* cs0_uavs[1] = { m_pBuffer_Float2_Ht };
	device->BindUnorderedAccessResourcesCS(cs0_uavs, 0, 1, threadID);

	// Consts
	//D3D11_MAPPED_SUBRESOURCE mapped_res;
	//m_pd3dImmediateContext->Map(m_pPerFrameCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_res);
	//assert(mapped_res.pData);
	//float per_frame_data = (float*)mapped_res.pData;
	//// g_Time
	//per_frame_data[0] = time * m_param.time_scale;
	//// g_ChoppyScale
	//per_frame_data[1] = m_param.choppy_scale;
	//// g_GridLen
	//per_frame_data[2] = m_param.dmap_dim / m_param.patch_length;
	//m_pd3dImmediateContext->Unmap(m_pPerFrameCB, 0);

	float per_frame_data[3] = {
		time * m_param.time_scale,					// g_Time
		m_param.choppy_scale,						// g_ChoppyScale
		m_param.dmap_dim / m_param.patch_length,	// g_GridLen
	};
	device->UpdateBuffer(m_pPerFrameCB, per_frame_data, threadID, sizeof(per_frame_data));

	//ID3D11Buffer* cs_cbs[2] = { m_pImmutableCB, m_pPerFrameCB };
	//m_pd3dImmediateContext->CSSetConstantBuffers(0, 2, cs_cbs);
	device->BindConstantBufferCS(m_pImmutableCB, 0, threadID);
	device->BindConstantBufferCS(m_pPerFrameCB, 1, threadID);

	// Run the CS
	UINT group_count_x = (m_param.dmap_dim + BLOCK_SIZE_X - 1) / BLOCK_SIZE_X;
	UINT group_count_y = (m_param.dmap_dim + BLOCK_SIZE_Y - 1) / BLOCK_SIZE_Y;
	device->Dispatch(group_count_x, group_count_y, 1, threadID);

	//// Unbind resources for CS
	//cs0_uavs[0] = NULL;
	//m_pd3dImmediateContext->CSSetUnorderedAccessViews(0, 1, cs0_uavs, (UINT*)(&cs0_uavs[0]));
	//cs0_srvs[0] = NULL;
	//cs0_srvs[1] = NULL;
	//m_pd3dImmediateContext->CSSetShaderResources(0, 2, cs0_srvs);

	device->UnBindUnorderedAccessResources(0, 1, threadID);
	device->UnBindResources(0, 2, threadID);


	// ------------------------------------ Perform FFT -------------------------------------------
	fft_512x512_c2c(&m_fft_plan, m_pBuffer_Float_Dxyz, m_pBuffer_Float_Dxyz, m_pBuffer_Float2_Ht);

	// --------------------------------- Wrap Dx, Dy and Dz ---------------------------------------
	// Push RT
	//ID3D11RenderTargetView* old_target;
	//ID3D11DepthStencilView* old_depth;
	//m_pd3dImmediateContext->OMGetRenderTargets(1, &old_target, &old_depth);
	//D3D11_VIEWPORT old_viewport;
	//UINT num_viewport = 1;
	//m_pd3dImmediateContext->RSGetViewports(&num_viewport, &old_viewport);

	ViewPort new_vp;
	new_vp.TopLeftX = 0;
	new_vp.TopLeftX = 0;
	new_vp.Width = (float)m_param.dmap_dim;
	new_vp.Height = (float)m_param.dmap_dim;
	new_vp.MinDepth = 0.0f;
	new_vp.MaxDepth = 1.0f;
	device->BindViewports(1, &new_vp, threadID);

	// Set RT
	device->BindRenderTargets(1, (Texture**)&m_pDisplacementMap, nullptr, threadID);

	// VS & PS
	device->BindVS(m_pQuadVS, threadID);
	device->BindPS(m_pUpdateDisplacementPS, threadID);

	// Constants
	//ID3D11Buffer* ps_cbs[2] = { m_pImmutableCB, m_pPerFrameCB };
	//m_pd3dImmediateContext->PSSetConstantBuffers(0, 2, ps_cbs);
	device->BindConstantBufferPS(m_pImmutableCB, 0, threadID);
	device->BindConstantBufferPS(m_pPerFrameCB, 1, threadID);

	// Buffer resources
	GPUResource* ps_srvs[1] = { m_pBuffer_Float_Dxyz };
	device->BindResourcesPS(ps_srvs, 0, 1, threadID);

	// IA setup
	GPUBuffer* vbs[1] = { m_pQuadVB };
	UINT strides[1] = { sizeof(XMFLOAT4) };
	UINT offsets[1] = { 0 };
	device->BindVertexBuffers(&vbs[0], 0, 1, &strides[0], &offsets[0], threadID);

	device->BindVertexLayout(m_pQuadLayout, threadID);
	device->BindPrimitiveTopology(TRIANGLESTRIP, threadID);

	// Perform draw call
	device->Draw(4, 0, threadID);

	// Unbind
	device->UnBindResources(0, 1, threadID);


	// ----------------------------------- Generate Normal ----------------------------------------
	// Set RT
	device->BindRenderTargets(1, (Texture**)&m_pGradientMap, nullptr, threadID);

	// VS & PS
	device->BindVS(m_pQuadVS, threadID);
	device->BindPS(m_pGenGradientFoldingPS, threadID);

	// Texture resource and sampler
	ps_srvs[0] = m_pDisplacementMap;
	device->BindResourcesPS(ps_srvs, 0, 1, threadID);

	device->BindSamplerPS(&m_pPointSamplerState, 0, threadID);

	// Perform draw call
	device->Draw(4, 0, threadID);

	// Unbind
	device->UnBindResources(0, 1, threadID);

	//// Pop RT
	//m_pd3dImmediateContext->RSSetViewports(1, &old_viewport);
	//m_pd3dImmediateContext->OMSetRenderTargets(1, &old_target, old_depth);
	//SAFE_RELEASE(old_target);
	//SAFE_RELEASE(old_depth);

	device->GenerateMips(m_pGradientMap, threadID);

	// Define CS_DEBUG_BUFFER to enable writing a buffer into a file.
#ifdef CS_DEBUG_BUFFER
	{
		m_pd3dImmediateContext->CopyResource(m_pDebugBuffer, m_pBuffer_Float_Dxyz);
		D3D11_MAPPED_SUBRESOURCE mapped_res;
		m_pd3dImmediateContext->Map(m_pDebugBuffer, 0, D3D11_MAP_READ, 0, &mapped_res);

		// set a break point below, and drag MappedResource.pData into in your Watch window
		// and cast it as (float*)

		// Write to disk
		XMFLOAT2* v = (XMFLOAT2*)mapped_res.pData;

		FILE* fp = fopen(".\\tmp\\Ht_raw.dat", "wb");
		fwrite(v, 512 * 512 * sizeof(float) * 2 * 3, 1, fp);
		fclose(fp);

		m_pd3dImmediateContext->Unmap(m_pDebugBuffer, 0);
	}
#endif

	device->EventEnd(threadID);
}

Texture2D* OceanSimulator::getD3D11DisplacementMap()
{
	return m_pDisplacementMap;
}

Texture2D* OceanSimulator::getD3D11GradientMap()
{
	return m_pGradientMap;
}


const OceanParameter& OceanSimulator::getParameters()
{
	return m_param;
}
